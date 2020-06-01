#pragma once

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#pragma pack(push, 1)
struct ui_vertex
{
    v2 P;
    v2 UV;
};
#pragma pack(pop)

struct ui_system
{
    texture FontAtlas;
    int AtlasWidth;
    int AtlasHeight;
    int BeginCharI;
    int EndCharI;
    int CharXCount;
    int CharYCount;
    int FontWidth;
    int FontHeight;
    
    pso RasterizeTextPSO;
    
    ID3D12Resource *TextVB;
    int TextVertCount;
    
    void SetErrorMessage(gpu_context *Context, char *String);
    void DrawMessage(gpu_context *Context, texture RenderTarget);
};