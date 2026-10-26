#import <simd/simd.h>
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
#include <dlfcn.h>
#if TARGET_OS_IPHONE
static id<MTLRenderPipelineState> crtSharpPipeline = nil;
#endif
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

extern "C" bool iosCRTMetalTextureAvailable(void *sdlTexture)
{
#if TARGET_OS_IPHONE
    if (!sdlTexture)
        return false;

    typedef void *(*MetalTextureLookup)(SDL_Texture *);

    MetalTextureLookup lookup =
        reinterpret_cast<MetalTextureLookup>(
            dlsym(RTLD_DEFAULT, "RSDK_GetMetalTexture")
        );

    
    if (!lookup) {
        SDL_Log("iOS CRT: Native texture symbol = NOT FOUND");
        return false;
    }

    SDL_Log("iOS CRT: Native texture symbol = FOUND");


    void *nativeTexture =
    lookup(static_cast<SDL_Texture *>(sdlTexture));

if (nativeTexture) {
    id<MTLTexture> texture =
        (__bridge id<MTLTexture>)nativeTexture;

    NSLog(@"iOS CRT: Source texture Metal device = %@",
          texture.device.name);
}

return nativeTexture != nullptr;
#else
    return false;
#endif
}
extern "C" int iosCRTMetalTextureStatus(void *sdlTexture)
{
#if TARGET_OS_IPHONE
    if (!sdlTexture)
        return 0;

    typedef int (*MetalTextureDiagnostic)(SDL_Texture *);

    MetalTextureDiagnostic lookup =
    reinterpret_cast<MetalTextureDiagnostic>(
        dlsym(RTLD_DEFAULT, "RSDK_GetMetalTextureDiagnostic")
    );

    if (!lookup)
        return 1;

    return lookup(static_cast<SDL_Texture *>(sdlTexture));
#else
    return 0;
#endif
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
NSLog(@"iOS CRT: Pipeline Metal device = %@",
      device.name);
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
crtSharpPipeline = pipeline;

NSLog(@"iOS CRT: CRT-Sharp pipeline created successfully");
return true;
    }
#else
    return false;
#endif
}
extern "C" bool iosCRTMetalRender(
    void *metalEncoder,
    void *sdlTexture,
    float pixelWidth,
    float pixelHeight,
    float textureWidth,
    float textureHeight,
    float viewWidth,
    float viewHeight,
    float screenDim)
{
#if TARGET_OS_IPHONE
    if (!metalEncoder || !sdlTexture || !crtSharpPipeline)
        return false;

    typedef void *(*MetalTextureLookup)(SDL_Texture *);

    MetalTextureLookup lookup =
        reinterpret_cast<MetalTextureLookup>(
            dlsym(RTLD_DEFAULT, "RSDK_GetMetalTexture")
        );

    if (!lookup)
        return false;

    void *nativeTexture =
        lookup(static_cast<SDL_Texture *>(sdlTexture));

    if (!nativeTexture)
        return false;

    id<MTLRenderCommandEncoder> encoder =
        (__bridge id<MTLRenderCommandEncoder>)metalEncoder;

    id<MTLTexture> texture =
        (__bridge id<MTLTexture>)nativeTexture;

    struct CRTUniforms {
        simd_float2 pixelSize;
        simd_float2 textureSize;
        simd_float2 viewSize;
        float screenDim;
    };

    CRTUniforms uniforms = {
        { pixelWidth, pixelHeight },
        { textureWidth, textureHeight },
        { viewWidth, viewHeight },
        screenDim
    };

    [encoder setRenderPipelineState:crtSharpPipeline];
    [encoder setFragmentTexture:texture atIndex:0];
    [encoder setFragmentBytes:&uniforms
                       length:sizeof(uniforms)
                      atIndex:0];

    [encoder drawPrimitives:MTLPrimitiveTypeTriangle
               vertexStart:0
               vertexCount:3];

    return true;
#else
    return false;
#endif
}
#endif 
