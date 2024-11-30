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

namespace hiveVG
{
    CSequenceFrameRenderer::CSequenceFrameRenderer(android_app *vApp) : m_pApp(vApp)
    {
        __initRenderer();
        __createProgram();
        __createQuadVAO();
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

    void CSequenceFrameRenderer::loadTexture(const std::string& vTexturePath)
    {
        if(m_pTextureHandle != nullptr) return;
        m_pTextureHandle = CTextureAsset::loadAsset(m_pApp->activity->assetManager, vTexturePath);
        if (m_pTextureHandle == nullptr)
        {
            LOG_ERROR(hiveVG::TAG_KEYWORD::SeqFrame_RENDERER_TAG, "Failed to load texture");
            return ;
        }
        LOG_INFO(hiveVG::TAG_KEYWORD::SeqFrame_RENDERER_TAG, "Load Texture Successfully into TextureID %d", m_pTextureHandle->getTextureID());
    }

    void CSequenceFrameRenderer::render()
    {
        glUseProgram(m_ProgramHandle);

        glClearColor(0.4f,0.2f,0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        assert(m_pTextureHandle);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_pTextureHandle->getTextureID());
        if(eglGetError() != EGL_SUCCESS)
            LOG_ERROR(hiveVG::TAG_KEYWORD::RENDERER_TAG, "GL Error Code %d has description", eglGetError());

        glBindVertexArray(m_QuadVAOHandle);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        // Present the rendered image. This is an implicit glFlush.
        auto SwapResult = eglSwapBuffers(m_Display, m_Surface);
        assert(SwapResult == EGL_TRUE);
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
                glDeleteProgram(ProgramHandle);
                return 0;
            }
            return ProgramHandle;
        }
        return 0;
    }

    void CSequenceFrameRenderer::__createProgram()
    {
        const char VertShaderCode[] = R"vertex(#version 300 es
                layout (location = 0) in vec2 inPosition;
                layout (location = 1) in vec2 inUV;

                out vec2 fragUV;

                void main() {
                    fragUV = inUV;
                    gl_Position = vec4(inPosition, 0.0, 1.0);
                }
                )vertex";
        const char FragShaderCode[] = R"fragment(#version 300 es
                precision mediump float;

                in vec2 fragUV;

                uniform sampler2D uTexture;

                out vec4 outColor;

                void main() {
                    outColor = texture(uTexture, fragUV);
                }
                )fragment";

        GLuint VertShaderHandle = __compileShader(GL_VERTEX_SHADER, VertShaderCode);
        if (VertShaderHandle == 0)
        {
            LOG_ERROR(hiveVG::TAG_KEYWORD::SeqFrame_RENDERER_TAG, "Compile VertexShader Failed");
            return;
        }
        GLuint FragShaderHandle = __compileShader(GL_FRAGMENT_SHADER, FragShaderCode);
        if (FragShaderHandle == 0)
        {
            LOG_ERROR(hiveVG::TAG_KEYWORD::SeqFrame_RENDERER_TAG, "Compile FragmentShader Failed");
            return;
        }
        m_ProgramHandle = __linkProgram(VertShaderHandle, FragShaderHandle);
    }

    void CSequenceFrameRenderer::__createQuadVAO()
    {
        const float Vertices[] = {
                // positions                // texCoords
                0.5f, 0.5f,  0.0f, 0.0f, // bottom left
                -0.5f, 0.5f,  1.0f, 0.0f, // bottom right
                -0.5f,  -0.5f,  1.0f, 1.0f, // top right
                0.5f,  - 0.5f,  0.0f, 1.0f  // top left
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
}