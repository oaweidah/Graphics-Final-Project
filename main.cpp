#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <functional>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm.hpp>
#include <gtc/type_ptr.hpp>
#include <gtx/string_cast.hpp>
#include <gtc/matrix_transform.hpp>

#include "TriTable.hpp"
#include "FragmentShader.hpp"
#include "VertexShader.hpp"

#include "Axes.cpp"

// Corners
#define BACK_BOTTOM_LEFT     1
#define BACK_BOTTOM_RIGHT    2
#define FRONT_BOTTOM_RIGHT   4
#define FRONT_BOTTOM_LEFT    8
#define BACK_TOP_LEFT       16
#define BACK_TOP_RIGHT      32
#define FRONT_TOP_RIGHT     64
#define FRONT_TOP_LEFT     128

float f(float x, float y, float z) {
    //return y - (sin(x) * cos(z));   // Function 1
    return x * x - y * y - z * z - z;   // Function 2
}

GLFWwindow * window;

using namespace glm;

// Camera controls function
void cameraControlsGlobe(GLFWwindow * window, mat4& V, float& r, float& theta, float& phi) {
    static const float zoomSpd = 0.5f; 
    static const float rotateSpd = 5.0f;

    vec3 camPos(r * sin(radians(theta)) * cos(radians(phi)),
        r * sin(radians(phi)),
        r * cos(radians(theta)) * cos(radians(phi)));
    vec3 camDir = normalize(-camPos);
    vec3 camUp(0.0f, 1.0f, 0.0f);

    // Keyboard movement WASD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        r = std::max(r - zoomSpd, 0.1f); 
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        r += zoomSpd;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        theta += rotateSpd;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        theta -= rotateSpd;
    }

    // Left mouse button movement
    static double prevX = -1, prevY = -1;
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (prevX >= 0 && prevY >= 0) {
            theta -= rotateSpd / 10 * float(xPos - prevX);
            phi += rotateSpd / 10 * float(yPos - prevY);
            // Clamping phi to avoid flipping
            phi = clamp(phi, -89.0f, 89.0f);
        }
    }

    prevX = xPos;
    prevY = yPos;

    // Update camera position and direction
    camPos.x = r * sin(radians(theta)) * cos(radians(phi));
    camPos.y = r * sin(radians(phi));
    camPos.z = r * cos(radians(theta)) * cos(radians(phi));
    camDir = normalize(-camPos);

    // Update View Matrix
    V = lookAt(camPos, camPos + camDir, camUp);
}

// Marching Cubes mesh
class marchingCubes {
    std::function<float(float, float, float)> genFunc;
    float isoValue = 0;
    float min = 0;
    float max = 1;
    float stepSize = 0.1;
    float stepCount = 0;

    std::vector<float> vertices;

    void addOffsetTris(int* verts, float x, float y, float z) {
        for (int i = 0; verts[i] >= 0; i += 3) {
            for (int j = 0; j < 3; j++) {
                vertices.emplace_back(x + stepSize * vertTable[verts[i + j]][0]);
                vertices.emplace_back(y + stepSize * vertTable[verts[i + j]][1]);
                vertices.emplace_back(z + stepSize * vertTable[verts[i + j]][2]);
            }
        }
    }

    void generateIterative() {
        float bbl, bbr, btr, btl, fbl, fbr, ftr, ftl;
        int which = 0;
        int* verts;

        for (float a = min; a < max; a += stepSize) {
            for (float b = min; b < max; b += stepSize) {
                // Square Testing
                bbl = genFunc(a, b, stepCount);
                bbr = genFunc(a + stepSize, b, stepCount);
                btr = genFunc(a + stepSize, b + stepSize, stepCount);
                btl = genFunc(a, b + stepSize, stepCount);
                fbl = genFunc(a, b, stepCount + stepSize);
                fbr = genFunc(a + stepSize, b, stepCount + stepSize);
                ftr = genFunc(a + stepSize, b + stepSize, stepCount + stepSize);
                ftl = genFunc(a, b + stepSize, stepCount + stepSize);
                
                which = 0;
                if (bbl < isoValue) {
                    which |= BACK_BOTTOM_LEFT;
                }
                if (bbr < isoValue) {
                    which |= BACK_BOTTOM_RIGHT;
                }
                if (btr < isoValue) {
                    which |= BACK_TOP_RIGHT;
                }
                if (btl < isoValue) {
                    which |= BACK_TOP_LEFT;
                }
                if (fbl < isoValue) {
                    which |= FRONT_BOTTOM_LEFT;
                }
                if (fbr < isoValue) {
                    which |= FRONT_BOTTOM_RIGHT;
                }
                if (ftr < isoValue) {
                    which |= FRONT_TOP_RIGHT;
                }
                if (ftl < isoValue) {
                    which |= FRONT_TOP_LEFT;
                }

                verts = TriTable[which];
                addOffsetTris(verts, a, b, stepCount);
            }
        }
        stepCount += stepSize;
        if (stepCount > max) {
            finished = true;
        }
    }

public:
    bool finished = false;

    marchingCubes(std::function<float(float, float, float)> f, float isoValue, float min, float max, float step)
        : genFunc(f), isoValue(isoValue), min(min), max(max), stepSize(step), stepCount(min) {}

    void generate() {
        generateIterative();
    }

    std::vector<float> getVertices() {
        return vertices;
    }
};

std::vector<float> compute_normals(std::vector<float> vertices) {
    std::vector<float> normals;
    vec3 p1, p2, p3, p1to2, p1to3, n, cross;

    for (int i = 0; i < vertices.size(); i += 9) {
        p1 = { vertices[i], vertices[i + 1], vertices[i + 2] };
        p2 = { vertices[i + 3], vertices[i + 4], vertices[i + 5] };
        p3 = { vertices[i + 6], vertices[i + 7], vertices[i + 8] };

        float p1to2x = p2.x - p1.x;
        float p1to2y = p2.y - p1.y;
        float p1to2z = p2.z - p1.z;
        float p1to3x = p3.x - p1.x;
        float p1to3y = p3.y - p1.y;
        float p1to3z = p3.z - p1.z;

        float crossX = (p1to2y * p1to3z) - (p1to2z * p1to3y);
        float crossY = (p1to2z * p1to3x) - (p1to2x * p1to3z);
        float crossZ = (p1to2x * p1to3y) - (p1to2y * p1to3x);

        cross = vec3(crossX, crossY, crossZ);
        n = normalize(cross);

        for (int j = 0; j < 3; j++) {
            normals.emplace_back(n.z);
            normals.emplace_back(n.y);
            normals.emplace_back(n.x);
        }
    }
    return normals;
}

void writePLY(std::string outputFile, std::vector<float> vertexData, std::vector<float> normalData) {
    std::ofstream plyFile(outputFile);

    if (plyFile.fail()) {
        return;
    }

    int vertexCount = vertexData.size() / 3;
    int faceCount = vertexCount / 3;

    plyFile << "ply" << std::endl;
    plyFile << "format ascii 1.0" << std::endl;
    plyFile << "element vertex " << vertexCount << std::endl;
    plyFile << "property float x" << std::endl;
    plyFile << "property float y" << std::endl;
    plyFile << "property float z" << std::endl;
    plyFile << "property float nx" << std::endl;
    plyFile << "property float ny" << std::endl;
    plyFile << "property float nz" << std::endl;
    plyFile << "element face " << faceCount << std::endl;
    plyFile << "property list uchar uint vertex_indices" << std::endl;
    plyFile << "end_header" << std::endl;

    for (size_t idx = 2; idx < vertexData.size(); idx += 3) {
        plyFile << vertexData[idx - 2] << " " << vertexData[idx - 1] << " " << vertexData[idx] << " " << normalData[idx - 2] << " " << normalData[idx - 1] << " " << normalData[idx] << std::endl;
    }

    for (size_t idx = 2; idx < vertexCount; idx += 3) {
        plyFile << "3 " << idx - 2 << " " << idx - 1 << " " << idx << std::endl;
    }

    plyFile.close();
}


int main(int argc, char* argv[]) {
    std::vector<float> normals;
    float min = -5.0f;
    float max = 5.0f;
    float stepValue = 0.05;
    float isoValue = -1.5;
    std::string filename = "Render.ply";
    bool generateFile = true;

    // Init window
    if (!glfwInit()) {
        printf("Failed to initialize GLFW\n");
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);

    window = glfwCreateWindow(1000, 1000, "Assignment 5", NULL, NULL);
    if (window == NULL) {
        printf("Failed to open window\n");
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // Init GLEW
    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        printf("Failed to initialize GLEW\n");
        glfwTerminate();
        return -1;
    }

    glClearColor(0.2, 0.2, 0.3, 0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Initial MVP
    mat4 mvp;
    vec3 eyePos(5, 5, 5);
    vec3 zero(0, 0, 0);
    vec3 up(0, 1, 0);
    mat4 view = lookAt(eyePos, zero, up);
    mat4 projection = perspective(radians(45.0f), 1.0f, 0.001f, 1000.0f);
    mat4 model = mat4(1.0f);
    mvp = projection * view * model;

    marchingCubes cubes(f, isoValue, min, max, stepValue);
    Cube drawCube(min, max);

    // VAO and buffers
    GLuint vao, vertexVBO, normalVBO, programID;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Vertex VBO
    glGenBuffers(1, &vertexVBO);
    glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        0,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
    );

    // Normal VBO
    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, 0, &normals, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        0,
        (void*)0
    );

    glBindVertexArray(0);

    // Shaders
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(vertexShaderID, 1, &vertexShader, NULL);
    glCompileShader(vertexShaderID);
    glShaderSource(fragmentShaderID, 1, &fragmentShader, NULL);
    glCompileShader(fragmentShaderID);
    programID = glCreateProgram();
    glAttachShader(programID, vertexShaderID);
    glAttachShader(programID, fragmentShaderID);
    glLinkProgram(programID);
    glDetachShader(programID, vertexShaderID);
    glDetachShader(programID, fragmentShaderID);
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);

    // Camera/Input Variables
    float r = 30.0f;
    float theta = 45.0f;
    float phi = 45.0f;
 
    GLfloat MODEL_COLOR[4] = { 0.0f, 1.0f, 1.0f, 1.0f };
    GLfloat LIGHT_DIRECTION[3] = { 5.0f, 5.0f, 5.0f };
    double prevTime = glfwGetTime();
    bool fileWritten = false;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        double currentTime = glfwGetTime();
        float deltaTime = currentTime - prevTime;
        prevTime = currentTime;

        cameraControlsGlobe(window, view, r, theta, phi);

        mvp = projection * view * model;

        if (!cubes.finished) {
            cubes.generate();
            std::vector<float> vertices = cubes.getVertices();
            normals = compute_normals(vertices);

            // Update array buffers
            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
            glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(GL_FLOAT), &normals[0], GL_DYNAMIC_DRAW);
            glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(GL_FLOAT), &vertices[0], GL_DYNAMIC_DRAW);
            glBindVertexArray(0);

        }
        else if (!fileWritten && generateFile) {
            std::vector<float> vertices = cubes.getVertices();
            normals = compute_normals(vertices);
            writePLY("Render.ply", vertices, normals);
            fileWritten = true;
        }

        // Draw axes and cube
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadMatrixf(value_ptr(projection));
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadMatrixf(value_ptr(view));
        drawCube.draw();

        glUseProgram(programID);

        GLuint matrixID = glGetUniformLocation(programID, "MVP");
        glUniformMatrix4fv(matrixID, 1, GL_FALSE, &mvp[0][0]);

        GLuint viewID = glGetUniformLocation(programID, "V");
        glUniformMatrix4fv(viewID, 1, GL_FALSE, &view[0][0]);

        GLuint colorID = glGetUniformLocation(programID, "modelColor");
        glUniform4fv(colorID, 1, MODEL_COLOR);

        GLuint lightDirID = glGetUniformLocation(programID, "lightDir");
        glUniform3fv(lightDirID, 1, LIGHT_DIRECTION);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, normals.size());
        glBindVertexArray(0);
        glUseProgram(0);


        glfwPollEvents();
        glfwSwapBuffers(window);
    }

    return 0;
}