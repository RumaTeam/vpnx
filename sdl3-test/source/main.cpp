// Smoke test for the devkitPro SDL3 Switch port: everything vpinball's platform layer needs from
// SDL (video + GL ES context, audio stream, joysticks, events, timing, paths). Logs go to nxlink.
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include <switch.h>

#include <SDL3/SDL.h>
#include <GLES3/gl3.h>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); fflush(stderr); } while (0)

static SDL_AudioStream* g_stream = nullptr;
static double g_phase = 0;

// 440 Hz tone, pushed on demand
static void SDLCALL AudioCb(void*, SDL_AudioStream* stream, int additional, int)
{
    std::vector<Sint16> buf(additional / sizeof(Sint16) + 2);
    for (size_t i = 0; i + 1 < buf.size(); i += 2) {
        g_phase += 2.0 * M_PI * 440.0 / 48000.0;
        buf[i] = buf[i + 1] = static_cast<Sint16>(8000 * std::sin(g_phase));
    }
    SDL_PutAudioStreamData(stream, buf.data(), additional);
}

int main(int, char**)
{
    socketInitializeDefault();
    nxlinkConnectToHost(false, true);
    LOG("SDL3 test: SDL %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_MICRO_VERSION);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_JOYSTICK | SDL_INIT_EVENTS)) {
        LOG("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    LOG("video driver: %s, audio driver: %s\n", SDL_GetCurrentVideoDriver(), SDL_GetCurrentAudioDriver());
    LOG("base path: %s\n", SDL_GetBasePath() ? SDL_GetBasePath() : "(null)");

    int displays = 0;
    SDL_DisplayID* ids = SDL_GetDisplays(&displays);
    for (int i = 0; i < displays; i++) {
        const SDL_DisplayMode* m = SDL_GetDesktopDisplayMode(ids[i]);
        if (m)
            LOG("display %d: %dx%d @ %.1f Hz\n", i, m->w, m->h, m->refresh_rate);
    }
    SDL_free(ids);

    // OpenGL ES 3.0 window, the same path bgfx's GLES renderer would use
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_Window* win = SDL_CreateWindow("sdl3-test", 1280, 720, SDL_WINDOW_OPENGL);
    if (!win) {
        LOG("SDL_CreateWindow failed: %s\n", SDL_GetError());
        return 1;
    }
    int w = 0, h = 0;
    SDL_GetWindowSizeInPixels(win, &w, &h);
    LOG("window %dx%d, props id %u\n", w, h, (unsigned)SDL_GetWindowProperties(win));

    SDL_GLContext ctx = SDL_GL_CreateContext(win);
    if (!ctx) {
        LOG("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_GL_SetSwapInterval(1);
    LOG("GL_VENDOR:   %s\nGL_RENDERER: %s\nGL_VERSION:  %s\nGLSL:        %s\n", glGetString(GL_VENDOR),
        glGetString(GL_RENDERER), glGetString(GL_VERSION), glGetString(GL_SHADING_LANGUAGE_VERSION));
    GLint maxTex = 0, maxUnits = 0, ext = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTex);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxUnits);
    glGetIntegerv(GL_NUM_EXTENSIONS, &ext);
    LOG("max texture %d, texture units %d, %d extensions\n", maxTex, maxUnits, ext);

    // Audio: a 440 Hz tone through an SDL audio stream (what vpinball's audio code uses)
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, 48000 };
    g_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, AudioCb, nullptr);
    if (g_stream) {
        SDL_ResumeAudioStreamDevice(g_stream);
        LOG("audio stream opened\n");
    } else {
        LOG("SDL_OpenAudioDeviceStream failed: %s\n", SDL_GetError());
    }

    int njoy = 0;
    SDL_JoystickID* joys = SDL_GetJoysticks(&njoy);
    LOG("joysticks: %d\n", njoy);
    for (int i = 0; i < njoy; i++)
        LOG("  joystick %d: %s\n", i, SDL_GetJoystickNameForID(joys[i]));
    SDL_free(joys);

    Uint64 start = SDL_GetTicks(), last = start;
    unsigned frames = 0;
    bool running = true;
    while (running && SDL_GetTicks() - start < 15000) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_EVENT_QUIT)
                running = false;
            else if (e.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN)
                LOG("joystick %u button %d\n", (unsigned)e.jbutton.which, e.jbutton.button);
            else if (e.type == SDL_EVENT_JOYSTICK_ADDED)
                LOG("joystick added %u\n", (unsigned)e.jdevice.which);
        }
        const float t = (SDL_GetTicks() - start) / 1000.0f;
        glClearColor(0.5f + 0.5f * std::sin(t), 0.5f + 0.5f * std::sin(t * 1.7f), 0.5f + 0.5f * std::sin(t * 2.3f), 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SDL_GL_SwapWindow(win);
        frames++;
        if (SDL_GetTicks() - last >= 1000) {
            LOG("fps %u\n", frames);
            frames = 0;
            last = SDL_GetTicks();
        }
    }

    LOG("done\n");
    if (g_stream)
        SDL_DestroyAudioStream(g_stream);
    SDL_GL_DestroyContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();
    fflush(stderr);
    socketExit();
    return 0;
}
