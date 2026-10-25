
#include <metal_stdlib>
using namespace metal;

struct CRTVertex {
    float4 position [[position]];
    float2 uv;
};

struct CRTUniforms {
    float2 pixelSize;
    float2 textureSize;
    float2 viewSize;
    float screenDim;
};

vertex CRTVertex crtVertex(uint id [[vertex_id]]) {
    float2 positions[3] = {
        float2(-1.0, -1.0),
        float2( 3.0, -1.0),
        float2(-1.0,  3.0)
    };

    CRTVertex output;
    output.position = float4(positions[id], 0.0, 1.0);
    output.uv = float2(
        (positions[id].x + 1.0) * 0.5,
        1.0 - (positions[id].y + 1.0) * 0.5
    );
    return output;
}

fragment float4 crtYeetron(
    CRTVertex input [[stage_in]],
    texture2d<float> source [[texture(0)]],
    constant CRTUniforms &u [[buffer(0)]]
) {
    constexpr sampler sampleMode(
        address::clamp_to_edge,
        filter::linear
    );

    float2 uv = input.uv;
    float2 viewPos =
        floor((u.textureSize / u.pixelSize) *
              uv * u.viewSize) + 0.5;

    float intensityPos =
        fract((viewPos.y * 3.0 + viewPos.x) *
              0.166667);

    float3 intensity = float3(1.1, 0.9, 0.9);

    if (intensityPos < 0.333) {
        intensity = intensity.xyz;
    } else if (intensityPos < 0.666) {
        intensity = intensity.zxy;
    } else {
        intensity = intensity.yzx;
    }

    float2 pixelPos = uv * u.textureSize;
    float2 roundedPos = floor(pixelPos);

    float scanline = clamp(
        abs(sin(pixelPos.y * 3.14159)) + 0.25,
        0.5, 1.0
    );

    pixelPos = fract(pixelPos) - 0.5;

    float2 inversePos =
        -uv * u.textureSize +
        roundedPos + 0.5;

    float xWeight = clamp(
        -abs(inversePos.x * 0.5) + 1.5,
        0.8, 1.25
    );

    float yWeight = clamp(
        -abs(inversePos.y * 2.0) + 1.25,
        0.5, 1.0
    );

    float2 colorWeight = float2(
        xWeight * yWeight,
        xWeight * ((scanline + yWeight) * 0.5)
    );

    scanline *= xWeight;

    float2 sampleUV =
        ((pixelPos - clamp(pixelPos, -0.25, 0.25))
         * 2.0 + roundedPos + 0.5)
        / u.textureSize;

    float4 color = source.sample(sampleMode, sampleUV);

    float3 blended = float3(
        scanline * color.r,
        colorWeight.x * color.g,
        colorWeight.y * color.b
    );

    float3 result =
        u.viewSize.y >= 720.0
        ? intensity * blended
        : blended;

    return float4(result * u.screenDim, color.a);
}
