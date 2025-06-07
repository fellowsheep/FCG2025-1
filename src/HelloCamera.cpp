#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

const GLuint WIDTH = 800, HEIGHT = 600;

// Vertex Shader
const GLchar* vertexShaderSource = R"glsl(
#version 450
layout(location = 0) in vec3 vertex_posicao;
layout(location = 1) in vec3 vertex_cores;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
out vec3 cores;
void main() {
    cores = vertex_cores;
    gl_Position = proj * view * model * vec4(vertex_posicao, 1.0);
}
)glsl";

// Fragment Shader
const GLchar* fragmentShaderSource = R"glsl(
#version 450
in vec3 cores;
out vec4 frag_colour;
void main() {
    frag_colour = vec4(cores, 1.0);
}
)glsl";

// Camera
float cam_speed = 1.0f;
float cam_yaw_speed = 10.0f;
glm::vec3 cam_pos(0.0f, 0.0f, 2.0f);
float cam_yaw = 0.0f;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

GLuint shaderID, VAO;
GLFWwindow* window;

// Prototyping
void processInput(GLFWwindow* window);
GLuint setupShader();
GLuint setupGeometry();
void transformaObjeto();
void especificaVisualizacao();
void especificaVisualizacaoLookAt();
void especificaProjecao();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

int main()
{
    glfwInit();
    window = glfwCreateWindow(WIDTH, HEIGHT, "Camera + Projecao", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    shaderID = setupShader();
    VAO = setupGeometry();

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderID);

        transformaObjeto();
        especificaVisualizacao();
        especificaProjecao();

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

GLuint setupShader()
{
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        cout << "Vertex Shader error:\n" << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cout << "Fragment Shader error:\n" << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cout << "Shader Program Linking error:\n" << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint setupGeometry()
{
    GLfloat vertices[] = {
        // posições         // cores
        0.5f,  0.5f, 0.0f,  1, 1, 0,
        0.5f, -0.5f, 0.0f,  0, 1, 1,
       -0.5f, -0.5f, 0.0f,  1, 0, 1,

       -0.5f,  0.5f, 0.0f,  0, 1, 1,
        0.5f,  0.5f, 0.0f,  1, 1, 0,
       -0.5f, -0.5f, 0.0f,  1, 0, 1
    };

    GLuint VBO, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void transformaObjeto()
{
    glm::mat4 translacao = glm::translate(glm::mat4(1.0f), glm::vec3(0.5f, -0.5f, 0.0f));
    glm::mat4 rotacao = glm::rotate(glm::mat4(1.0f), glm::radians(30.0f), glm::vec3(0, 0, 1));
    glm::mat4 model = translacao * rotacao;

    GLuint loc = glGetUniformLocation(shaderID, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(model));
}

void especificaVisualizacao()
{
    glm::mat4 view = glm::rotate(glm::mat4(1.0f), glm::radians(-cam_yaw), glm::vec3(0, 1, 0));
    view = glm::translate(view, -cam_pos);

    GLuint loc = glGetUniformLocation(shaderID, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
}

void especificaVisualizacaoLookAt()
{
    glm::vec3 cameraTarget = cam_pos + glm::vec3(
                                           sin(glm::radians(cam_yaw)),
                                           0.0f,
                                           -cos(glm::radians(cam_yaw)));
    glm::mat4 view = glm::lookAt(cam_pos, cameraTarget, glm::vec3(0, 1, 0));
    GLuint loc = glGetUniformLocation(shaderID, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
}

void especificaProjecao()
{
    glm::mat4 proj = glm::perspective(glm::radians(89.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    //glm::mat4 proj = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);
    GLuint loc = glGetUniformLocation(shaderID, "proj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));
}

void processInput(GLFWwindow* window)
{
    float velocity = cam_speed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cam_pos.x -= velocity;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cam_pos.x += velocity;
    if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
        cam_pos.y += velocity;
    if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
        cam_pos.y -= velocity;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cam_pos.z -= velocity;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cam_pos.z += velocity;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cam_yaw += cam_yaw_speed * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cam_yaw -= cam_yaw_speed * deltaTime;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}
