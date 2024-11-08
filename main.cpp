#include <cstdio>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

using namespace std;

void framebufferSizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window){
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

struct Buffer{
    size_t width, height;
    uint32_t* data;
};

struct Sprite{
    size_t width, height;
    uint8_t* data;
};

void clearBuffer(Buffer* buffer, uint32_t colour){
    for (size_t i = 0; i < buffer->width * buffer->height; i++)    {
        buffer->data[i] = colour;
    }
}

void drawSprite(Buffer* buffer, const Sprite& sprite, size_t x, size_t y, uint32_t colour){
    for (size_t i = 0; i < sprite.width; i++){
        for (size_t j = 0; j < sprite.height; j++){
            if (sprite.data[j * sprite.width + i] && (sprite.height - 1 + y - j) < buffer->height && (x + i) < buffer->width){
                buffer->data[(sprite.height - 1 + y - j) * buffer->width + (x + i)] = colour;
            }
        }
    }
}

uint32_t rgbToUint32(uint8_t r, uint8_t g, uint8_t b){
    return (r << 24) | (g << 16) | (b << 8) | 255;
}

const char* vertexShader =
    "\n"
    "#version 330\n"
    "\n"
    "noperspective out vec2 TexCoord;\n"
    "\n"
    "void main(void){\n"
    "\n"
    "    TexCoord.x = (gl_VertexID == 2)? 2.0: 0.0;\n"
    "    TexCoord.y = (gl_VertexID == 1)? 2.0: 0.0;\n"
    "    \n"
    "    gl_Position = vec4(2.0 * TexCoord - 1.0, 0.0, 1.0);\n"
    "}\n";

const char* fragmentShader =
    "\n"
    "#version 330\n"
    "\n"
    "uniform sampler2D buffer;\n"
    "noperspective in vec2 TexCoord;\n"
    "\n"
    "out vec3 outColor;\n"
    "\n"
    "void main(void){\n"
    "    outColor = texture(buffer, TexCoord).rgb;\n"
    "}\n";

const size_t bufferWidth = 224;
const size_t bufferHeight = 256;

int main(){
    //create alien sprite
    Sprite alien;
    alien.width = 11;
    alien.height = 8;
    alien.data = new uint8_t[11 * 8]{
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
        0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
    };

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //create window
    GLFWwindow* window = glfwCreateWindow(bufferWidth, bufferHeight, "space invaders", NULL, NULL);
    if (window == NULL)    {
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))    {
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, bufferWidth, bufferHeight);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    //create buffer
    uint32_t clearColour = rgbToUint32(0, 0, 0);
    Buffer buffer;
    buffer.width = bufferWidth;
    buffer.height = bufferHeight;
    buffer.data = new uint32_t[buffer.width * buffer.height];
    clearBuffer(&buffer, clearColour);

    //create vertex array object
    GLuint fullscreen_triangle_vao;
    glGenVertexArrays(1, &fullscreen_triangle_vao);
    glBindVertexArray(fullscreen_triangle_vao);

    GLuint shaderID = glCreateProgram();

    //create vertex shader
    GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vertexShader, 0);
    glCompileShader(vertex);
    glAttachShader(shaderID, vertex);
    glDeleteShader(vertex);

    //create fragment shader
    GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fragmentShader, 0);
    glCompileShader(fragment);
    glAttachShader(shaderID, fragment);
    glDeleteShader(fragment);

    glLinkProgram(shaderID);
    glUseProgram(shaderID);

    //create OpenGL texture for buffer
    GLuint bufferTexture;
    glGenTextures(1, &bufferTexture);
    glBindTexture(GL_TEXTURE_2D, bufferTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, buffer.width, buffer.height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    GLint location = glGetUniformLocation(shaderID, "buffer");
    glUniform1i(location, 0);

    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);

    //render loop
    while (!glfwWindowShouldClose(window)){
        //input
        processInput(window);

        //render commands
        clearBuffer(&buffer, clearColour);
        drawSprite(&buffer, alien, 112, 128, rgbToUint32(0, 255, 0));

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        //check and call events, swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    delete[] buffer.data;
    delete[] alien.data;

    glfwTerminate();

    return 0;
}

