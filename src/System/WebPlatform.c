#ifdef __EMSCRIPTEN__
#include "game.h"
#include "webplatform.h"

// Keep the original nested scene loops, but never occupy the browser event loop.
EM_ASYNC_JS(int, WebWaitFrame, (void), {
	var paused = document.hidden || Module['ottoPaused'];
	while (document.hidden || Module['ottoPaused']) {
		await new Promise(resolve => window.addEventListener('otto-resume', resolve, {once: true}));
	}
	await new Promise(resolve => requestAnimationFrame(resolve));
	return paused ? 1 : 0;
});

void WebSyncSave(void)
{
	EM_ASM({ Module['ottoSyncSave'](); });
}

void WebShowAlert(const char* message)
{
	EM_ASM({ Module['ottoStatus'](UTF8ToString($0), true); }, message);
}
#endif
