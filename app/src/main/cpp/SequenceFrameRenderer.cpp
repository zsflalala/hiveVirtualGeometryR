#include "SequenceFrameRenderer.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <cassert>
#include <android/imagedecoder.h>
#include <android/asset_manager.h>
#include "Common.h"
#include "TextureAsset.h"
#include "ShaderSource.h"
#include "stb_image.h"

namespace hiveVG
{
#define HIVE_LOGTAG hiveVG::TAG_KEYWORD::SeqFrame_RENDERER_TAG
    CSequenceFrameRenderer::CSequenceFrameRenderer(android_app *vApp) : m_pApp(vApp)
    {
        m_initResources.clear();
        __initRenderer();
        __initAlgorithm();
        __createScreenVAO();
        m_NearLastFrameTime = __getCurrentTime();
        m_FarLastFrameTime  = __getCurrentTime();
    }

    CSequenceFrameRenderer::~CSequenceFrameRenderer()
    {
        if (m_Display != EGL_NO_DISPLAY)
        {
            eglMakeCurrent(m_Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (m_Context != EGL_NO_CONTEXT)
            {
                eglDestroyContext(m_Display, m_Context);
                m_Context = EGL_NO_CONTEXT;
            }
            if (m_Surface != EGL_NO_SURFACE)
            {
                eglDestroySurface(m_Display, m_Surface);
                m_Surface = EGL_NO_SURFACE;
            }
            eglTerminate(m_Display);
            m_Display = EGL_NO_DISPLAY;
        }
        glDeleteVertexArrays(1, &m_QuadVAOHandle);
        glDeleteProgram(m_ProgramHandle);
    }

    void CSequenceFrameRenderer::__initRenderer()
    {
        constexpr EGLint Attributes[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_BLUE_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_RED_SIZE, 8,
                EGL_DEPTH_SIZE, 24,
                EGL_NONE
        };

        // The default Display is probably what you want on Android
        auto Display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        eglInitialize(Display, nullptr, nullptr);

        // figure out how many configs there are
        EGLint NumConfigs;
        eglChooseConfig(Display, Attributes, nullptr, 0, &NumConfigs);

        // get the list of configurations
        std::unique_ptr<EGLConfig[]> pSupportedConfigs(new EGLConfig[NumConfigs]);
        eglChooseConfig(Display, Attributes, pSupportedConfigs.get(), NumConfigs, &NumConfigs);

        // Find a pConfig we like.
        // Could likely just grab the first if we don't care about anything else in the pConfig.
        // Otherwise hook in your own heuristic
        auto pConfig = *std::find_if(
                pSupportedConfigs.get(),
                pSupportedConfigs.get() + NumConfigs,
                [&Display](const EGLConfig &Config)
                {
                    EGLint Red, Green, Blue, Depth;
                    if (eglGetConfigAttrib(Display, Config, EGL_RED_SIZE, &Red)
                        && eglGetConfigAttrib(Display, Config, EGL_GREEN_SIZE, &Green)
                        && eglGetConfigAttrib(Display, Config, EGL_BLUE_SIZE, &Blue)
                        && eglGetConfigAttrib(Display, Config, EGL_DEPTH_SIZE, &Depth))
                    {

                        LOG_INFO(hiveVG::TAG_KEYWORD::RENDERER_TAG, "Found pConfig with Red: %d, Green: %d, Blue: %d, Depth: %d", Red, Green, Blue, Depth);
                        return Red == 8 && Green == 8 && Blue == 8 && Depth == 24;
                    }
                    return false;
                });

        LOG_INFO(hiveVG::TAG_KEYWORD::RENDERER_TAG, "Found %d configs", NumConfigs);

        // create the proper window Surface
        EGLint Format;
        eglGetConfigAttrib(Display, pConfig, EGL_NATIVE_VISUAL_ID, &Format);
        EGLSurface Surface = eglCreateWindowSurface(Display, pConfig, m_pApp->window, nullptr);

        // Create a GLES 3 Context
        EGLint ContextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        EGLContext Context = eglCreateContext(Display, pConfig, nullptr, ContextAttribs);

        // get some window metrics
        auto MadeCurrent = eglMakeCurrent(Display, Surface, Surface, Context);
        assert(MadeCurrent);

        m_Display = Display;
        m_Surface = Surface;
        m_Context = Context;
    }

    void CSequenceFrameRenderer::__initAlgorithm()
    {
        GLuint NearSnowTextureHandle    = __loadTexture("Textures/nearSnow.png");
        GLuint FarSnowTextureHandle     = __loadTexture("Textures/farSnow.png");
        GLuint CartoonTextureHandle     = __loadTexture("Textures/houseWithSnow.png");
        GLuint BackgroundTextureHandle  = __loadTexture("Textures/background.jpg");

        GLuint NearSnowShaderProgram    = __createProgram(SnowVertexShaderSource, SnowFragmentShaderSource);
        GLuint FarSnowShaderProgram     = __createProgram(SnowVertexShaderSource, SnowFragmentShaderSource);
        GLuint CartoonShaderProgram     = __createProgram(QuadVertexShaderSource, QuadFragmentShaderSource);
        GLuint BackgroundShaderProgram  = __createProgram(QuadVertexShaderSource, QuadFragmentShaderSource);

        m_initResources.push_back(NearSnowTextureHandle);
        m_initResources.push_back(FarSnowTextureHandle);
        m_initResources.push_back(CartoonTextureHandle);
        m_initResources.push_back(BackgroundTextureHandle);
        m_initResources.push_back(NearSnowShaderProgram);
        m_initResources.push_back(FarSnowShaderProgram);
        m_initResources.push_back(CartoonShaderProgram);
        m_initResources.push_back(BackgroundShaderProgram);
    }

    GLuint CSequenceFrameRenderer::__compileShader(GLenum vType, const char *vShaderCode)
    {
        GLuint ShaderHandle = glCreateShader(vType);
        if (ShaderHandle != 0)
        {
            glShaderSource(ShaderHandle, 1, &vShaderCode, nullptr);
            glCompileShader(ShaderHandle);
            GLint CompileStatus = GL_FALSE;
            glGetShaderiv(ShaderHandle, GL_COMPILE_STATUS, &CompileStatus);
            if (CompileStatus == GL_FALSE)
            {
                glDeleteShader(ShaderHandle);
                return 0;
            }
            return ShaderHandle;
        }
        return 0;
    }

    GLuint CSequenceFrameRenderer::__linkProgram(GLuint vVertShaderHandle, GLuint vFragShaderHandle)
    {
        assert(vVertShaderHandle != 0 && vFragShaderHandle != 0);
        GLuint ProgramHandle = glCreateProgram();
        if (ProgramHandle != 0)
        {
            glAttachShader(ProgramHandle, vVertShaderHandle);
            glAttachShader(ProgramHandle, vFragShaderHandle);
            glLinkProgram(ProgramHandle);
            GLint LinkStatus = GL_FALSE;
            glGetProgramiv(ProgramHandle, GL_LINK_STATUS, &LinkStatus);
            if (LinkStatus == GL_FALSE)
            {
                LOG_ERROR(HIVE_LOGTAG, "Link Shader Program Failed");
                glDeleteProgram(ProgramHandle);
                return 0;
            }
            return ProgramHandle;
        }
        LOG_ERROR(HIVE_LOGTAG, "Link Shader Program Failed");
        glDeleteShader(vVertShaderHandle);
        glDeleteShader(vFragShaderHandle);
        return 0;
    }

    GLuint CSequenceFrameRenderer::__createProgram(const char* vVertexShaderCode, const char* vFragmentShaderCode)
    {
        GLuint VertShaderHandle = __compileShader(GL_VERTEX_SHADER, vVertexShaderCode);
        if (VertShaderHandle == 0)
        {
            LOG_ERROR(HIVE_LOGTAG, "Compile VertexShader Failed");
            return 0;
        }
        GLuint FragShaderHandle = __compileShader(GL_FRAGMENT_SHADER, vFragmentShaderCode);
        if (FragShaderHandle == 0)
        {
            glDeleteShader(VertShaderHandle);
            LOG_ERROR(HIVE_LOGTAG, "Compile FragmentShader Failed");
            return 0;
        }
        m_ProgramHandle = __linkProgram(VertShaderHandle, FragShaderHandle);
        LOG_INFO(HIVE_LOGTAG, "Create Shader Program Succeeded.");
        return m_ProgramHandle;
    }

    GLuint CSequenceFrameRenderer::__loadTexture(const std::string& vTexturePath)
    {
        auto TextureHandle = CTextureAsset::loadAsset(m_pApp->activity->assetManager, vTexturePath);
        if (TextureHandle == nullptr)
        {
            LOG_ERROR(HIVE_LOGTAG, "Failed to load texture");
            return 0;
        }
        LOG_INFO(HIVE_LOGTAG, "Load Texture Successfully into TextureID %d", TextureHandle->getTextureID());
        m_pTextureHandles.push_back(TextureHandle);
        return TextureHandle->getTextureID();
    }

    void CSequenceFrameRenderer::__createScreenVAO()
    {
        const float Vertices[] = {
                // positions              // texCoords
                -1.0f, 1.0f, 0.0f, 0.0f, // bottom left
                 1.0f, 1.0f, 1.0f, 0.0f, // bottom right
                 1.0f, -1.0f, 1.0f, 1.0f, // top right
                  -1.0f, -1.0f, 0.0f, 1.0f  // top left
        };

        const u_int Indices[] = {
                0, 1, 2,
                0, 2, 3
        };

        glGenVertexArrays(1, &m_QuadVAOHandle);
        glBindVertexArray(m_QuadVAOHandle);

        GLuint QuadVBO, QuadEBO;
        glGenBuffers(1, &QuadVBO);
        glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_DYNAMIC_DRAW);

        glGenBuffers(1, &QuadEBO);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(Indices), Indices, GL_STATIC_DRAW);

        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    void CSequenceFrameRenderer::render()
    {
        glUseProgram(m_ProgramHandle);

        glClearColor(0.0f,1.0f,1.0f,1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_pTextureHandles[0]->getTextureID());
        __checkGLError();
        glBindVertexArray(m_QuadVAOHandle);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        auto SwapResult = eglSwapBuffers(m_Display, m_Surface);
        assert(SwapResult == EGL_TRUE);
    }

    void CSequenceFrameRenderer::renderBlendingSnow(const int vRow, const int vColumn)
    {
        double CurrentTime = __getCurrentTime();
        double DeltaTime = CurrentTime - m_NearLastFrameTime;
        if(DeltaTime >= 1.0 / m_FramePerSecond)
        {
            m_NearLastFrameTime = CurrentTime;
            m_NearCurrentFrame = (m_NearCurrentFrame + 1) % (vRow * vColumn);
        }

        glClearColor(0.2f,0.3f,0.2f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        //background
        glUseProgram(m_initResources[7]);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_initResources[3]);
        glBindVertexArray(m_QuadVAOHandle);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //farsnow
        int  Row = m_NearCurrentFrame / vColumn;
        int  Col = m_NearCurrentFrame % vColumn;
        float U0 = Col / (float)vColumn;
        float V0 = Row / (float)vRow;
        float U1 = (Col + 1) / (float)vColumn;
        float V1 = (Row + 1) / (float)vRow;
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m_initResources[5]);
        glUniform2f(glGetUniformLocation(m_initResources[5], "uvOffset"), U0, V0);
        glUniform2f(glGetUniformLocation(m_initResources[5], "uvScale"), U1 - U0, V1 - V0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_initResources[1]);
        glBindVertexArray(m_QuadVAOHandle);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //cartoon
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m_initResources[6]);
        glUniform1i(glGetUniformLocation(m_initResources[6], "quadTexture"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_initResources[2]);
        glBindVertexArray(m_QuadVAOHandle);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        //nearSnow
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(m_initResources[4]);
        glUniform2f(glGetUniformLocation(m_initResources[4], "uvOffset"), U0, V0);
        glUniform2f(glGetUniformLocation(m_initResources[4], "uvScale"), U1 - U0, V1 - V0);
//        glUniform1i(glGetUniformLocation(m_initResources[4], "snowTexture"), 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_initResources[0]);
        glBindVertexArray(m_QuadVAOHandle);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        auto SwapResult = eglSwapBuffers(m_Display, m_Surface);
        assert(SwapResult == EGL_TRUE);
    }

    double CSequenceFrameRenderer::__getCurrentTime()
    {
        struct timeval tv;
        gettimeofday(&tv, nullptr);
        return tv.tv_sec + tv.tv_usec / 1000000.0;
    }

    bool CSequenceFrameRenderer::__checkGLError()
    {
        if(eglGetError() != EGL_SUCCESS)
        {
            LOG_ERROR(hiveVG::TAG_KEYWORD::RENDERER_TAG, "GL Error Code %d.", eglGetError());
            return false;
        }
        return true;
    }
}