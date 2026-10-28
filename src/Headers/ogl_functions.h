#pragma once

#ifdef __EMSCRIPTEN__
	#include <GL/gl.h>
	#include <GL/glext.h>
	#undef glActiveTextureARB
	#undef glClientActiveTextureARB
#else
	#include <SDL3/SDL_opengl.h>
#endif

extern PFNGLACTIVETEXTUREARBPROC			procptr_glActiveTextureARB;
extern PFNGLCLIENTACTIVETEXTUREARBPROC		procptr_glClientActiveTextureARB;

#define glActiveTextureARB					procptr_glActiveTextureARB
#define glClientActiveTextureARB			procptr_glClientActiveTextureARB

void OGL_InitFunctions(void);
