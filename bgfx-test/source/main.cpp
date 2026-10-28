// Smoke test: bgfx (OpenGL ES 3.2 via EGL, statically linked) on the Switch's native window,
// with SDL3 providing the window/events. bgfx creates its own EGL surface on the default NWindow, so
// the SDL window is created WITHOUT SDL_WINDOW_OPENGL (see patches/sdl3-switch.patch).
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>

#include <switch.h>

#include <SDL3/SDL.h>
#include <bgfx/bgfx.h>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fflush(stderr); } while (0)

struct Callback : bgfx::CallbackI {
    void fatal(const char* file, uint16_t line, bgfx::Fatal::Enum code, const char* str) override
    {
        LOG("!!! bgfx fatal %d at %s:%u: %s\n", (int)code, file, line, str);
        fflush(stderr);
        abort();
    }
    void traceVargs(const char* file, uint16_t line, const char* format, va_list args) override
    {
        char buf[1024];
        vsnprintf(buf, sizeof(buf), format, args);
        LOG("bgfx: %s:%u %s", file, line, buf);
    }
    void profilerBegin(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerBeginLiteral(const char*, uint32_t, const char*, uint16_t) override {}
    void profilerEnd() override {}
    uint32_t cacheReadSize(uint64_t) override { return 0; }
    bool cacheRead(uint64_t, void*, uint32_t) override { return false; }
    void cacheWrite(uint64_t, const void*, uint32_t) override {}
    void screenShot(const char*, uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, const void*, uint32_t, bool) override {}
    void captureBegin(uint32_t, uint32_t, uint32_t, bgfx::TextureFormat::Enum, bool) override {}
    void captureEnd() override {}
    void captureFrame(const void*, uint32_t) override {}
};

int main(int, char**)
{
    socketInitializeDefault();
    nxlinkConnectToHost(false, true);
    LOG("bgfx test on HOS\n");

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK)) {
        LOG("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* win = SDL_CreateWindow("bgfx-test", 1280, 720, 0);  // no SDL_WINDOW_OPENGL on purpose
    if (!win) {
        LOG("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(win, &w, &h);
    LOG("SDL window %dx%d\n", w, h);

    Callback callback;
    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGLES;
    // this bgfx fork (vpinball's) takes the main window as a swap chain
    init.swapChain.nwh = nwindowGetDefault();
    init.swapChain.ndt = nullptr;
    init.swapChain.width = w;
    init.swapChain.height = h;
    init.reset = BGFX_RESET_VSYNC;
    init.callback = &callback;
    if (!bgfx::init(init)) {
        LOG("bgfx::init failed\n");
        return 1;
    }

    const bgfx::Caps* caps = bgfx::getCaps();
    LOG("renderer: %s, vendor 0x%x, device 0x%x\n", bgfx::getRendererName(caps->rendererType), caps->vendorId, caps->deviceId);
    LOG("max texture %u, max fb attachments %u, max draw calls %u\n", caps->limits.maxTextureSize,
        caps->limits.maxFBAttachments, caps->limits.maxDrawCalls);
    LOG("caps: compute=%d index32=%d draw_indirect=%d\n", !!(caps->supported & BGFX_CAPS_COMPUTE),
        !!(caps->supported & BGFX_CAPS_INDEX32), !!(caps->supported & BGFX_CAPS_DRAW_INDIRECT));

    bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);
    bgfx::setViewRect(0, 0, 0, uint16_t(w), uint16_t(h));

    const uint32_t start = SDL_GetTicks();
    uint32_t last = start;
    unsigned frames = 0;
    while (SDL_GetTicks() - start < 15000) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
        }
        const float t = (SDL_GetTicks() - start) / 1000.0f;
        const uint8_t r = uint8_t(20 + 20 * (1 + sinf(t)));
        const uint8_t g = uint8_t(30 + 20 * (1 + sinf(t * 1.3f)));
        const uint8_t b = uint8_t(60 + 40 * (1 + sinf(t * 0.7f)));
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, (r << 24) | (g << 16) | (b << 8) | 0xff, 1.0f, 0);
        bgfx::touch(0);
        bgfx::dbgTextClear();
        bgfx::dbgTextPrintf(2, 2, 0x0f, "bgfx %s on Nintendo Switch (HOS)", bgfx::getRendererName(caps->rendererType));
        bgfx::dbgTextPrintf(2, 4, 0x0a, "SDL3 window + native NWindow, GLES via nouveau");
        bgfx::dbgTextPrintf(2, 6, 0x0e, "t = %.1f s", t);
        bgfx::frame();
        frames++;
        if (SDL_GetTicks() - last >= 1000) {
            LOG("fps %u\n", frames);
            frames = 0;
            last = SDL_GetTicks();
        }
    }

    LOG("done\n");
    bgfx::shutdown();
    SDL_DestroyWindow(win);
    SDL_Quit();
    fflush(stderr);
    socketExit();
    return 0;
}
