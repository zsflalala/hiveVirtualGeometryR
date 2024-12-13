#pragma once

#include <string>
#include <vector>
#include <EGL/egl.h>
#include <GLES3/gl3.h>

struct android_app;

class CTextureAsset;
namespace hiveVG
{
    class CSequenceFrameRenderer
    {
    public:
        CSequenceFrameRenderer(android_app *vApp);
        virtual ~CSequenceFrameRenderer();

        void render();
        void renderBlendingSnow(const int vRow, const int vColumn);

    private:
        void            __initRenderer();
        void            __initAlgorithm();
        GLuint          __loadTexture(const std::string& vTexturePath);
        static GLuint   __compileShader(GLenum vType, const char *vShaderCode);
        static GLuint   __linkProgram(GLuint vVertShaderHandle, GLuint vFragShaderHandle);
        void            __createScreenVAO();
        GLuint          __createProgram(const char* vVertexShaderCode, const char* vFragmentShaderCode);
        static double   __getCurrentTime();
        static bool     __checkGLError();

        android_app*                    m_pApp{};
        GLuint                          m_ProgramHandle     = 0;
        GLuint                          m_QuadVAOHandle     = 0;
        EGLDisplay                      m_Display           = EGL_NO_DISPLAY;
        EGLSurface                      m_Surface           = EGL_NO_SURFACE;
        EGLContext                      m_Context           = EGL_NO_CONTEXT;
        std::vector<GLuint>             m_initResources;
        double                          m_NearLastFrameTime = 0.0;
        double                          m_FarLastFrameTime  = 0.0;
        int                             m_NearCurrentFrame  = 0;
        int                             m_FarCurrentFrame   = 1;
        const int                       m_FramePerSecond    = 48;
        const int                       m_FarFramePerSecond = 48;
        bool                            m_IsFinished        = false;

        std::vector<std::shared_ptr<CTextureAsset> > m_pTextureHandles;
    };

} // hiveVG

