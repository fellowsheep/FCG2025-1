#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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
    GLuint texID;
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

// "Paleta" de blocos -- IDs das texturas
GLuint texIDList[10];

// Código do Vertex Shader
const GLchar *vertexShaderSource = R"glsl(
 #version 450
 layout (location = 0) in vec3 position;
 layout (location = 1) in vec2 texc;
 
 uniform mat4 view;
 uniform mat4 proj;
 uniform mat4 model;
 out vec2 tex_coord;
 void main()
 {
	tex_coord = vec2(texc.s,1.0-texc.t);
	gl_Position =  proj * view * model * vec4(position, 1.0);
 }
 )glsl";

// Código do Fragment Shader
const GLchar *fragmentShaderSource = R"glsl(
 #version 450
in vec2 tex_coord;
out vec4 color;
uniform sampler2D tex_buff;
void main()
{
	 color = texture(tex_buff,tex_coord);
}
)glsl";

// Cabeçalhos de algumas função
int loadTexture(string filePath); 

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
    //count++;
    // troca a visibilidade de um voxel selecionado
    if (key == GLFW_KEY_DELETE && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = false;
    }
    if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        grid[selecaoY][selecaoX][selecaoZ].visivel = true;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        int texID_atual = grid[selecaoY][selecaoX][selecaoZ].texID;
        grid[selecaoY][selecaoX][selecaoZ].texID = (texID_atual + 1) % 3;
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
        int texID_atual = grid[selecaoY][selecaoX][selecaoZ].texID;
        if (texID_atual < 5)
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
        texID_atual = (texID_atual + 1) % 3;
        mudouCor = true;
        grid[selecaoY][selecaoX][selecaoZ].corPos = corAtual;
        grid[selecaoY][selecaoX][selecaoZ].texID = texID_atual;
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
    // x  y    z    u       v
    // frente
	0.5,  0.5, 0.5, 1.0, 1.0, //v0 
	0.5, -0.5, 0.5, 1.0, 0.0, //v1
   -0.5, -0.5, 0.5, 0.0, 0.0, //v2
   -0.5,  0.5, 0.5, 0.0, 1.0, //v3
	0.5,  0.5, 0.5, 1.0, 1.0, //v4
   -0.5, -0.5, 0.5, 0.0, 0.0, //v5
	
	// trás
	 0.5,  0.5, -0.5, 1.0, 1.0, //v6 
	 0.5, -0.5, -0.5, 1.0, 0.0, //v7 
	-0.5, -0.5, -0.5, 0.0, 0.0, //v8 
	-0.5,  0.5, -0.5, 0.0, 1.0, //v9 
	 0.5,  0.5, -0.5, 1.0, 1.0, //v10 
	-0.5, -0.5, -0.5, 0.0, 0.0, //v11 

	// esquerda
	-0.5, -0.5,  0.5, 1.0, 1.0, //v12 
	-0.5,  0.5,  0.5, 1.0, 0.0, //v13
	-0.5, -0.5, -0.5, 0.0, 0.0, //v14 
	-0.5, -0.5, -0.5, 0.0, 1.0, //v15 
	-0.5,  0.5, -0.5, 1.0, 1.0, //v16 
	-0.5,  0.5,  0.5, 0.0, 0.0, //v17 

	//
	0.5, -0.5,  0.5, 1.0, 1.0, //v18 
	0.5,  0.5,  0.5, 1.0, 0.0, //v19
	0.5, -0.5, -0.5, 0.0, 0.0, //v20
	0.5, -0.5, -0.5, 0.0, 1.0, //v21
	0.5,  0.5, -0.5, 1.0, 1.0, //v22 
	0.5,  0.5,  0.5, 0.0, 0.0, //v23 
	
	//
	-0.5, -0.5,  0.5, 1.0, 1.0, //v24 
	 0.5, -0.5,  0.5, 1.0, 0.0, //v25
	 0.5, -0.5, -0.5, 0.0, 0.0, //v26
	 0.5, -0.5, -0.5, 0.0, 1.0, //v27
	-0.5, -0.5, -0.5, 1.0, 1.0, //v28 
	-0.5, -0.5,  0.5, 0.0, 0.0, //v29 
	
	//
	-0.5, 0.5,  0.5, 1.0, 1.0, //v30 
	 0.5, 0.5,  0.5, 1.0, 0.0, //v31 
	 0.5, 0.5, -0.5, 0.0, 0.0, //v32 
	 0.5, 0.5, -0.5, 0.0, 1.0, //v33 
	-0.5, 0.5, -0.5, 1.0, 1.0, //v34 
	-0.5, 0.5,  0.5, 0.0, 0.0  //v35
}; 

    GLuint VBO, vao;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &VBO);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // 1 atributo - coordenadas x, y, z
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)0);
    glEnableVertexAttribArray(0);

     // 2 atributo - coordenadas de textura s, t 
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid *)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    return vao;
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
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //--------------------------
    texIDList[0] = loadTexture("../assets/block_tex/empty.png");
    texIDList[2] = loadTexture("../assets/block_tex/glass.png");
    texIDList[1] = loadTexture("../assets/block_tex/moss_block.png");
    //...


    //-------------------------

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
                grid[y][x][z].texID = 0;
                grid[y][x][z].fatorEscala = 0.98f;
            }
        }
    }

    grid[selecaoY][selecaoX][selecaoZ].selecionado = true;
    


    // Ativando o primeiro buffer de textura do OpenGL
	glActiveTexture(GL_TEXTURE0);

    glUseProgram(shaderID);

	// Criando a variável uniform pra mandar a textura pro shader
	glUniform1i(glGetUniformLocation(shaderID, "tex_buff"), 0);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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
                    
                    //else{
                    //    //setColor(shaderID, colorList[grid[y][x][z].corPos]);
                    //    grid[selecaoY][selecaoX][selecaoZ].texID = 0;
                    //}
                    //se for um voxel visivel
                    if (grid[y][x][z].visivel || grid[y][x][z].selecionado)
                    {
                        float fatorEscala = grid[y][x][z].fatorEscala;
                        transformaObjeto(grid[y][x][z].pos.x, grid[y][x][z].pos.y, grid[y][x][z].pos.z, 0.0f, 0.0f, 0.0f, fatorEscala, fatorEscala, fatorEscala);
                        
                        
                        GLuint texID = grid[y][x][z].texID;
                        if(grid[y][x][z].selecionado)
                        {
                            texID = 1; //usei moss como a cor do selecionado!
                        }
                        glBindTexture(GL_TEXTURE_2D, texIDList[texID]); // Conectando ao buffer de textura
                        
                        if (texID == 0) //empty 
                            glDisable(GL_DEPTH_TEST);

                        glDrawArrays(GL_TRIANGLES, 0, 36);

                        if (texID == 0) //empty 
                        {
                            glEnable(GL_DEPTH_TEST);
                        }

                    }
                }
            }
        }

        //manda desenhar o selecionado de novo, para podermos enxergar sempre
        float fatorEscala = grid[selecaoY][selecaoX][selecaoZ].fatorEscala;
        transformaObjeto(grid[selecaoY][selecaoX][selecaoZ].pos.x, grid[selecaoY][selecaoX][selecaoZ].pos.y, grid[selecaoY][selecaoX][selecaoZ].pos.z, 0.0f, 0.0f, 0.0f, fatorEscala, fatorEscala, fatorEscala);
        glBindTexture(GL_TEXTURE_2D, texIDList[1]); // Conectando ao buffer de textura           
        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

int loadTexture(string filePath)
{
	GLuint texID;

	// Gera o identificador da textura na memória
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);

	if (data)
	{
		if (nrChannels == 3) // jpg, bmp
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		else // png
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;
	}

	stbi_image_free(data);

	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}
