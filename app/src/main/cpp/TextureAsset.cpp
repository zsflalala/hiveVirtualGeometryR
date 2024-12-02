#include "TextureAsset.h"
#include "Common.h"
#include <cassert>
#include <iostream>
#include <android/imagedecoder.h>

std::shared_ptr<CTextureAsset>
CTextureAsset::loadAsset(AAssetManager *vAssetManager, const std::string &vAssetPath) {
    // Get the image from asset manager
    auto pPicAsset = AAssetManager_open(
            vAssetManager,
            vAssetPath.c_str(),
            AASSET_MODE_BUFFER);

    // Make a decoder to turn it into a texture
    AImageDecoder *pAndroidDecoder = nullptr;
    auto result = AImageDecoder_createFromAAsset(pPicAsset, &pAndroidDecoder);
    assert(result == ANDROID_IMAGE_DECODER_SUCCESS);

    // make sure we get 8 bits per channel out. RGBA order.
    AImageDecoder_setAndroidBitmapFormat(pAndroidDecoder, ANDROID_BITMAP_FORMAT_RGBA_8888);

    // Get the image header, to help set everything up
    const AImageDecoderHeaderInfo *pAndroidHeader = nullptr;
    pAndroidHeader = AImageDecoder_getHeaderInfo(pAndroidDecoder);

    // important metrics for sending to GL
    auto width = AImageDecoderHeaderInfo_getWidth(pAndroidHeader);
    auto height = AImageDecoderHeaderInfo_getHeight(pAndroidHeader);
    auto stride = AImageDecoder_getMinimumStride(pAndroidDecoder);
    // Get the bitmap data of the image
    auto upAndroidImageData = std::make_unique<std::vector<uint8_t>>(height * stride);
    auto decodeResult = AImageDecoder_decodeImage(
            pAndroidDecoder,
            upAndroidImageData->data(),
            stride,
            upAndroidImageData->size());
    assert(decodeResult == ANDROID_IMAGE_DECODER_SUCCESS);

    // Get an opengl texture
    GLuint TextureId;
    glGenTextures(1, &TextureId);
    glBindTexture(GL_TEXTURE_2D, TextureId);

    // Clamp to the edge, you'll get odd results alpha blending if you don't
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load the texture into VRAM
    glTexImage2D(
            GL_TEXTURE_2D, // target
            0, // mip level
            GL_RGBA, // internal format, often advisable to use BGR
            width, // width of the texture
            height, // height of the texture
            0, // border (always 0)
            GL_RGBA, // format
            GL_UNSIGNED_BYTE, // type
            upAndroidImageData->data() // Data to upload
    );
    // generate mip levels. Not really needed for 2D, but good to do
    glGenerateMipmap(GL_TEXTURE_2D);

    bool isValid = (glIsTexture(TextureId) == GL_TRUE);
    if (!isValid)
    {
        LOG_ERROR(hiveVG::TAG_KEYWORD::SeqFrame_RENDERER_TAG, "Texture type error");
        return nullptr;
    }
    // cleanup helpers
    AImageDecoder_delete(pAndroidDecoder);
    AAsset_close(pPicAsset);

    return std::shared_ptr<CTextureAsset>(new CTextureAsset(TextureId));
}

CTextureAsset::~CTextureAsset() {
    // return texture resources
    glDeleteTextures(1, &m_textureID);
    m_textureID = 0;
}

CTextureAsset::CTextureAsset(GLuint TextureId) : m_textureID(TextureId) {}
