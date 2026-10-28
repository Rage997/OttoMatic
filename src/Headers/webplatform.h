#pragma once

#ifdef __EMSCRIPTEN__
#ifdef __cplusplus
extern "C++" {
#endif
#include <emscripten.h>
#include <emscripten/html5.h>
#ifdef __cplusplus
}
extern "C" {
#endif

int WebWaitFrame(void);
void WebSyncSave(void);
void WebShowAlert(const char* message);
#ifdef __cplusplus
}
#endif
#endif
