#include "Renderer.h"
#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <GLES3/gl3.h>
#include <memory>
#include <vector>
#include <cassert>
#include <android/imagedecoder.h>
#include "Common.h"

hiveVG::CRenderer::~CRenderer()
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
    glDeleteVertexArrays(1, &m_TriangleVAOHandle);
    glDeleteProgram(m_ProgramHandle);
}

void hiveVG::CRenderer::render()
{
    glClearColor(0.1f,0.2f,0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(m_ProgramHandle);
    glBindVertexArray(m_TriangleVAOHandle);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    // Present the rendered image. This is an implicit glFlush.
    auto SwapResult = eglSwapBuffers(m_Display, m_Surface);
    assert(SwapResult == EGL_TRUE);
}

void hiveVG::CRenderer::__initRenderer()
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
            [&Display](const EGLConfig &Config) {
                EGLint Red, Green, Blue, Depth;
                if (eglGetConfigAttrib(Display, Config, EGL_RED_SIZE, &Red)
                    && eglGetConfigAttrib(Display, Config, EGL_GREEN_SIZE, &Green)
                    && eglGetConfigAttrib(Display, Config, EGL_BLUE_SIZE, &Blue)
                    && eglGetConfigAttrib(Display, Config, EGL_DEPTH_SIZE, &Depth)) {

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

hiveVG::CRenderer::CRenderer(android_app *vApp) : m_pApp(vApp)
{
    __initRenderer();
    __createProgram();
    __createTriangleVAO();
}

GLuint hiveVG::CRenderer::__compileShader(GLenum vType, const char *vShaderCode)
{
    GLuint ShaderHandle = glCreateShader(vType);
    if (ShaderHandle != 0)
    {
        glShaderSource(ShaderHandle, 1, &vShaderCode, nullptr);
        glCompileShader(ShaderHandle);
        GLint Result = GL_FALSE;
        glGetShaderiv(ShaderHandle, GL_COMPILE_STATUS, &Result);
        if (Result == GL_FALSE)
        {
            glDeleteShader(ShaderHandle);
            return 0;
        }
        return ShaderHandle;
    }
    return 0;
}

GLuint hiveVG::CRenderer::__linkProgram(GLuint vVertShaderHandle, GLuint vFragShaderHandle)
{
    GLuint ProgramHandle = glCreateProgram();
    if (ProgramHandle != 0)
    {
        glAttachShader(ProgramHandle, vVertShaderHandle);
        glAttachShader(ProgramHandle, vFragShaderHandle);
        glLinkProgram(ProgramHandle);
        GLint Result = GL_FALSE;
        glGetProgramiv(ProgramHandle, GL_LINK_STATUS, &Result);
        if (Result == GL_FALSE)
        {
            glDeleteProgram(ProgramHandle);
            return 0;
        }
        return ProgramHandle;
    }
    return 0;
}

void hiveVG::CRenderer::__createTriangleVAO()
{
    const float Vertices[] = {
            0.0f, 0.5f, 0.0f, 1.0f, 0.0f,
            -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.0f, 0.0f, 1.0f
    };
    glGenVertexArrays(1, &m_TriangleVAOHandle);
    glBindVertexArray(m_TriangleVAOHandle);

    GLuint VertexBufferHandle;
    glGenBuffers(1, &VertexBufferHandle);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBufferHandle);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertices), Vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
}

void hiveVG::CRenderer::__createProgram()
{
    const char VertShaderCode[] =
            "#version 300 es \n"
            "layout (location = 0) in vec2 vPosition;\n"
            "layout (location = 1) in vec3 vColor;\n"
            "out vec3 OurColor;\n"
            "void main() { \n"
            "gl_Position  = vec4(vPosition, 0.0, 1.0);\n"
            "OurColor = vColor;\n"
            "}\n";
    const char FragShaderCode[] =
            "#version 300 es \n"
            "precision mediump float;\n"
            "in vec3 OurColor;\n"
            "out vec4 FragColor;\n"
            "void main() { \n"
            "FragColor = vec4(OurColor,1.0f);\n"
            "}\n";

    GLuint VertShaderHandle = __compileShader(GL_VERTEX_SHADER, VertShaderCode);
    if (VertShaderHandle == 0)
    {
        LOG_ERROR(hiveVG::TAG_KEYWORD::RENDERER_TAG, "Compile VertexShader Failed");
        return;
    }
    GLuint FragShaderHandle = __compileShader(GL_FRAGMENT_SHADER, FragShaderCode);
    if (FragShaderHandle == 0)
    {
        LOG_ERROR(hiveVG::TAG_KEYWORD::RENDERER_TAG, "Compile FragmentShader Failed");
        return;
    }
    m_ProgramHandle = __linkProgram(VertShaderHandle, FragShaderHandle);
}

