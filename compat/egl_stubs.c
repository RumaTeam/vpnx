// EGL stubs for the Vulkan variant of the app. The static SDL3 of devkitPro has an EGL video backend (SDL_egl.c) that is
// compiled in but never used here: the window is created without SDL_WINDOW_OPENGL and bgfx talks to the NWindow through
// Vulkan. Without Mesa's EGL in the link (it cannot share a binary with nxvk, both embed Mesa's util code) these symbols would
// be undefined; every stub reports "EGL not available".
#include <stddef.h>

typedef void* EGLDisplay;
typedef void* EGLConfig;
typedef void* EGLContext;
typedef void* EGLSurface;
typedef void* EGLNativeWindowType;
typedef unsigned int EGLBoolean;
typedef int EGLint;
typedef unsigned int EGLenum;
typedef void (*EGLFuncPtr)(void);

#define STUB_FALSE ((EGLBoolean)0)
#define EGL_NOT_INITIALIZED_ERR 0x3001

EGLDisplay eglGetDisplay(void* native) { (void)native; return NULL; }
EGLDisplay eglGetPlatformDisplay(EGLenum platform, void* native, const EGLint* attribs) { (void)platform; (void)native; (void)attribs; return NULL; }
EGLBoolean eglInitialize(EGLDisplay d, EGLint* major, EGLint* minor) { (void)d; (void)major; (void)minor; return STUB_FALSE; }
EGLBoolean eglTerminate(EGLDisplay d) { (void)d; return STUB_FALSE; }
EGLBoolean eglBindAPI(EGLenum api) { (void)api; return STUB_FALSE; }
EGLBoolean eglChooseConfig(EGLDisplay d, const EGLint* attribs, EGLConfig* cfgs, EGLint size, EGLint* num) { (void)d; (void)attribs; (void)cfgs; (void)size; if (num) *num = 0; return STUB_FALSE; }
EGLBoolean eglGetConfigAttrib(EGLDisplay d, EGLConfig c, EGLint a, EGLint* v) { (void)d; (void)c; (void)a; (void)v; return STUB_FALSE; }
EGLContext eglCreateContext(EGLDisplay d, EGLConfig c, EGLContext share, const EGLint* attribs) { (void)d; (void)c; (void)share; (void)attribs; return NULL; }
EGLBoolean eglDestroyContext(EGLDisplay d, EGLContext c) { (void)d; (void)c; return STUB_FALSE; }
EGLSurface eglCreateWindowSurface(EGLDisplay d, EGLConfig c, EGLNativeWindowType w, const EGLint* attribs) { (void)d; (void)c; (void)w; (void)attribs; return NULL; }
EGLSurface eglCreatePbufferSurface(EGLDisplay d, EGLConfig c, const EGLint* attribs) { (void)d; (void)c; (void)attribs; return NULL; }
EGLBoolean eglDestroySurface(EGLDisplay d, EGLSurface s) { (void)d; (void)s; return STUB_FALSE; }
EGLBoolean eglMakeCurrent(EGLDisplay d, EGLSurface draw, EGLSurface read, EGLContext c) { (void)d; (void)draw; (void)read; (void)c; return STUB_FALSE; }
EGLBoolean eglSwapBuffers(EGLDisplay d, EGLSurface s) { (void)d; (void)s; return STUB_FALSE; }
EGLBoolean eglSwapInterval(EGLDisplay d, EGLint interval) { (void)d; (void)interval; return STUB_FALSE; }
EGLBoolean eglWaitGL(void) { return STUB_FALSE; }
EGLBoolean eglWaitNative(EGLint engine) { (void)engine; return STUB_FALSE; }
EGLint eglGetError(void) { return EGL_NOT_INITIALIZED_ERR; }
const char* eglQueryString(EGLDisplay d, EGLint name) { (void)d; (void)name; return ""; }
EGLFuncPtr eglGetProcAddress(const char* name) { (void)name; return NULL; }
