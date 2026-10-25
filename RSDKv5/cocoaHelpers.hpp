#ifndef COCOA_HELPERS_H
#define COCOA_HELPERS_H

const char* getResourcesPath(void);
const char* getAppResourcesPath(void);

bool getDLCEnabled();

void showMissingDataFileAlert();

int getDisplayRefresh();
// iOS native Metal CRT rendering bridge.
#ifdef __cplusplus
extern "C" {
#endif

bool iosCRTMetalAvailable(void *metalEncoder);

#ifdef __cplusplus
}
#endif
#endif
