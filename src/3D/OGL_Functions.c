#include "game.h"

#ifdef __EMSCRIPTEN__
#include <emscripten/html5_webgl.h>
#include <gl4esinit.h>

static void GetMainFramebufferSize(int* width, int* height)
{
	SDL_GetWindowSizeInPixels(gSDLWindow, width, height);
}
#endif

PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB			= NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB	= NULL;

void OGL_InitFunctions(void)
{
#ifdef __EMSCRIPTEN__
	// Resolve GLES backend calls separately from gl4es's desktop entrypoints.
	set_getprocaddress(emscripten_webgl_get_proc_address);
	set_getmainfbsize(GetMainFramebufferSize);
	initialize_gl4es();
	procptr_glActiveTextureARB = (PFNGLACTIVETEXTUREARBPROC) gl4es_GetProcAddress("glActiveTextureARB");
	procptr_glClientActiveTextureARB = (PFNGLCLIENTACTIVETEXTUREARBPROC) gl4es_GetProcAddress("glClientActiveTextureARB");
#else
	procptr_glActiveTextureARB			= (PFNGLACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress("glActiveTextureARB");
	procptr_glClientActiveTextureARB	= (PFNGLCLIENTACTIVETEXTUREARBPROC) SDL_GL_GetProcAddress("glClientActiveTextureARB");
#endif

	GAME_ASSERT(procptr_glActiveTextureARB);
	GAME_ASSERT(procptr_glClientActiveTextureARB);
}
