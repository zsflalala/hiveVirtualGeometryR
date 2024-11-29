#pragma once
#include <memory>
#include <string>
#include <vector>
#include <android/asset_manager.h>
#include <GLES3/gl3.h>

class CTextureAsset {
public:
    /*!
     * Loads a texture asset from the assets/ directory
     * @param vAssetManager Asset manager to use
     * @param vAssetPath The path to the asset
     * @return a shared pointer to a texture asset, resources will be reclaimed when it's cleaned up
     */
    static std::shared_ptr<CTextureAsset> loadAsset(AAssetManager *vAssetManager, const std::string &vAssetPath);
    ~CTextureAsset();
    [[nodiscard]] constexpr GLuint getTextureID() const { return m_textureID; }

private:
    inline explicit CTextureAsset(GLuint vTextureId);

    GLuint m_textureID;
};