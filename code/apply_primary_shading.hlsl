#define RS "DescriptorTable(UAV(u0)), DescriptorTable(UAV(u1)), DescriptorTable(UAV(u2)), DescriptorTable(UAV(u3)), DescriptorTable(UAV(u4)), RootConstants(num32BitConstants=4, b0)"

#include "constants.hlsl"
#include "scene.hlsl"

RWTexture2D<float4> LightTex: register(u0);
RWTexture2D<float4> PositionTex: register(u1);
RWTexture2D<float4> AlbedoTex: register(u2);
RWTexture2D<float4> EmissionTex: register(u3);
RWTexture2D<float4> RayDirTex: register(u4);

struct context
{
    float Time;
    float3 CamP;
};

ConstantBuffer<context> Context: register(b0);

[RootSignature(RS)]
[numthreads(32, 32, 1)]
void main(uint2 ThreadId: SV_DispatchThreadID)
{
    float3 P = PositionTex[ThreadId].xyz;
    float3 Rd = RayDirTex[ThreadId].xyz;
    float3 Sky = Env(Rd, Context.Time);
    
    if (length(P) < 10e30)
    {
        float3 Emission = EmissionTex[ThreadId].rgb;
        float3 Albedo = AlbedoTex[ThreadId].rgb;
        
        float3 Illumination = LightTex[ThreadId].rgb;
        float3 FirstBrdf = Albedo;
        float3 Radiance = FirstBrdf*Illumination + Emission;
        
        float Depth = length(Context.CamP - P);
        Radiance = lerp(Sky, Radiance, Fog(Depth));
        
        LightTex[ThreadId] = float4(Radiance, 1.0);
    }
    else
    {
        LightTex[ThreadId] = float4(Sky, 1.0);
    }
}