#ifdef __APPLE__

#include <SDL.h>


#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#if TARGET_OS_OSX
#import <AppKit/AppKit.h>
#else
#import <UIKit/UIKit.h>
#endif

#include "cocoaHelpers.hpp"

const char* getResourcesPath(void)
{
    static char pathStorage[1024] = {0};
    
    if (!strlen(pathStorage)) {
        @autoreleasepool {
            NSFileManager *fileManager = [[NSFileManager alloc] init];
            NSArray *urls = [fileManager URLsForDirectory:NSDocumentDirectory inDomains:NSUserDomainMask];
            strncpy(pathStorage, [urls[0] fileSystemRepresentation], sizeof(pathStorage));
        }
    }
    
    return pathStorage;
}

const char* getAppResourcesPath(void)
{
    static char pathStorage[1024] = {0};
    
    if (!strlen(pathStorage)) {
        @autoreleasepool {
            NSString *resourceDirectory = [[NSBundle mainBundle] resourcePath];
            
            char* str = (char*)[resourceDirectory UTF8String];
            strncpy(pathStorage, str, sizeof(pathStorage));
            
        }
    }
    
    return pathStorage;
}


bool getDLCEnabled() {
    return [[NSUserDefaults standardUserDefaults] boolForKey:@"PLUS_DLC"] == YES;
}

void showMissingDataFileAlert() {
    const SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "OK" },
    };
    
    const SDL_MessageBoxData messageboxdata = {
        SDL_MESSAGEBOX_INFORMATION,
        NULL,
        "Missing Game!",
        "The engine could not find a game configuration. Please copy a Data.rsdk file into the app through Files on your device, or Finder/iTunes on a PC or Mac. The engine will now halt.",
        SDL_arraysize(buttons),
        buttons,
        nullptr
    };
    
    int buttonid;
    if (SDL_ShowMessageBox(&messageboxdata, &buttonid) < 0) {
        SDL_Log("error displaying message box??");
    }
}


int getDisplayRefresh() {
#if !TARGET_OS_MACCATALYST && !TARGET_OS_OSX
    UIScreen* screen = [UIScreen mainScreen];
    if([screen respondsToSelector:@selector(maximumFramesPerSecond)])
        return (int)[screen maximumFramesPerSecond];
#endif
    return 60;
}
// Native iOS CRT Metal bridge.
extern "C" bool iosCRTMetalAvailable(void *metalEncoder)
{
#if TARGET_OS_IPHONE
    if (!metalEncoder)
        return false;

    id<MTLRenderCommandEncoder> encoder =
        (__bridge id<MTLRenderCommandEncoder>)metalEncoder;

    return [encoder respondsToSelector:
        @selector(setRenderPipelineState:)];
#else
    return false;
#endif
}
// Load the original CRT-Sharp Metal shader library.
extern "C" bool iosCRTMetalShaderLoaded(void)
{
#if TARGET_OS_IPHONE
    @autoreleasepool {
        id<MTLDevice> device = MTLCreateSystemDefaultDevice();
        if (!device)
            return false;

        NSString *path = [[[NSBundle mainBundle] resourcePath]
            stringByAppendingPathComponent:@"Shaders/CRT-Yeetron.metallib"];

        NSError *error = nil;
        id<MTLLibrary> library =
            [device newLibraryWithFile:path error:&error];

        if (!library)
            return false;

        id<MTLFunction> vertex =
            [library newFunctionWithName:@"crtVertex"];
        id<MTLFunction> fragment =
            [library newFunctionWithName:@"crtYeetron"];

        if (!vertex || !fragment)
    return false;

// Test creation of the original CRT-Sharp render pipeline.
MTLRenderPipelineDescriptor *descriptor =
    [[MTLRenderPipelineDescriptor alloc] init];

descriptor.vertexFunction = vertex;
descriptor.fragmentFunction = fragment;
descriptor.colorAttachments[0].pixelFormat =
    MTLPixelFormatBGRA8Unorm;

NSError *pipelineError = nil;
id<MTLRenderPipelineState> pipeline =
    [device newRenderPipelineStateWithDescriptor:descriptor
                                          error:&pipelineError];

if (!pipeline) {
    NSLog(@"iOS CRT: Pipeline creation failed: %@",
          pipelineError);
    return false;
}

NSLog(@"iOS CRT: CRT-Sharp pipeline created successfully");
return true;
    }
#else
    return false;
#endif
}
#endif 
