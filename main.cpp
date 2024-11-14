#include <cstdio>
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>

using namespace std;

#define MAX_BULLETS 128

//global variables for player input
int inputDir = 0;
bool fire = 0;

struct Buffer{
    size_t width, height;
    uint32_t* data;
};

struct Sprite{
    size_t width, height;
    uint8_t* data;
};

struct Alien{
    size_t x, y;
    uint8_t type;
};

struct Player{
    size_t x, y;
    size_t lives;
};

struct Bullet {
    size_t x, y;
    int dir;
};

struct Game{
    size_t width, height;
    size_t alienNum;
    size_t bulletNum;
    Alien* aliens;
    Player player;
    Bullet bullets[MAX_BULLETS];
};

struct SpriteAnimation{
    bool loop;
    size_t frameNum;
    size_t frameDuration;
    size_t time;
    Sprite** frames;
};

enum AlienType : uint8_t{
    ALIEN_DEAD = 0,
    ALIEN_A = 1,
    ALIEN_B = 2,
    ALIEN_C = 3
};

void framebufferSizeCallback(GLFWwindow* window, int width, int height){
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window, int key, int scancode, int action, int mods){
    switch (key) {
    case GLFW_KEY_ESCAPE:
        if(action == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
        break;
    case GLFW_KEY_RIGHT:
        if (action == GLFW_PRESS) inputDir += 1;
        else if (action == GLFW_RELEASE) inputDir -= 1;
        break;
    case GLFW_KEY_LEFT:
        if (action == GLFW_PRESS) inputDir -= 1;
        else if (action == GLFW_RELEASE) inputDir += 1;
        break;    
    case GLFW_KEY_SPACE:
        if (action == GLFW_PRESS) fire = true;
        break;
    }
}

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
const int frameDelay = 1000 / 60 + 1; //60fps

int main(){
    //create alien sprites
    Sprite alienSprites[6];

    alienSprites[0].width = 8;
    alienSprites[0].height = 8;
    alienSprites[0].data = new uint8_t[8 * 8]{
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,0,0,0,0,0,1, // @......@
        0,1,0,0,0,0,1,0  // .@....@.
    };

    alienSprites[1].width = 8;
    alienSprites[1].height = 8;
    alienSprites[1].data = new uint8_t[8 * 8]{
        0,0,0,1,1,0,0,0, // ...@@...
        0,0,1,1,1,1,0,0, // ..@@@@..
        0,1,1,1,1,1,1,0, // .@@@@@@.
        1,1,0,1,1,0,1,1, // @@.@@.@@
        1,1,1,1,1,1,1,1, // @@@@@@@@
        0,0,1,0,0,1,0,0, // ..@..@..
        0,1,0,1,1,0,1,0, // .@.@@.@.
        1,0,1,0,0,1,0,1  // @.@..@.@
    };

    alienSprites[2].width = 11;
    alienSprites[2].height = 8;
    alienSprites[2].data = new uint8_t[11 * 8]{
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,0,0,1,0,0,0,1,0,0,0, // ...@...@...
        0,0,1,1,1,1,1,1,1,0,0, // ..@@@@@@@..
        0,1,1,0,1,1,1,0,1,1,0, // .@@.@@@.@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,0,1,0,0,0,0,0,1,0,1, // @.@.....@.@
        0,0,0,1,1,0,1,1,0,0,0  // ...@@.@@...
    };

    alienSprites[3].width = 11;
    alienSprites[3].height = 8;
    alienSprites[3].data = new uint8_t[11 * 8]{
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        1,0,0,1,0,0,0,1,0,0,1, // @..@...@..@
        1,0,1,1,1,1,1,1,1,0,1, // @.@@@@@@@.@
        1,1,1,0,1,1,1,0,1,1,1, // @@@.@@@.@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        0,0,1,0,0,0,0,0,1,0,0, // ..@.....@..
        0,1,0,0,0,0,0,0,0,1,0  // .@.......@.
    };

    alienSprites[4].width = 12;
    alienSprites[4].height = 8;
    alienSprites[4].data = new uint8_t[12 * 8]{
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,0,1,1,0,0,1,1,0,0,0, // ...@@..@@...
        0,0,1,1,0,1,1,0,1,1,0,0, // ..@@.@@.@@..
        1,1,0,0,0,0,0,0,0,0,1,1  // @@........@@
    };


    alienSprites[5].width = 12;
    alienSprites[5].height = 8;
    alienSprites[5].data = new uint8_t[12 * 8]{
        0,0,0,0,1,1,1,1,0,0,0,0, // ....@@@@....
        0,1,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        1,1,1,0,0,1,1,0,0,1,1,1, // @@@..@@..@@@
        1,1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@@
        0,0,1,1,1,0,0,1,1,1,0,0, // ..@@@..@@@..
        0,1,1,0,0,1,1,0,0,1,1,0, // .@@..@@..@@.
        0,0,1,1,0,0,0,0,1,1,0,0  // ..@@....@@..
    };

    Sprite alienDeathSprite;
    alienDeathSprite.width = 13;
    alienDeathSprite.height = 7;
    alienDeathSprite.data = new uint8_t[13 * 7]{
        0,1,0,0,1,0,0,0,1,0,0,1,0, // .@..@...@..@.
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        1,1,0,0,0,0,0,0,0,0,0,1,1, // @@.........@@
        0,0,0,1,0,0,0,0,0,1,0,0,0, // ...@.....@...
        0,0,1,0,0,1,0,1,0,0,1,0,0, // ..@..@.@..@..
        0,1,0,0,1,0,0,0,1,0,0,1,0  // .@..@...@..@.
    };

    //player sprite
    Sprite playerSprite;
    playerSprite.width = 11;
    playerSprite.height = 7;
    playerSprite.data = new uint8_t[11 * 7]{
        0,0,0,0,0,1,0,0,0,0,0, // .....@.....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,0,0,0,1,1,1,0,0,0,0, // ....@@@....
        0,1,1,1,1,1,1,1,1,1,0, // .@@@@@@@@@.
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
        1,1,1,1,1,1,1,1,1,1,1, // @@@@@@@@@@@
    };

    //bullet sprite
    Sprite bulletSprite;
    bulletSprite.width = 1;
    bulletSprite.height = 3;
    bulletSprite.data = new uint8_t[3]{
        1, // @
        1, // @
        1  // @
    };

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    //create window
    GLFWwindow* window = glfwCreateWindow(bufferWidth, bufferHeight, "space invaders", NULL, NULL);
    if (window == NULL){
        cout << "Failed to create GLFW window" << endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
        cout << "Failed to initialize GLAD" << endl;
        return -1;
    }

    glViewport(0, 0, bufferWidth, bufferHeight);

    glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

    //turn on vsync
    glfwSwapInterval(1);

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

    //create alien animation
    SpriteAnimation alienAnimation[3];

    for (size_t i = 0; i < 3; i++){
        alienAnimation[i].loop = true;
        alienAnimation[i].frameNum = 2;
        alienAnimation[i].frameDuration = 10;
        alienAnimation[i].time = 0;

        alienAnimation[i].frames = new Sprite * [2];
        alienAnimation[i].frames[0] = &alienSprites[2 * i];
        alienAnimation[i].frames[1] = &alienSprites[2 * i + 1];
    }

    //create game struct
    Game game;
    game.width = bufferWidth;
    game.height = bufferHeight;
    game.bulletNum = 0;
    game.alienNum = 55;
    game.aliens = new Alien[game.alienNum];

    game.player.x = 112 - 5;
    game.player.y = 32;

    game.player.lives = 3;

    for (size_t i = 0; i < 5; i++){
        for (size_t j = 0; j < 11; j++){
            Alien& alien = game.aliens[i * 11 + j];
            alien.type = (5 - i) / 2 + 1;

            const Sprite& sprite = alienSprites[2 * (alien.type - 1)];

            alien.x = 16 * j + 20 + (alienDeathSprite.width - sprite.width) / 2;
            alien.y = 17 * i + 128;
        }
    }

    uint8_t* deathCounters = new uint8_t[game.alienNum];
    for (size_t i = 0; i < game.alienNum; i++){
        deathCounters[i] = 10;
    }

    //render loop
    while (!glfwWindowShouldClose(window)){
        auto frameStart = chrono::high_resolution_clock::now();

        //process user input
        glfwSetKeyCallback(window, processInput);

        //render commands
        clearBuffer(&buffer, clearColour);

        //draw aliens
        for (size_t i = 0; i < game.alienNum; i++){
            if (!deathCounters[i]) continue;

            const Alien& alien = game.aliens[i];

            if (alien.type == ALIEN_DEAD) {
                drawSprite(&buffer, alienDeathSprite, alien.x, alien.y, rgbToUint32(0, 255, 0));
            }

            else {
                const SpriteAnimation& animation = alienAnimation[alien.type - 1];
                size_t currentFrame = animation.time / animation.frameDuration;
                const Sprite& sprite = *animation.frames[currentFrame];
                drawSprite(&buffer, sprite, alien.x, alien.y, rgbToUint32(0, 255, 0));
            }            
        }

        //draw player
        drawSprite(&buffer, playerSprite, game.player.x, game.player.y, rgbToUint32(0, 255, 0));

        //draw bullets
        for (size_t i = 0; i < game.bulletNum; i++)
        {
            const Bullet& bullet = game.bullets[i];
            const Sprite& sprite = bulletSprite;
            drawSprite(&buffer, sprite, bullet.x, bullet.y, rgbToUint32(0, 255, 0));
        }

        //update animations
        for (size_t i = 0; i < 3; i++) {
            alienAnimation[i].time++;
            if (alienAnimation[i].time == alienAnimation[i].frameNum * alienAnimation[i].frameDuration) {
                alienAnimation[i].time = 0;
            }
        }

        for (size_t i = 0; i < game.alienNum; i++) {
            const Alien& alien = game.aliens[i];
            if (alien.type == ALIEN_DEAD && deathCounters[i]) {
                deathCounters[i]--;
            }
        }

        //update bullets
        for (size_t i = 0; i < game.bulletNum; i++){
            game.bullets[i].y += game.bullets[i].dir;
            if (game.bullets[i].y >= game.height || game.bullets[i].y < bulletSprite.height){
                game.bullets[i] = game.bullets[game.bulletNum - 1];
                game.bulletNum--;
                continue;
            }
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer.width, buffer.height, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, buffer.data);
        
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        //check and call events, swap buffers
        glfwSwapBuffers(window);

        //update player movement
        int playerDir = 2 * inputDir;

        if (playerDir != 0){
            if (game.player.x + playerSprite.width + playerDir >= game.width) {
                game.player.x = game.width - playerSprite.width;
            }
            else if ((int)game.player.x + playerDir <= 0) {
                game.player.x = 0;
                playerDir *= -1;
            }
            else game.player.x += playerDir;
        }

        if (fire && game.bulletNum < MAX_BULLETS){
            game.bullets[game.bulletNum].x = game.player.x + playerSprite.width / 2;
            game.bullets[game.bulletNum].y = game.player.y + playerSprite.height;
            game.bullets[game.bulletNum].dir = 2;
            game.bulletNum++;
        }

        fire = false;

        glfwPollEvents();

        this_thread::sleep_for(chrono::milliseconds(frameDelay));
    }

    for (size_t i = 0; i < 6; i++){
        delete[] alienSprites[i].data;
    }

    delete[] alienDeathSprite.data;

    for (size_t i = 0; i < 3; i++){
        delete[] alienAnimation[i].frames;
    }

    delete[] buffer.data;
    delete[] game.aliens;
    delete[] deathCounters;

    glfwTerminate();

    return 0;
}

