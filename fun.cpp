// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

GLFWwindow *window;

// Include GLM
#include <glm/glm.hpp>

using namespace glm;

#include "shader.cpp"

#include <glm/gtc/matrix_transform.hpp>


#include <ctime>

typedef float Matrix3x3[3][3];
Matrix3x3 theMatrix;
int NEdges = 3;
float refpt[2];
float shearValue = 2;


void matrixSetIdentity(Matrix3x3 m)
// Initialises the matrix as Unit Matrix
{
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            m[i][j] = (i == j);
}


void matrixPreMultiply(Matrix3x3 a, Matrix3x3 b)
// Multiplies matrix a times b, putting result in b
{
    int i, j;
    Matrix3x3 tmp;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            tmp[i][j] = a[i][0] * b[0][j] + a[i][1] * b[1][j] + a[i][2] * b[2][j];
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            theMatrix[i][j] = tmp[i][j];
}


void Translate(float tx, float ty) {
    Matrix3x3 m;
    matrixSetIdentity(m);
    m[0][2] = tx;
    m[1][2] = ty;
    matrixPreMultiply(m, theMatrix);
}


void Scale(float sx, float sy) {
    Matrix3x3 m;
    matrixSetIdentity(m);
    m[0][0] = sx;
    m[0][2] = (1 - sx) * refpt[0];
    m[1][1] = sy;
    m[1][2] = (1 - sy) * refpt[1];
    matrixPreMultiply(m, theMatrix);
}


void Rotate(float a) {
    Matrix3x3 m;
    matrixSetIdentity(m);
    a = a * 22 / 1260;
    m[0][0] = cos(a);
    m[0][1] = -sin(a);
    m[0][2] = refpt[0] * (1 - cos(a)) + refpt[1] * sin(a);
    m[1][0] = sin(a);
    m[1][1] = cos(a);
    m[1][2] = refpt[1] * (1 - cos(a)) - refpt[0] * sin(a);
    matrixPreMultiply(m, theMatrix);
}


void Reflect(int xy) {
    Matrix3x3 m;
    matrixSetIdentity(m);
    if (xy == 1)
        m[1][1] = -1;
    if (xy == 2)
        m[0][0] = -1;
    matrixPreMultiply(m, theMatrix);
}


void Shear(int xy, float shearValue) {
    Matrix3x3 m;
    matrixSetIdentity(m);
    if (xy == 1)
        m[0][1] = shearValue;
    if (xy == 2)
        m[1][0] = shearValue;
    matrixPreMultiply(m, theMatrix);
}


void TransformPoints(float ptsIni[20][2], float ptsFin[20][2]) {

    for (int k = 0; k < NEdges && k < 20; k++) {
        ptsFin[k][0] = theMatrix[0][0] * ptsIni[k][0] + theMatrix[0][1] * ptsIni[k][1] + theMatrix[0][2];
        ptsFin[k][1] = theMatrix[1][0] * ptsIni[k][0] + theMatrix[1][1] * ptsIni[k][1] + theMatrix[1][2];
    }

}


int display(float ptsIni[20][2], float ptsFin[20][2]) {

    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "Assignment 1", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("SimpleTransform.vertexshader", "SingleColor.fragmentshader");


    // Get a handle for our "MVP" uniform
    GLuint MatrixID = glGetUniformLocation(programID, "MVP");

    // Projection matrix : 100� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(100.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    // Or, for an ortho camera :
    //glm::mat4 Projection = glm::ortho(-10.0f,10.0f,-10.0f,10.0f,0.0f,100.0f); // In world coordinates

    // Camera matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(0, 0, 10), // Camera is at (4,3,3), in World Space
            glm::vec3(0, 0, 0), // and looks at the origin
            glm::vec3(0, 1, 0)  // Head is up (set to 0,-1,0 to look upside-down)
    );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);
    // Our ModelViewProjection : multiplication of our 3 matrices
    glm::mat4 MVP = Projection * View * Model; // Remember, matrix multiplication is the other way around



    GLfloat g_vertex_buffer_data[] = {
            ptsIni[0][0], ptsIni[0][1], 0.0f,
            ptsIni[1][0], ptsIni[1][1], 0.0f,

            ptsIni[1][0], ptsIni[1][1], 0.0f,
            ptsIni[2][0], ptsIni[2][1], 0.0f,

            ptsIni[2][0], ptsIni[2][1], 0.0f,
            ptsIni[0][0], ptsIni[0][1], 0.0f,

            -100.0f, 0.0f, 0.0f,
            100.0, 0.0f, 0.0f,

            0.0, -100.0f, 0.0f,
            0.0, 100.0f, 0.0f
    };


    GLfloat g_vertex_buffer_data2[] = {
            ptsFin[0][0], ptsFin[0][1], 0.0f,
            ptsFin[1][0], ptsFin[1][1], 0.0f,

            ptsFin[1][0], ptsFin[1][1], 0.0f,
            ptsFin[2][0], ptsFin[2][1], 0.0f,

            ptsFin[2][0], ptsFin[2][1], 0.0f,
            ptsFin[0][0], ptsFin[0][1], 0.0f,

            -100.0f, 0.0f, 0.0f,
            100.0, 0.0f, 0.0f,

            0.0, -100.0f, 0.0f,
            0.0, 100.0f, 0.0f
    };



    printf("%f %f %f\n%f %f %f\n%f %f %f\n", ptsFin[0][0], ptsFin[0][1], 0.0f,
           ptsFin[1][0], ptsFin[1][1], 0.0f,
           ptsFin[2][0], ptsFin[2][1], 0.0f);


    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);


    int counter = 0;

    time_t t = time(0);

    do {


        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Use our shader
        glUseProgram(programID);



        // Send our transformation to the currently bound shader,
        // in the "MVP" uniform
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);



        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void *) 0            // array buffer offset
        );

        // Draw the triangle !
        glDrawArrays(GL_LINES, 0, 5*2); // 5 indices starting at 0 -> 5 lines

        glDisableVertexAttribArray(0);

        // Swap buffers
        glfwSwapBuffers(window);
        glfwPollEvents();


        if ((counter == 0) && (time(0) - t > 2)) {
            glGenBuffers(1, &vertexbuffer);
            glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
            glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data2), g_vertex_buffer_data2, GL_STATIC_DRAW);
            counter++;
        }


    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    // Cleanup VBO
    glDeleteBuffers(1, &vertexbuffer);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteProgram(programID);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;


}


int main(int argc, char **argv) {

    do {
        int choice;
        printf("\n\nEnter your choice according to the following ---- >\n");
        printf("1 for translation\n");
        printf("2 for scaling\n");
        printf("3 for rotation\n");
        printf("4 for reflection\n");
        printf("5 for shearing\n");
        printf("Any other number to exit\n");
        printf("Enter your choice : ");
        scanf("%d", &choice);


        float ptsIni[20][2] = {{2.0f, 1.0f},
                                  {3.0f, 1.0f},
                                  {3.0f, 2.0f}};

        float ptsFin[20][2] = {{0.0f, 0.0f},
                               {0.0f, 0.0f},
                               {0.0f, 0.0f}};

        matrixSetIdentity(theMatrix);

        switch (choice) {

            case 1:
                float dx, dy;
                printf("Amount to translate x : ");
                scanf("%f", &dx);
                printf("Amount to translate y : ");
                scanf("%f", &dy);
                Translate(dx, dy);
                break;
            case 2:
                float sx, sy;
                printf("Scaling in x direction : ");
                scanf("%f", &sx);
                printf("Scaling in y direction : ");
                scanf("%f", &sy);
                Scale(sx, sy);
                break;
            case 3:
                float angle;
                printf("Enter reference point of rotation --- >\nx = ");
                scanf("%f", &refpt[0]);
                printf("y = ");
                scanf("%f", &refpt[1]);
                printf("Angle of rotation : ");
                scanf("%f", &angle);
                Rotate(angle);
                break;
            case 4:
                int xy;
                printf("For reflection about x-axis enter 1 or enter 2 for reflection about y-axis : ");
                scanf("%d", &xy);
                Reflect(xy);
                break;
            case 5:
                int x_y;
                float  shearValue;
                printf("For shearing about x-axis enter 1 or enter 2 for shearing about y-axis : ");
                scanf("%d", &x_y);
                printf("Enter amount of shear : ");
                scanf("%f", &shearValue);
                Shear(x_y, shearValue);
                break;
            default:
                return 0;
        }

        TransformPoints(ptsIni, ptsFin);
        display(ptsIni, ptsFin);

    } while (true);

}
