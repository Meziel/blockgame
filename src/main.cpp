#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#include <cmath>
#include <vector>
#include <cstdio>
#include <algorithm> // For std::remove_if

// ------------------------------------------------------
// Explicit WebGL Context Initialization
// ------------------------------------------------------
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context;

void initContext() {
    EmscriptenWebGLContextAttributes attr;
    emscripten_webgl_init_context_attributes(&attr);
    
    // Attempt to create a WebGL 2.0 context first.
    attr.majorVersion = 2;
    context = emscripten_webgl_create_context("#canvas", &attr);
    if (context <= 0) {
        printf("WebGL 2.0 not supported. Trying WebGL 1.0...\n");
        // Fall back to WebGL 1.0
        attr.majorVersion = 1;
        context = emscripten_webgl_create_context("#canvas", &attr);
    }
    if (context <= 0) {
        printf("Failed to create WebGL context!\n");
    } else {
        printf("WebGL context created successfully.\n");
        emscripten_webgl_make_context_current(context);
    }
}

// ------------------------------------------------------
// Shader sources for flat coloring
// ------------------------------------------------------
const char* vertexShaderSource = R"(
attribute vec2 aPosition;
uniform vec2 uTranslation;
uniform vec2 uScale;
void main() {
    // Apply translation and scale
    vec2 pos = aPosition * uScale + uTranslation;
    // Convert to clip space
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

const char* fragmentShaderSource = R"(
precision mediump float;
uniform vec4 uColor;
void main() {
    gl_FragColor = uColor;
}
)";

// ------------------------------------------------------
// Global variables for shaders and buffers
// ------------------------------------------------------
static GLuint program = 0;
static GLint aPositionLoc = -1;
static GLint uTranslationLoc = -1;
static GLint uScaleLoc = -1;
static GLint uColorLoc = -1;

static GLuint playerVBO = 0; // 2D quad for the player
static GLuint spikeVBO = 0;  // 2D triangle for spikes

// Player state
static float playerY = 0.0f;        // Player vertical position
static float playerVelocity = 0.0f; // Player vertical velocity
static bool  isOnGround = true;

// World scrolling variables
static float scrollSpeed = 0.02f;      // Speed at which spikes move left
static float spikeSpawnTimer = 0.0f;
static float spikeSpawnInterval = 2.0f; // seconds between spawns

// Gravity and jump settings
static const float gravity = -0.06f;
static const float jumpVelocity = 0.02f;

// Time tracking for updates
static double lastFrameTime = 0.0;

// Structure for spikes
struct Spike {
    float x;
    float y;
};
static std::vector<Spike> spikes;

// ------------------------------------------------------
// Compile a shader from source
// ------------------------------------------------------
GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint status = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, nullptr, buffer);
        printf("Shader compile error: %s\n", buffer);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

// ------------------------------------------------------
// Link vertex and fragment shaders into a program
// ------------------------------------------------------
GLuint createProgram(const char* vsSource, const char* fsSource) {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSource);
    if (!vs || !fs) {
        return 0;
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint status;
    glGetProgramiv(prog, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        char buffer[512];
        glGetProgramInfoLog(prog, 512, nullptr, buffer);
        printf("Program link error: %s\n", buffer);
        glDeleteProgram(prog);
        return 0;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

// ------------------------------------------------------
// Initialize GL objects (VBOs, shaders, etc.)
// ------------------------------------------------------
void initGL() {
    program = createProgram(vertexShaderSource, fragmentShaderSource);
    glUseProgram(program);

    // Retrieve attribute and uniform locations
    aPositionLoc = glGetAttribLocation(program, "aPosition");
    uTranslationLoc = glGetUniformLocation(program, "uTranslation");
    uScaleLoc = glGetUniformLocation(program, "uScale");
    uColorLoc = glGetUniformLocation(program, "uColor");

    // Define the player quad (a square centered at (0,0))
    GLfloat playerVertices[] = {
        -0.05f, -0.05f,
         0.05f, -0.05f,
        -0.05f,  0.05f,
         0.05f, -0.05f,
         0.05f,  0.05f,
        -0.05f,  0.05f
    };
    glGenBuffers(1, &playerVBO);
    glBindBuffer(GL_ARRAY_BUFFER, playerVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(playerVertices), playerVertices, GL_STATIC_DRAW);

    // Define a spike as a triangle (isosceles)
    GLfloat spikeVertices[] = {
        -0.05f, 0.0f,
         0.05f, 0.0f,
         0.0f,  0.1f
    };
    glGenBuffers(1, &spikeVBO);
    glBindBuffer(GL_ARRAY_BUFFER, spikeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(spikeVertices), spikeVertices, GL_STATIC_DRAW);

    // Set initial GL state
    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// ------------------------------------------------------
// Handle user input (jump on space key)
// ------------------------------------------------------
extern "C" {
EMSCRIPTEN_KEEPALIVE
void onKeyDown(int keyCode) {
    // Space key is typically keyCode 32
    if (keyCode == 32) {
        if (isOnGround) {
            playerVelocity = jumpVelocity;
            isOnGround = false;
        }
    }
}
}

// ------------------------------------------------------
// Update game logic: player physics, spike spawning/movement, collision
// ------------------------------------------------------
void update(double currentTime) {
    float deltaTime = float(currentTime - lastFrameTime);
    lastFrameTime = currentTime;

    // Update player physics (gravity and jump)
    playerVelocity += gravity * deltaTime;
    playerY += playerVelocity;
    if (playerY < -0.4f) {
        // Simulate ground collision
        playerY = -0.4f;
        playerVelocity = 0.0f;
        isOnGround = true;
    }

    // Spawn spikes periodically
    spikeSpawnTimer += deltaTime;
    if (spikeSpawnTimer >= spikeSpawnInterval) {
        spikeSpawnTimer = 0.0f;
        // Spawn spike off-screen to the right
        spikes.push_back({1.2f, -0.4f});
    }

    // Move spikes to the left
    for (auto &spike : spikes) {
        spike.x -= scrollSpeed * deltaTime * 60.0f;
    }

    // Remove spikes that have gone off-screen to the left
    spikes.erase(
        std::remove_if(spikes.begin(), spikes.end(),
            [](const Spike &s){ return s.x < -1.2f; }),
        spikes.end()
    );

    // Check collisions (using simple bounding boxes)
    for (auto &spike : spikes) {
        float spikeWidth = 0.05f;  // half-width approximation
        float spikeHeight = 0.1f;  // half-height from base to tip

        bool collisionX = (fabs(spike.x - 0.0f) < (spikeWidth + 0.05f));
        bool collisionY = (fabs(spike.y - playerY) < (spikeHeight + 0.05f));

        if (collisionX && collisionY) {
            // Collision: reset player state and clear spikes
            playerY = -0.4f;
            playerVelocity = 0.0f;
            isOnGround = true;
            spikes.clear();
            printf("Collision! Resetting...\n");
            break;
        }
    }
}

// ------------------------------------------------------
// Render the scene: draw player and spikes
// ------------------------------------------------------
void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(program);

    // Draw the player (a square)
    glBindBuffer(GL_ARRAY_BUFFER, playerVBO);
    glEnableVertexAttribArray(aPositionLoc);
    glVertexAttribPointer(aPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    // Player remains at x=0, with y position varying
    glUniform2f(uTranslationLoc, 0.0f, playerY);
    glUniform2f(uScaleLoc, 1.0f, 1.0f);
    glUniform4f(uColorLoc, 0.0f, 0.0f, 0.0f, 1.0f); // Black color for player
    glDrawArrays(GL_TRIANGLES, 0, 6);

    // Draw spikes
    glBindBuffer(GL_ARRAY_BUFFER, spikeVBO);
    glVertexAttribPointer(aPositionLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glUniform2f(uScaleLoc, 1.0f, 1.0f);
    glUniform4f(uColorLoc, 1.0f, 1.0f, 1.0f, 1.0f); // White color for spikes
    for (auto &spike : spikes) {
        glUniform2f(uTranslationLoc, spike.x, spike.y);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }
    glDisableVertexAttribArray(aPositionLoc);
}

// ------------------------------------------------------
// Main loop called by Emscripten's requestAnimationFrame
// ------------------------------------------------------
void mainLoop() {
    double currentTime = emscripten_get_now() / 1000.0; // Convert ms to seconds
    update(currentTime);
    render();
}

// ------------------------------------------------------
// Entry point
// ------------------------------------------------------
int main() {
    // First, initialize the WebGL context explicitly
    initContext();

    // Then initialize shaders, buffers, and other GL state
    initGL();

    // Start the main loop using the browser's requestAnimationFrame
    emscripten_set_main_loop(mainLoop, 0, 1);
    return 0;
}
