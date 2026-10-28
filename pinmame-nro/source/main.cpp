// libnx host for libpinmame: draws the game's DMD to the screen and plays its audio.
//
// Threading: PinMAME calls every callback from its own emulation thread. Callbacks therefore only
// copy data into mutex-protected buffers; the main thread draws, an audio thread feeds audout.
// The libnx console is not thread-safe and cannot coexist with a framebuffer, so all logging
// goes to nxlink (stderr).
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <switch.h>

#include "libpinmame.h"

static const char* kBasePath = "/switch/vpnx/pinmame/";

constexpr int kFbWidth = 1280;
constexpr int kFbHeight = 720;

constexpr int kAudioRate = 48000;          // audout is fixed to 48 kHz stereo s16
constexpr int kAudioFrames = 1024;         // frames per audout buffer: 4096 bytes = one page
constexpr int kAudioBuffers = 4;
constexpr size_t kRingFrames = 8192;       // ~170 ms; the oldest audio is dropped beyond this

// --- logging -----------------------------------------------------------------------------------

// Cleared before socketExit(): stderr is redirected to the nxlink socket, and writing to it after
// the socket is closed faults inside newlib's _write_r (PinMAME can still log while stopping).
static std::atomic<bool> g_logEnabled{true};

static void LogV(const char* format, va_list args)
{
    if (!g_logEnabled)
        return;
    char buf[1024];
    vsnprintf(buf, sizeof(buf), format, args);
    fputs(buf, stderr);
}

static void Log(const char* format, ...) __attribute__((format(printf, 1, 2)));
static void Log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogV(format, args);
    va_end(args);
}

#define LOG(...) Log(__VA_ARGS__)

// --- exit tracing ------------------------------------------------------------------------------
// Linked with -Wl,--wrap=exit,--wrap=abort,--wrap=__assert_func: logs who ends the process.
// Resolve callers with anchor_rt/anchor_static as for crash pcs.
extern "C" {
void __real_exit(int code);
void __real_abort(void);
void __real___assert_func(const char* file, int line, const char* func, const char* expr);

void __wrap_exit(int code)
{
    LOG("!!! exit(%d) called from 0x%lx\n", code, (unsigned long)__builtin_return_address(0));
    fflush(stderr);
    __real_exit(code);
}

void __wrap_abort(void)
{
    LOG("!!! abort() called from 0x%lx\n", (unsigned long)__builtin_return_address(0));
    fflush(stderr);
    __real_abort();
}

void __wrap___assert_func(const char* file, int line, const char* func, const char* expr)
{
    LOG("!!! assert failed: %s:%d %s: %s\n", file, line, func ? func : "?", expr);
    fflush(stderr);
    __real___assert_func(file, line, func, expr);
}
}

// --- crash reporting ---------------------------------------------------------------------------
// Only catches faults on threads libnx routes to the handler (svcBreak on other threads is
// reported by Atmosphere in /atmosphere/crash_reports instead). The next launch prints the dump.
static const char* kDumpPath = "/switch/vpnx/pinmame/exception_dump.txt";

// address anchor for turning runtime pcs into ELF addresses (NRO load base is randomised)
__attribute__((used, noinline)) void CrashAnchor() {}

alignas(16) u8 __nx_exception_stack[0x4000];
u64 __nx_exception_stack_size = sizeof(__nx_exception_stack);

extern "C" void __libnx_exception_handler(ThreadExceptionDump* ctx)
{
    FILE* f = fopen(kDumpPath, "w");
    if (!f)
        return;
    fprintf(f, "anchor_rt: 0x%lx\n", (unsigned long)&CrashAnchor);
    fprintf(f, "error_desc: 0x%x\n", ctx->error_desc);
    fprintf(f, "pc: 0x%lx\nlr: 0x%lx\nsp: 0x%lx\nfp: 0x%lx\nfar: 0x%lx\nesr: 0x%x\n",
            (unsigned long)ctx->pc.x, (unsigned long)ctx->lr.x, (unsigned long)ctx->sp.x,
            (unsigned long)ctx->fp.x, (unsigned long)ctx->far.x, ctx->esr);
    fclose(f);
}

static void PrintPreviousCrash()
{
    FILE* f = fopen(kDumpPath, "r");
    if (!f)
        return;
    LOG("\n!!! previous run crashed:\n");
    char line[128];
    while (fgets(line, sizeof(line), f))
        LOG("  %s", line);
    fclose(f);
    remove(kDumpPath);
}

// --- DMD ---------------------------------------------------------------------------------------

struct DmdState {
    std::mutex mutex;
    int index = -1;              // display to show: the largest DMD-like (2/4 bit) display
    int width = 0, height = 0, depth = 0;
    std::vector<u8> pixels;      // raw shade per dot, 0..3 (depth 2) or 0..15 (depth 4)
    std::atomic<unsigned> frames{0};
};
static DmdState g_dmd;

static void PINMAMECALLBACK OnDisplayAvailable(int index, int count, PinmameDisplayLayout* l, void*)
{
    LOG("display %d/%d: %dx%d depth %d\n", index + 1, count, l->width, l->height, l->depth);
    if ((l->depth != 2 && l->depth != 4) || l->width <= 0 || l->height <= 0)
        return;
    std::lock_guard<std::mutex> lock(g_dmd.mutex);
    if (l->width * l->height >= g_dmd.width * g_dmd.height) {
        g_dmd.index = index;
        g_dmd.width = l->width;
        g_dmd.height = l->height;
        g_dmd.depth = l->depth;
        g_dmd.pixels.assign(static_cast<size_t>(l->width) * l->height, 0);
    }
}

static void PINMAMECALLBACK OnDisplayUpdated(int index, void* data, PinmameDisplayLayout*, void*)
{
    if (!data || index != g_dmd.index)
        return;
    std::lock_guard<std::mutex> lock(g_dmd.mutex);
    memcpy(g_dmd.pixels.data(), data, g_dmd.pixels.size());
    g_dmd.frames++;
}

// Round dots, integer-scaled to fit the screen, as pre-rendered sprites (one per shade).
class DmdRenderer {
public:
    void Draw(u32* fb, u32 strideBytes)
    {
        {
            std::lock_guard<std::mutex> lock(g_dmd.mutex);
            m_width = g_dmd.width;
            m_height = g_dmd.height;
            m_levels = g_dmd.depth == 4 ? 16 : 4;
            m_pixels = g_dmd.pixels;
        }
        memset(fb, 0, static_cast<size_t>(strideBytes) * kFbHeight);
        if (m_width <= 0 || m_pixels.size() != static_cast<size_t>(m_width) * m_height)
            return;

        const int cell = std::max(1, std::min(kFbWidth / m_width, kFbHeight / m_height));
        if (cell != m_cell || m_levels != m_spriteLevels)
            BuildSprites(cell, m_levels);

        const int x0 = (kFbWidth - m_width * cell) / 2;
        const int y0 = (kFbHeight - m_height * cell) / 2;
        const u32 stride = strideBytes / sizeof(u32);
        for (int y = 0; y < m_height; y++) {
            for (int x = 0; x < m_width; x++) {
                const u32* sprite = &m_sprites[static_cast<size_t>(m_pixels[y * m_width + x] % m_levels) * cell * cell];
                u32* dst = fb + static_cast<size_t>(y0 + y * cell) * stride + x0 + x * cell;
                for (int row = 0; row < cell; row++, dst += stride)
                    memcpy(dst, sprite + row * cell, cell * sizeof(u32));
            }
        }
    }

private:
    void BuildSprites(int cell, int levels)
    {
        m_cell = cell;
        m_spriteLevels = levels;
        m_sprites.assign(static_cast<size_t>(levels) * cell * cell, RGBA8_MAXALPHA(0, 0, 0));
        const float c = (cell - 1) / 2.0f, radius = cell * 0.44f;
        for (int level = 0; level < levels; level++) {
            const float i = static_cast<float>(level) / (levels - 1);
            // unlit dots stay faintly visible, like a real DMD
            const u32 color = RGBA8_MAXALPHA(static_cast<int>(28 + 227 * i), static_cast<int>(10 + 100 * i), static_cast<int>(2 + 8 * i));
            for (int y = 0; y < cell; y++)
                for (int x = 0; x < cell; x++)
                    if ((x - c) * (x - c) + (y - c) * (y - c) <= radius * radius)
                        m_sprites[(static_cast<size_t>(level) * cell + y) * cell + x] = color;
        }
    }

    int m_width = 0, m_height = 0, m_levels = 4;
    std::vector<u8> m_pixels;
    int m_cell = 0, m_spriteLevels = 0;
    std::vector<u32> m_sprites;
};

// --- audio -------------------------------------------------------------------------------------

struct AudioRing {
    std::mutex mutex;
    std::vector<s16> data = std::vector<s16>(kRingFrames * 2);  // interleaved stereo
    size_t read = 0, count = 0;                                 // in frames
    unsigned long long produced = 0, consumed = 0, dropped = 0, underruns = 0;
    int peak = 0;  // largest |sample| written since the last stats line (0 = PinMAME sent silence)

    void Write(const s16* in, int frames)
    {
        std::lock_guard<std::mutex> lock(mutex);
        for (int i = 0; i < frames * 2; i++)
            peak = std::max(peak, std::abs(static_cast<int>(in[i])));
        for (int i = 0; i < frames; i++) {
            if (count == kRingFrames) {  // full: drop the oldest frame to bound latency
                read = (read + 1) % kRingFrames;
                count--;
                dropped++;
            }
            const size_t w = (read + count) % kRingFrames;
            data[w * 2] = in[i * 2];
            data[w * 2 + 1] = in[i * 2 + 1];
            count++;
        }
        produced += frames;
    }

    // Always fills `frames` frames; missing audio becomes silence.
    void Read(s16* out, int frames)
    {
        std::lock_guard<std::mutex> lock(mutex);
        int i = 0;
        for (; i < frames && count > 0; i++) {
            out[i * 2] = data[read * 2];
            out[i * 2 + 1] = data[read * 2 + 1];
            read = (read + 1) % kRingFrames;
            count--;
        }
        if (i < frames) {
            memset(out + i * 2, 0, (frames - i) * 2 * sizeof(s16));
            underruns += frames - i;
        }
        consumed += i;
    }
};
static AudioRing g_ring;
static int g_pinmameChannels = 1;
static std::atomic<bool> g_audioRun{true};

static int PINMAMECALLBACK OnAudioAvailable(PinmameAudioInfo* info, void*)
{
    g_pinmameChannels = info->channels;
    LOG("audio: %d ch, %.0f Hz, %d samples/frame\n", info->channels, info->sampleRate, info->samplesPerFrame);
    return info->samplesPerFrame;
}

static int PINMAMECALLBACK OnAudioUpdated(void* buffer, int samples, void*)
{
    // PinMAME delivers int16 (mono or stereo); audout wants stereo
    const s16* in = static_cast<const s16*>(buffer);
    if (g_pinmameChannels == 2) {
        g_ring.Write(in, samples);
    } else {
        std::vector<s16> stereo(static_cast<size_t>(samples) * 2);
        for (int i = 0; i < samples; i++)
            stereo[i * 2] = stereo[i * 2 + 1] = in[i];
        g_ring.Write(stereo.data(), samples);
    }
    return samples;
}

static void AudioThread()
{
    const Result initRc = audoutInitialize();
    const Result startRc = R_SUCCEEDED(initRc) ? audoutStartAudioOut() : initRc;
    LOG("audoutInitialize/Start: rc=0x%x\n", startRc);
    if (R_FAILED(startRc))
        return;
    unsigned appendFailures = 0, waitFailures = 0;
    float volume = -1;
    Result vrc = audoutSetAudioOutVolume(1.0f);
    audoutGetAudioOutVolume(&volume);
    LOG("audout volume set: rc=0x%x, now %.2f\n", vrc, volume);
    LOG("audout: %u Hz, %u ch, pcm format %d\n", audoutGetSampleRate(), audoutGetChannelCount(), (int)audoutGetPcmFormat());
    constexpr size_t kBytes = kAudioFrames * 2 * sizeof(s16);
    AudioOutBuffer bufs[kAudioBuffers] = {};
    void* mem[kAudioBuffers];

    // Startup test tone, generated here before any PinMAME audio: three 0.3 s beeps (660 Hz) with
    // 0.3 s gaps, ~1.8 s in total. If it is audible the audout path works and any silence afterwards
    // comes from the emulated game. (~14 buffers of 1024 frames = 0.3 s)
    constexpr int kBeepStepBuffers = 14, kBeepSteps = 6;
    int beepStep = 0;  // counts buffers; even 14-buffer blocks are beeps, odd ones are gaps
    double phase = 0;
    auto submit = [&](int i) {
        s16* out = static_cast<s16*>(mem[i]);
        if (beepStep < kBeepSteps * kBeepStepBuffers) {
            const bool on = ((beepStep / kBeepStepBuffers) % 2) == 0;
            for (int f = 0; f < kAudioFrames; f++) {
                phase += 2.0 * M_PI * 660.0 / kAudioRate;
                out[f * 2] = out[f * 2 + 1] = on ? static_cast<s16>(12000 * std::sin(phase)) : 0;
            }
            beepStep++;
        } else {
            g_ring.Read(out, kAudioFrames);
        }
        bufs[i].next = nullptr;
        bufs[i].buffer = mem[i];
        bufs[i].buffer_size = kBytes;
        bufs[i].data_size = kBytes;
        bufs[i].data_offset = 0;
        if (R_FAILED(audoutAppendAudioOutBuffer(&bufs[i])))
            appendFailures++;
    };

    for (int i = 0; i < kAudioBuffers; i++) {
        mem[i] = aligned_alloc(0x1000, kBytes);
        memset(mem[i], 0, kBytes);
        submit(i);
    }

    // Buffers finish in the order they were queued, so refill them round-robin.
    int next = 0;
    while (g_audioRun) {
        AudioOutBuffer* released = nullptr;
        u32 count = 0;
        if (R_FAILED(audoutWaitPlayFinish(&released, &count, 100000000ULL))) {
            waitFailures++;
            continue;
        }
        for (u32 n = 0; n < count; n++) {
            submit(next);
            next = (next + 1) % kAudioBuffers;
        }
    }

    LOG("audout: %u append failures, %u wait failures\n", appendFailures, waitFailures);
    audoutStopAudioOut();
    audoutExit();
    for (void* m : mem)
        free(m);
}

// --- PinMAME glue ------------------------------------------------------------------------------

// --- clocks ------------------------------------------------------------------------------------

static ClkrstSession g_cpuClk;
static bool g_cpuClkOpen = false;
static u32 g_cpuClkOriginal = 0;

static u32 GetCpuClockHz()
{
    u32 hz = 0;
    if (g_cpuClkOpen)
        clkrstGetClockRate(&g_cpuClk, &hz);
    return hz;
}

static void InitClocks(bool overclock)
{
    if (R_FAILED(clkrstInitialize()) || R_FAILED(clkrstOpenSession(&g_cpuClk, PcvModuleId_CpuBus, 3))) {
        LOG("clkrst unavailable\n");
        return;
    }
    g_cpuClkOpen = true;
    g_cpuClkOriginal = GetCpuClockHz();
    LOG("applet type %d, CPU clock %u MHz\n", (int)appletGetAppletType(), g_cpuClkOriginal / 1000000);
    if (overclock) {
        Result rc = clkrstSetClockRate(&g_cpuClk, 1785000000);
        LOG("overclock requested: rc=0x%x, CPU clock now %u MHz\n", rc, GetCpuClockHz() / 1000000);
    }
}

static void ExitClocks()
{
    if (!g_cpuClkOpen)
        return;
    if (GetCpuClockHz() != g_cpuClkOriginal)
        clkrstSetClockRate(&g_cpuClk, g_cpuClkOriginal);  // hand the clock back as we found it
    clkrstCloseSession(&g_cpuClk);
    clkrstExit();
}

static void PINMAMECALLBACK OnStateUpdated(int state, void*) { LOG("state: %d\n", state); }
static void PINMAMECALLBACK OnLogMessage(PINMAME_LOG_LEVEL, const char* format, va_list args, void*) { LogV(format, args); }
static int PINMAMECALLBACK IsKeyPressed(PINMAME_KEYCODE, void*) { return 0; }

static void PINMAMECALLBACK CollectGame(PinmameGame* game, void* user)
{
    if (!game->found)
        return;
    static_cast<std::vector<std::string>*>(user)->emplace_back(game->name);
    LOG("  rom set: %-12s %s %s (%s)\n", game->name, game->year, game->manufacturer, game->description);
}

int main(int argc, char** argv)
{
    socketInitializeDefault();
    nxlinkConnectToHost(false, true);
    PrintPreviousCrash();
    bool overclock = false;
    for (int i = 1; i < argc; i++)
        overclock |= std::string(argv[i]) == "oc";
    InitClocks(overclock);
    LOG("anchor_rt: 0x%lx\n", (unsigned long)&CrashAnchor);
    LOG("libpinmame on HOS\nROMs: %sroms/*.zip\n", kBasePath);

    NWindow* win = nwindowGetDefault();
    Framebuffer fb;
    framebufferCreate(&fb, win, kFbWidth, kFbHeight, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    PinmameConfig config = {
        PINMAME_AUDIO_FORMAT_INT16,
        kAudioRate,
        "",
        &OnStateUpdated,
        &OnDisplayAvailable,
        &OnDisplayUpdated,
        &OnAudioAvailable,
        &OnAudioUpdated,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        &IsKeyPressed,
        &OnLogMessage,
        nullptr,
    };
    snprintf(const_cast<char*>(config.vpmPath), PINMAME_MAX_PATH, "%s", kBasePath);

    PinmameSetConfig(&config);
    PinmameSetCheat(0);
    PinmameSetHandleKeyboard(0);
    PinmameSetHandleMechanics(0);
    PinmameSetDmdMode(PINMAME_DMD_MODE_RAW);

    std::vector<std::string> found;
    PinmameGetGames(&CollectGame, &found);
    LOG("games with ROMs found: %zu\n", found.size());

    std::thread audio(AudioThread);

    bool running = false;
    if (!found.empty()) {
        // nxlink --args "<romname> [oc]" picks the game (default: first found); "oc" raises the CPU to 1785 MHz
        std::string rom = found[0];
        for (int i = 1; i < argc; i++)
            for (const auto& name : found)
                if (name == argv[i])
                    rom = name;
        LOG("running %s\n", rom.c_str());
        running = PinmameRun(rom.c_str()) == PINMAME_STATUS_OK;
        LOG("PinmameRun: %s\n", running ? "OK" : "FAILED");
    } else {
        LOG("no ROM found in %sroms\n", kBasePath);
    }

    DmdRenderer dmd;
    u64 lastStat = armGetSystemTick();
    unsigned frames = 0, lastDmdFrames = 0;
    u64 drawTicks = 0;
    unsigned long long lastProduced = 0, lastConsumed = 0, lastUnderruns = 0, lastDropped = 0;
    const char* endReason = "appletMainLoop() returned false";

    while (appletMainLoop()) {
        padUpdate(&pad);
        if (padGetButtonsDown(&pad) & HidNpadButton_Plus) {
            endReason = "+ pressed";
            break;
        }

        u32 stride;
        u32* buf = static_cast<u32*>(framebufferBegin(&fb, &stride));
        const u64 t0 = armGetSystemTick();
        dmd.Draw(buf, stride);
        drawTicks += armGetSystemTick() - t0;
        framebufferEnd(&fb);
        frames++;

        const u64 now = armGetSystemTick();
        if (armTicksToNs(now - lastStat) >= 1000000000ULL) {
            lastStat = now;
            unsigned long long produced, consumed, underruns, dropped;
            int peak;
            {
                std::lock_guard<std::mutex> lock(g_ring.mutex);
                produced = g_ring.produced;
                consumed = g_ring.consumed;
                underruns = g_ring.underruns;
                dropped = g_ring.dropped;
                peak = g_ring.peak;
                g_ring.peak = 0;
            }
            const unsigned dmdFrames = g_dmd.frames;
            LOG("fps %u  draw %.2f ms  dmd +%u  audio prod +%llu cons +%llu underrun +%llu drop +%llu peak %d  cpu %uMHz running=%d\n",
                frames, armTicksToNs(drawTicks / (frames ? frames : 1)) / 1e6, dmdFrames - lastDmdFrames,
                produced - lastProduced, consumed - lastConsumed, underruns - lastUnderruns, dropped - lastDropped, peak, GetCpuClockHz() / 1000000,
                running ? PinmameIsRunning() : 0);
            frames = 0;
            drawTicks = 0;
            lastDmdFrames = dmdFrames;
            lastProduced = produced;
            lastConsumed = consumed;
            lastUnderruns = underruns;
            lastDropped = dropped;
        }
    }

    LOG("main loop ended: %s (running=%d)\n", endReason, running ? PinmameIsRunning() : 0);
    if (running)
        PinmameStop();
    g_audioRun = false;
    audio.join();
    ExitClocks();
    framebufferClose(&fb);
    fflush(stderr);
    g_logEnabled = false;
    socketExit();
    return 0;
}
