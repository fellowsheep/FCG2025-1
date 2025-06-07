#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;

int count = 0;

// Dimensões da janela
const GLuint WIDTH = 800, HEIGHT = 600;

// Variáveis globais de controle da câmera (posição, direção e orientação)
glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 20.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f;
float pitch = 0.0f;
bool firstMouse = true;
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
float fov = 45.0f;

// Controle de tempo entre frames
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// IDs de shader e VAO
GLuint shaderID, VAO;
GLFWwindow *window;

struct Voxel
{
    glm::vec3 pos;
    float fatorEscala;
    bool visivel = true, selecionado = false;
    int corPos;
};

int selecaoX, selecaoY, selecaoZ;
const int TAM = 10;
Voxel grid[TAM][TAM][TAM];

glm::vec4 colorList[] = {
    {0.5f, 0.5f, 0.5f, 0.5f}, // cinza     0   -- reservado para a interface
    {1.0f, 0.0f, 0.0f, 1.0f}, // vermelho  1
    {0.0f, 1.0f, 0.0f, 1.0f}, // verde     2
    {0.0f, 0.0f, 1.0f, 1.0f}, // azul      3
    {1.0f, 1.0f, 0.0f, 1.0f}, // amarelo   4
    {1.0f, 0.0f, 1.0f, 1.0f}, // magenta   5
    {0.0f, 1.0f, 1.0f, 1.0f}, // ciano     6
    {1.0f, 1.0f, 1.0f, 1.0f}, // branco    7
    {0.0f, 0.0f, 0.0f, 1.0f}, // preto     8  
};

// Código do Vertex Shader
const GLchar *vertexShaderSource = R"glsl(
    #version 450
    layout(location = 0) in vec3 position;
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 proj;
    void main() {
        gl_Position = proj * view * model * vec4(position, 1.0);
    }
)glsl";

// Código do Fragment Shader
const GLchar *fragmentShaderSource = R"glsl(
    #version 450
    uniform vec4 uColor;
    out vec4 color;
    void main() {
        color = uColor;
    }
)glsl";

// Atualiza o viewport ao redimensionar a janela
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// Callback para movimentação do mouse — controla rotação da câmera
void mouse_callback(GLFWwindow *window, double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.05f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);

    glm::vec3 right = glm::normalize(glm::cross(cameraFront, glm::vec3(0.0, 1.0, 0.0)));
    cameraUp = glm::normalize(glm::cross(right, cameraFront));
}

// Callback de scroll — altera o FOV (zoom)
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
    if (fov >= 1.0f && fov <= 120.0f)
        fov -= yoffset;
    if (fov <= 1.0f)
        fov = 1.0f;
    if (fov >= 120.0f)
        fov = 120.0f;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{

    // printf("Contador atual: %d\n",count);
    count++;
    // troca a visibilidade de um voxel selecionado
    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = false;
    }
    if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = true;
    }

    // testa a seleção do voxel na grid
    // Troca a cor do antigo voxel selecionado para cinza

    bool mudouSelecao = false;

    if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
    {
        if (selecaoX + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoX++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
    {
        if (selecaoX - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoX--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }

    if (key == GLFW_KEY_UP && action == GLFW_PRESS)
    {
        if (selecaoY + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoY++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
    {
        if (selecaoY - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoY--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }

    if (key == GLFW_KEY_PAGE_UP && action == GLFW_PRESS)
    {
        if (selecaoZ + 1 < TAM)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoZ++;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }
    if (key == GLFW_KEY_PAGE_DOWN && action == GLFW_PRESS)
    {
        if (selecaoZ - 1 >= 0)
        {
            grid[selecaoY][selecaoX][selecaoZ].selecionado = false;
            selecaoZ--;
            mudouSelecao = true;
            grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
        }
    }

    // muda a cor do voxel
    bool mudouCor = false;
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        // printf("Entrou no C\n");
        int corAtual = grid[selecaoY][selecaoX][selecaoZ].corPos;
        if (corAtual < 5)
        {
            corAtual++;
            printf("Troquei a cor para %d\n", corAtual);
        }
        else
        {
            // printf("Volta pra cor inicial\n");
            corAtual = 0;
            printf("Troquei a cor para %d\n", corAtual);
        }
        mudouCor = true;
        grid[selecaoY][selecaoX][selecaoZ].corPos = corAtual;
    }

    // printf("\n\n\n");
}

// Processa as teclas pressionadas para movimentar a câmera no espaço 3D
void processInput(GLFWwindow *window)
{
    float cameraSpeed = 5.0f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
}

// Define a matriz de visualização usando a posição e direção da câmera
void especificaVisualizacao()
{
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    GLuint loc = glGetUniformLocation(shaderID, "view");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(view));
}

// Define a matriz de projeção perspectiva com base no FOV
void especificaProjecao()
{
    glm::mat4 proj = glm::perspective(glm::radians(fov), (float)WIDTH / HEIGHT, 0.1f, 100.0f);
    GLuint loc = glGetUniformLocation(shaderID, "proj");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(proj));
}

void transformaObjeto(float xpos, float ypos, float zpos, float xrot, float yrot, float zrot, float sx, float sy, float sz)
{
    glm::mat4 transform = glm::mat4(1.0f); // matriz identidade

    // especifica as transformações sobre o objeto - model
    transform = glm::translate(transform, glm::vec3(xpos, ypos, zpos));

    transform = glm::rotate(transform, glm::radians(xrot), glm::vec3(1, 0, 0));
    transform = glm::rotate(transform, glm::radians(yrot), glm::vec3(0, 1, 0));
    transform = glm::rotate(transform, glm::radians(zrot), glm::vec3(0, 0, 1));

    transform = glm::scale(transform, glm::vec3(sx, sy, sz));

    // Envia os dados para o shader
    GLuint loc = glGetUniformLocation(shaderID, "model");
    glUniformMatrix4fv(loc, 1, GL_FALSE, glm::value_ptr(transform));
}

// Compila shaders e cria o programa de shader
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
        cout << "Vertex Shader error:\n"
             << infoLog << endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        cout << "Fragment Shader error:\n"
             << infoLog << endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        cout << "Shader Program Linking error:\n"
             << infoLog << endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Cria o VAO com os vértices e cores do cubo 3D
GLuint setupGeometry()
{
    GLfloat vertices[] = {
        // frente
        0.5, 0.5, 0.5, 0.5, -0.5, 0.5, -0.5, -0.5, 0.5,
        -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5, 0.5,
        // trás
        0.5, 0.5, -0.5, 0.5, -0.5, -0.5, -0.5, -0.5, -0.5,
        -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5, -0.5,
        // esquerda
        -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, -0.5,
        -0.5, -0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5,
        // direita
        0.5, -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5, -0.5,
        0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, 0.5,
        // baixo
        -0.5, -0.5, 0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5,
        0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, -0.5, 0.5,
        // cima
        -0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, -0.5,
        0.5, 0.5, -0.5, -0.5, 0.5, -0.5, -0.5, 0.5, 0.5};

    GLuint VBO, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLvoid *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
}

void setColor(GLuint shaderID, glm::vec4 cor)
{
    GLint loc = glGetUniformLocation(shaderID, "uColor");
    glUniform4f(loc, cor.r, cor.g, cor.b, cor.a);
}

// Função principal da aplicação
int main()
{
    glfwInit();
    window = glfwCreateWindow(WIDTH, HEIGHT, "Camera Cube", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

    shaderID = setupShader();
    VAO = setupGeometry();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float xPos, yPos, zPos;

    selecaoX = 0;
    selecaoY = 0;
    selecaoZ = TAM - 1;

    for (int y = 0, yPos = -TAM / 2; y < TAM; y++, yPos += 1.0f)
    {
        // printf("Camada: %d\n", y);
        for (int x = 0, xPos = -TAM / 2; x < TAM; x++, xPos += 1.0f)
        {

            for (int z = 0, zPos = -TAM / 2; z < TAM; z++, zPos += 1.0f)
            {
                grid[y][x][z].pos = glm::vec3(xPos, yPos, zPos);
                grid[y][x][z].corPos = 0;
                grid[y][x][z].fatorEscala = 0.98f;
            }
        }
    }

    grid[selecaoY][selecaoX][selecaoZ].selecionado = true;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderID);

        especificaVisualizacao();
        especificaProjecao();

        // renderizar os objetos
        glBindVertexArray(VAO);

        // navega na grid tridimensional pelos seus índices
        for (int x = 0; x < TAM; x++)
        {
            for (int y = 0; y < TAM; y++)
            {
                for (int z = 0; z < TAM; z++)
                {
                    if(grid[y][x][z].selecionado){ //se estiver selecionado, da um brilho no objeto
                        setColor(shaderID, colorList[grid[y][x][z].corPos] +0.3f);
                    }
                    else{
                        setColor(shaderID, colorList[grid[y][x][z].corPos]);
                    }
                    //se for um voxel visivel
                    if (grid[y][x][z].visivel || grid[y][x][z].selecionado)
                    {
                        float fatorEscala = grid[y][x][z].fatorEscala;
                        transformaObjeto(grid[y][x][z].pos.x, grid[y][x][z].pos.y, grid[y][x][z].pos.z, 0.0f, 0.0f, 0.0f, fatorEscala, fatorEscala, fatorEscala);
                        glDrawArrays(GL_TRIANGLES, 0, 36);
                    }
                }
            }
        }
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}