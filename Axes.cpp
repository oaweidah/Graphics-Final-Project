#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm.hpp>

class Cube {

    float min;
    float max;

    glm::vec3 xcol = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 ycol = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 zcol = glm::vec3(0.0f, 0.0f, 1.0f);

public:

    Cube(float min, float max) : min(min + 0.1), max(max + 0.1) {}

    void draw() {
        float extendAxis = 0.5f;
        float arrowLength = 0.25f;

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();

        glLineWidth(2.0f);
        glBegin(GL_LINES);

        glColor4f(1.0, 0.0f, 0.0f, 1.0f);
        glVertex3f(min, min, min);
        glVertex3f(max + extendAxis, min, min);

        glColor4f(0.0, 1.0f, 0.0f, 1.0f);
        glVertex3f(min, min, min);
        glVertex3f(min, max + extendAxis, min);

        glColor4f(0.0, 0.0f, 1.0f, 1.0f);
        glVertex3f(min, min, min);
        glVertex3f(min, min, max + extendAxis);

        glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
        glColor4f(0.5f, 0.5f, 0.5f, 1.0f);
        glVertex3f(min, max, min);
        glVertex3f(max, max, min);

        glVertex3f(max, min, min);
        glVertex3f(max, max, min);

        glVertex3f(max, min, min);
        glVertex3f(max, min, max);

        glVertex3f(max, min, max);
        glVertex3f(max, max, max);

        glVertex3f(max, max, min);
        glVertex3f(max, max, max);

        glVertex3f(min, max, min);
        glVertex3f(min, max, max);

        glVertex3f(min, max, max);
        glVertex3f(min, min, max);

        glVertex3f(min, min, max);
        glVertex3f(max, min, max);

        glVertex3f(min, max, max);
        glVertex3f(max, max, max);
        glEnd();

        glPopMatrix();
    }
};