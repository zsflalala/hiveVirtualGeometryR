namespace hiveVG
{
    std::string TexturePathBackground = "Textures/background4.jpg";
    const char SnowVertexShaderSource[] = R"vertex(#version 300 es
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;

        void main()
        {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = vec2(aTexCoord.x, aTexCoord.y);
        }
        )vertex";

    const char SnowFragmentShaderSource[] = R"fragment(#version 300 es
        precision mediump float;
        out vec4 FragColor;

        in vec2 TexCoord;
        uniform vec2 uvOffset;
        uniform vec2 uvScale;
        uniform sampler2D snowTexture;

        void main()
        {
            vec2 FinalUV = (TexCoord * uvScale) + uvOffset;
            vec4 SnowColor = texture(snowTexture, FinalUV);
            if(SnowColor.a < 0.1)
                discard;
            FragColor = SnowColor;
        }
        )fragment";

    const char QuadVertexShaderSource[] = R"vertex(#version 300 es
        layout (location = 0) in vec2 aPos;
        layout (location = 1) in vec2 aTexCoord;

        out vec2 TexCoord;
//        uniform mat4 Model;
//        uniform mat4 View;
//        uniform mat4 Projection;

        void main()
        {
            gl_Position = vec4(aPos, 0.0, 1.0);
            TexCoord = vec2(aTexCoord.x, aTexCoord.y);
        }
        )vertex";

    const char QuadFragmentShaderSource[] = R"fragment(#version 300 es
        precision mediump float;
        out vec4 FragColor;

        in vec2 TexCoord;
        uniform sampler2D quadTexture;

        void main()
        {
            vec4 QuadColor = texture(quadTexture, TexCoord);
            FragColor = QuadColor;
        }
        )fragment";

    const char VertShaderCode[] = R"vertex(#version 300 es
        layout (location = 0) in vec2 inPosition;
        layout (location = 1) in vec2 inUV;

        out vec2 fragUV;

        void main() {
            fragUV = vec2(inUV.x, inUV.y);
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
}