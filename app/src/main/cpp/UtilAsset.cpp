#include "UtilAsset.h"
#include <string>
#include <vector>
#include <android/asset_manager.h>
#include "stb_image.h"

namespace hiveVG
{
    bool readAssetFile(AAssetManager* vAssetManager, const char* vFileName, std::vector<uint8_t>& voBuffer)
    {
        AAsset* Descriptor = AAssetManager_open(vAssetManager,vFileName,AASSET_MODE_BUFFER);
        if(Descriptor == nullptr) return false;
        size_t FileLength = AAsset_getLength(Descriptor);
        voBuffer.resize(FileLength);
        int64_t readSize = AAsset_read(Descriptor,voBuffer.data(),voBuffer.size());
        AAsset_close(Descriptor);
        return (readSize == voBuffer.size());
    }
    uint8_t* readImageFromAsset(AAssetManager* vAssetManager, const char* vFileName, int32_t* voImgWidth, int32_t* voImgHeight)
    {
        std::vector<uint8_t> ReadinBuffer;
        bool ReadinResult = readAssetFile(vAssetManager, vFileName, ReadinBuffer);
        if(!ReadinResult) return nullptr;
        int PicChannelCount = 0;
        uint8_t* ImgBuffer = stbi_load_from_memory(ReadinBuffer.data(),ReadinBuffer.size(),voImgWidth,voImgHeight,&PicChannelCount,4);
        return ImgBuffer;

    }
    void freeImageFromAsset(uint8_t* vBuffer) { stbi_image_free(vBuffer);}
}