#pragma once

#include <EGL/egl.h>
#include <GLES3/gl3.h>

struct android_app;

namespace hiveVG
{
    class CRenderer
    {
    public:
        CRenderer(android_app *vApp);
        virtual ~CRenderer();

        void render();

    private:
        void          __initRenderer();
        static GLuint __compileShader(GLenum vType, const char *vShaderCode);
        static GLuint __linkProgram(GLuint vVertShaderHandle, GLuint vFragShaderHandle);
        void          __createTriangleVAO();
        void          __createProgram();

        android_app* m_pApp;
        GLuint       m_ProgramHandle     = 0;
        GLuint       m_TriangleVAOHandle = 0;
        EGLDisplay   m_Display           = EGL_NO_DISPLAY;
        EGLSurface   m_Surface           = EGL_NO_SURFACE;
        EGLContext   m_Context           = EGL_NO_CONTEXT;
    };
}
