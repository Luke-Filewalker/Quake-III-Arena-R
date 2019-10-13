#version 460
#extension GL_NV_ray_tracing : require
#extension GL_GOOGLE_include_directive : require
#include "RayPayload.glsl"

layout(location = 0) rayPayloadInNV RayPayload rp;

void main()
{
    rp.color = vec4(0.0, 0.0, 0.2, 0);
}