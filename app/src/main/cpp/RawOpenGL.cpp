#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

const char* pSnowVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main() 
    {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* pSnowFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;
    uniform vec2 uvOffset;
    uniform vec2 uvScale; 
    uniform sampler2D snowTexture;

    void main()
    {
        vec2 TexCoords = (TexCoord * uvScale) + uvOffset; // ��̬����UV
        vec4 SnowColor = texture(snowTexture, TexCoords);
        FragColor = SnowColor;
    }
)";

const char* pQuadVertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;

    out vec2 TexCoord;

    void main() 
    {
        gl_Position = vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

const char* pQuadFragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;
    uniform sampler2D quadTexture;

    void main()
    {
        vec4 CartoonColor = texture(quadTexture, TexCoord);
        FragColor = CartoonColor;
    }
)";

GLuint compileShader(GLenum vType, const char* vSource) 
{
    GLuint Shader = glCreateShader(vType);
    glShaderSource(Shader, 1, &vSource, NULL);
    glCompileShader(Shader);

    int Success;
    char pInfoLog[512];
    glGetShaderiv(Shader, GL_COMPILE_STATUS, &Success);
    if (!Success) 
    {
        glGetShaderInfoLog(Shader, 512, NULL, pInfoLog);
        std::cerr << "Error: Shader compilation failed\n" << pInfoLog << std::endl;
    }
    return Shader;
}

GLuint loadTexture(const char* vPath)
{
    GLuint TextureID = 0;
    int Width, Height, Channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* pData = stbi_load(vPath, &Width, &Height, &Channels, 0);
    std::cout << vPath << " width: " << Width << " height: " << Height << " Channels: " << Channels << std::endl;
    if (pData)
    {
        GLenum Format = GL_RGB;
        if (Channels == 1)
            Format = GL_RED;
        else if (Channels == 3)
            Format = GL_RGB;
        else if (Channels == 4)
            Format = GL_RGBA;

        glGenTextures(1, &TextureID);
        glBindTexture(GL_TEXTURE_2D, TextureID);
        glTexImage2D(GL_TEXTURE_2D, 0, Format, Width, Height, 0, Format, GL_UNSIGNED_BYTE, pData);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << vPath << std::endl;
    }
    stbi_image_free(pData);
    return TextureID;
}

GLuint linkProgram(GLuint vVertShaderHandle, GLuint vFragShaderHandle)
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
    glDeleteShader(vVertShaderHandle);
    glDeleteShader(vFragShaderHandle);
    return 0;
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    const int WindowWidth = 1600, WindowHeight = 900;
    GLFWwindow* pWindow = glfwCreateWindow(WindowWidth, WindowHeight, "Sequence Frames", NULL, NULL);
    if (!pWindow)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(pWindow);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    GLuint NearSnowTextureHandle   = loadTexture("./Textures/nearSnow.png");
    GLuint FarSnowTextureHandle    = loadTexture("./Textures/farSnow.png");
    GLuint CartoonTextureHandle    = loadTexture("./Textures/house2.png");
    GLuint BackgroundTextureHandle = loadTexture("./Textures/background4.jpg");

    GLuint NearSnowVertexShader   = compileShader(GL_VERTEX_SHADER, pSnowVertexShaderSource);
    GLuint NearSnowFragmentShader = compileShader(GL_FRAGMENT_SHADER, pSnowFragmentShaderSource);
    GLuint NearSnowShaderProgram  = linkProgram(NearSnowVertexShader, NearSnowFragmentShader);

    GLuint CartoonVertexShader   = compileShader(GL_VERTEX_SHADER, pQuadVertexShaderSource);
    GLuint CartoonFragmentShader = compileShader(GL_FRAGMENT_SHADER, pQuadFragmentShaderSource);
    GLuint CartoonShaderProgram  = linkProgram(CartoonVertexShader, CartoonFragmentShader);

    GLuint BackgroundVertexShader   = compileShader(GL_VERTEX_SHADER, pQuadVertexShaderSource);
    GLuint BackgroundFragmentShader = compileShader(GL_FRAGMENT_SHADER, pQuadFragmentShaderSource);
    GLuint BackgroundShaderProgram  = linkProgram(BackgroundVertexShader, BackgroundFragmentShader);

    GLuint FarSnowVertexShader   = compileShader(GL_VERTEX_SHADER, pSnowVertexShaderSource);
    GLuint FarSnowFragmentShader = compileShader(GL_FRAGMENT_SHADER, pSnowFragmentShaderSource);
    GLuint FarSnowShaderProgram  = linkProgram(FarSnowVertexShader, FarSnowFragmentShader);

    const float pVertices[] = {
        // positions   // texCoords
        -1.0f, -1.0f,  0.0f, 0.0f, // bottom left
         1.0f, -1.0f,  1.0f, 0.0f, // bottom right
         1.0f,  1.0f,  1.0f, 1.0f, // top right
        -1.0f,  1.0f,  0.0f, 1.0f  // top left
    };

    const unsigned int pIndices[] = {
        0, 1, 2,
        0, 2, 3
    };

    GLuint QuadVAO, QuadVBO, QuadEBO;
    glGenVertexArrays(1, &QuadVAO);
    glBindVertexArray(QuadVAO);
    glGenBuffers(1, &QuadVBO);
    glGenBuffers(1, &QuadEBO);
    glBindBuffer(GL_ARRAY_BUFFER, QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(pVertices), pVertices, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, QuadEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(pIndices), pIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    int ROWS = 8, COLS = 16;
    int CurrentFrame = 0;
    double LastTime = glfwGetTime();
    const double FrameTime = 1.0 / 24.0; // 24 FPS

    while (!glfwWindowShouldClose(pWindow))
    {
        double CurrentTime = glfwGetTime();
        double DeltaTime = CurrentTime - LastTime;
        if(DeltaTime >= FrameTime)
        {
            LastTime = CurrentTime;
            CurrentFrame = (CurrentFrame + 1) % (ROWS * COLS);
        }

        glClearColor(0.3f, 0.2f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(BackgroundShaderProgram);
        glUniform1i(glGetUniformLocation(BackgroundShaderProgram, "quadTexture"), 0);
        glBindVertexArray(QuadVAO);
        glBindTexture(GL_TEXTURE_2D, BackgroundTextureHandle);
        glActiveTexture(GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        int  Row = CurrentFrame / COLS;
        int  Col = CurrentFrame % COLS;
        float U0 = Col / (float)COLS;
        float V0 = Row / (float)ROWS;
        float U1 = (Col + 1) / (float)COLS;
        float V1 = (Row + 1) / (float)ROWS;

        glUseProgram(FarSnowShaderProgram);
        glUniform2f(glGetUniformLocation(FarSnowShaderProgram, "uvOffset"), U0, V0);
        glUniform2f(glGetUniformLocation(FarSnowShaderProgram, "uvScale"), U1 - U0, V1 - V0);
        glBindVertexArray(QuadVAO);
        glUniform1i(glGetUniformLocation(FarSnowShaderProgram, "snowTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, FarSnowTextureHandle);
        glActiveTexture(GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUseProgram(CartoonShaderProgram);
        glUniform1i(glGetUniformLocation(CartoonShaderProgram, "quadTexture"), 0);
        glBindVertexArray(QuadVAO);
        glBindTexture(GL_TEXTURE_2D, CartoonTextureHandle);
        glActiveTexture(GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        glUseProgram(NearSnowShaderProgram);
        glUniform2f(glGetUniformLocation(NearSnowShaderProgram, "uvOffset"), U0, V0);
        glUniform2f(glGetUniformLocation(NearSnowShaderProgram, "uvScale"), U1 - U0, V1 - V0);
        glBindVertexArray(QuadVAO);
        glUniform1i(glGetUniformLocation(NearSnowShaderProgram, "snowTexture"), 0);
        glBindTexture(GL_TEXTURE_2D, NearSnowTextureHandle);
        glActiveTexture(GL_TEXTURE0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        
        glfwSwapBuffers(pWindow);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &QuadVAO);
    glDeleteBuffers(1, &QuadVBO);
    glDeleteBuffers(1, &QuadEBO);
    glDeleteProgram(NearSnowShaderProgram);
    glDeleteProgram(FarSnowShaderProgram);
    glDeleteProgram(CartoonShaderProgram);
    glDeleteProgram(BackgroundShaderProgram);
    glfwTerminate();
    return 0;
}