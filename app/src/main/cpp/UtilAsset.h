#pragma once
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <vector>
#include <android/asset_manager.h>

namespace hiveVG
{
    bool readAssetFile(AAssetManager* vAssetManager, const char* vFileName, std::vector<uint8_t>& voBuffer);
    uint8_t* readImageFromAsset(AAssetManager* vAssetManager, const char* vFileName, int32_t* voImgWidth, int32_t* voImgHeight);
    void freeImageFromAsset(uint8_t* vBuffer);
}