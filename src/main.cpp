//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada

#include <cmath>
#include <cstdio>
#include <cstdlib>

#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>
#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"

// Constantes que definem as velocidades
#define MOVEMENT_AMOUNT 0.02f
#define ROTATION_SPEED_X 0.01f
#define ROTATION_SPEED_Y 0.004f
#define PI 3.14159265358979323846

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef std::string string;

////////////////
// ESTRUTURAS //
////////////////

// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject {
    string       name;        // Nome do objeto
    void*        first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    int          num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    vec3    bbox_max;
};

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj".
struct ObjModel {
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando modelo \"%s\"... ", filename);

        string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        printf("OK.\n");
    }
};

// Estrutura que guarda uma lista de objetos presentes
struct MapObject {
    int object_type;
    vec3 object_position;
    vec3 object_size;
    const char * obj_file_name;
};

// Estrutura que define a planta de um nível
struct Level {
    int height;
    int width;
    std::vector<std::vector<char>> plant;
};

///////////////////////////
// DECLARAÇÃO DE FUNÇÕES //
///////////////////////////

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name, int object_id, glm::mat4 model); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);
void DrawPlayer(float x, float y, float z, float angle_y, float scale);
int GetCollisionTypeInPosition(vec4 position);
Level LoadLevelFromFile(const char* filepath);
void DrawMapObjects();
void CalculatePlayerPosition();
void UpdatePlayerPosition(vec4 target_pos, vec4 target_pos_xonly, vec4 target_pos_zonly);
int GetObjectIndexInCoordinate(int x, int z);
void RegisterLevelObjects(Level level);
void RegisterObjectInMapVector(char tile_type, float x, float z);
void RegisterObjectInMap(int obj_id, vec3 obj_position, vec3 obj_size, const char * obj_file_name);
int getSmallestValueInVector(std::vector<int> vecN);

vec4 GetPlayerSpawnCoordinates(std::vector<std::vector<char>> plant);
void PrintGPUInfoInTerminal();

// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);

///////////////////////
// VARIÁVEIS GLOBAIS //
///////////////////////

std::map<string, SceneObject> g_VirtualScene;
std::stack<glm::mat4>  g_MatrixStack;
// Vetor que contém dados sobre os objetos dentro do mapa (usado para tratar colisões)
std::vector<MapObject> map_objects;

/***************************************/
/** CONSTANTES ESPECÍFICAS DE OBJETOS **/
/***************************************/

// Tipos de objetos
#define PLANE  0
#define COW    1
#define BUNNY  2
#define SPHERE 3
#define CUBE   4
#define PLAYER 5
#define WATER  6
#define DIRT   7
#define DIRTBLOCK 8

// Tipos de colisões
#define COLLISION_SOLID 0
#define COLLISION_LOCK  1
#define COLLISION_DIRTBLOCK 2
#define COLLISION_FLOOR 3
#define COLLISION_DIRT  4
#define COLLISION_WATER 5
#define COLLISION_LAVA  6
#define COLLISION_NONE -1

// Razão de proporção da janela (largura/altura).
float g_ScreenRatio = 1.0f;
int g_WindowWidth = 800, g_WindowHeight = 600;

// Ângulos de Euler
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = false;

// Variáveis de posição do jogador
vec4 player_position;
vec4 player_direction = vec4(0.0f, 0.0f, 2.0f, 0.0f);

// CÂMERA LOOKAT
float g_CameraTheta = PI; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 2.5f; // Distância da câmera para a origem

// CÂMERA FIRST PERSON
vec4 camera_position_c  = vec4(player_position[0], player_position[1] + 0.6f, player_position[2], 1.0f); // Ponto "c", centro da câmera
vec4 camera_view_vector = player_direction; // Vetor "view", sentido para onde a câmera está virada
vec4 camera_up_vector   = vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
vec4 camera_u_vector    = crossproduct(camera_up_vector, -camera_view_vector); // Vetor U da câmera (aponta para a lateral)
vec4 camera_lookat_l;

// Variável de controle da câmera
bool g_useFirstPersonCamera = false;

// Variáveis de controle do cursor
double g_LastCursorPosX, g_LastCursorPosY;

// Variável de controle do botão esquerdo do mouse.
bool g_LeftMouseButtonPressed = false;

// Variáveis de controle das teclas de movimentação
bool key_w_pressed = false;
bool key_a_pressed = false;
bool key_s_pressed = false;
bool key_d_pressed = false;

// Variável de controle de registro dos objetos (para não cadastrar mais de uma vez)
bool registered_all_objects = false;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint vertex_shader_id;
GLuint fragment_shader_id;
GLuint program_id = 0;
GLint model_uniform;
GLint view_uniform;
GLint projection_uniform;
GLint object_id_uniform;
GLint bbox_min_uniform;
GLint bbox_max_uniform;
GLint anim_timer_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

int main(int argc, char* argv[])
{
    // Inicializações
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    glfwSetErrorCallback(ErrorCallback);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    window = glfwCreateWindow(g_WindowWidth, g_WindowHeight, "The Homogeneous Cow Maze", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Funções de callback
    glfwSetKeyCallback(window, KeyCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    glfwSetCursorPosCallback(window, CursorPosCallback);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    glfwSetWindowSize(window, g_WindowWidth, g_WindowHeight); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    PrintGPUInfoInTerminal();
    LoadShadersFromFiles();

    // Carregamos imagens para serem utilizadas como textura
    LoadTextureImage("../../data/textures/ground.png"); 			// TextureImage0
    LoadTextureImage("../../data/textures/wall.png");   			// TextureImage1
    LoadTextureImage("../../data/textures/wall.png");    			// TextureImage2
    LoadTextureImage("../../data/textures/animated/water1.png");    // TextureImage3
    LoadTextureImage("../../data/textures/animated/water2.png");    // TextureImage4
    LoadTextureImage("../../data/textures/animated/water3.png");    // TextureImage5
    LoadTextureImage("../../data/textures/animated/water4.png");    // TextureImage6
    LoadTextureImage("../../data/textures/animated/water5.png");    // TextureImage7
    LoadTextureImage("../../data/textures/animated/water6.png");    // TextureImage8
    LoadTextureImage("../../data/textures/animated/water7.png");    // TextureImage9
    LoadTextureImage("../../data/textures/animated/water8.png");    // TextureImage10
    LoadTextureImage("../../data/textures/animated/water9.png");    // TextureImage11
    LoadTextureImage("../../data/textures/animated/water10.png");    // TextureImage12
    LoadTextureImage("../../data/textures/animated/water11.png");    // TextureImage13
    LoadTextureImage("../../data/textures/animated/water12.png");    // TextureImage14
    LoadTextureImage("../../data/textures/animated/water13.png");    // TextureImage15
    LoadTextureImage("../../data/textures/animated/water14.png");    // TextureImage16
    LoadTextureImage("../../data/textures/animated/water15.png");    // TextureImage17
    LoadTextureImage("../../data/textures/animated/water16.png");    // TextureImage18
    LoadTextureImage("../../data/textures/dirt.png");    			 // TextureImage19
    LoadTextureImage("../../data/textures/dirtblock.png");    		 // TextureImage20

    // Variável de controle de animação
    int curr_anim_tile = 0;
    int anim_timer = 0;
    #define ANIMATION_SPEED 10

    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel spheremodel("../../data/objects/sphere.obj");
    ComputeNormals(&spheremodel);
    BuildTrianglesAndAddToVirtualScene(&spheremodel);

    ObjModel bunnymodel("../../data/objects/bunny.obj");
    ComputeNormals(&bunnymodel);
    BuildTrianglesAndAddToVirtualScene(&bunnymodel);

    ObjModel planemodel("../../data/objects/plane.obj");
    ComputeNormals(&planemodel);
    BuildTrianglesAndAddToVirtualScene(&planemodel);

    ObjModel cubemodel("../../data/objects/cube.obj");
    ComputeNormals(&cubemodel);
    BuildTrianglesAndAddToVirtualScene(&cubemodel);

    ObjModel cowmodel("../../data/objects/cow.obj");
    ComputeNormals(&cowmodel);
    BuildTrianglesAndAddToVirtualScene(&cowmodel);

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    // Carrega nível um
    Level level_one = LoadLevelFromFile("../../data/levels/1");
    RegisterLevelObjects(level_one);
    player_position = GetPlayerSpawnCoordinates(level_one.plant);
    camera_lookat_l = player_position;

    // Ficamos em loop, renderizando, até que o usuário feche a janela
    while (!glfwWindowShouldClose(window))
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);

        // Movimentação do personagem
        CalculatePlayerPosition();

        // Controle do tipo de câmera
        if (g_useFirstPersonCamera) {
            camera_position_c = player_position;
        }
        else {
            float r = g_CameraDistance;
            float y = r*sin(g_CameraPhi);
            float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
            float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

            camera_position_c  = vec4(x+player_position[0],y,z+player_position[2],1.0f); // Ponto "c", centro da câmera
            camera_lookat_l    = player_position; // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
            camera_u_vector    = crossproduct(camera_up_vector, -camera_view_vector);
        }

        player_direction = vec4(camera_view_vector[0] + 0.01f, 0.0f, camera_view_vector[2] + 0.01f, 0.0f);

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        glm::mat4 projection;
		//glm::mat4 model = Matrix_Identity();
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -20.0f; // Posição do "far plane"

        // Projeção perspectiva
        float field_of_view = 3.141592 / 3.0f;
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo.
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        /////////////
        // JOGADOR //
        /////////////

        float vecangle = acos(dotproduct(player_direction, vec4(1.0f,0.0f,0.0f,0.0f))/norm(player_direction));
        if (player_direction[2] > 0.0f)
            vecangle = -vecangle;
        DrawPlayer(player_position[0], player_position[1] - 0.3f, player_position[2], vecangle - PI/2, 0.3f);

        ///////////////////
        // RESTO DO MAPA //
        ///////////////////

        DrawMapObjects();

        // Animação dos tiles (todos são animados em simetria)
        // CASO DESEJA-SE ANIMÁ-LOS DE FORMA INDEPENDENTE, deve-se
        // copiar este trecho para cada switch na função de renderização.
        anim_timer++;
        if (anim_timer >= ANIMATION_SPEED) {
        	anim_timer = 0;
        	curr_anim_tile++;
        	if (curr_anim_tile == 16)
        		curr_anim_tile = 0;
        }
		glUniform1i(anim_timer_uniform, curr_anim_tile);

        if (g_ShowInfoText) {
            TextRendering_ShowEulerAngles(window);
            TextRendering_ShowProjection(window);
            TextRendering_ShowFramesPerSecond(window);
        }

        glfwSwapBuffers(window);
        // Verificação de eventos
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que registra os objetos do nível com base na sua planta
void RegisterLevelObjects(Level level) {
    float center_x = (level.width-1)/2.0f;
    float center_z = (level.height-1)/2.0f;

    for(int line = 0; line < level.height; line++) {
        for(int col = 0; col < level.width; col++) {
            char current_tile = level.plant[line][col];
            float x = -(center_x - col);
            float z = -(center_z - line);

            RegisterObjectInMapVector(current_tile, x, z);
        }
    }
}

// Função que registra um objeto em dada posição do mapa
// CASO SE QUEIRA ADICIONAR NOVOS OBJETOS, DEVE-SE FAZÊ-LO AQUI
void RegisterObjectInMapVector(char tile_type, float x, float z) {
    /* Propriedades de objetos (deslocamento, tamanho, etc) */
    // Dimensões
    vec3 cube_size = vec3(1.0f, 1.0f, 1.0f);
    float cube_vertical_shift = -0.5f;

    // Tile plano
    vec3 tile_size = vec3(1.0f, 0.0f, 1.0f);
    float floor_shift = -1.0f;

    /* Adicione novos tipos de objetos abaixo */
    switch(tile_type) {
    // Cubo
    case 'B': {
        RegisterObjectInMap(CUBE, vec3(x, cube_vertical_shift, z), cube_size, "cube");
        break;
    }

    // Água
    case 'W':{
        RegisterObjectInMap(WATER, vec3(x, floor_shift, z), tile_size, "plane");
        break;
    }

    // Terra
    case 'd':{
        RegisterObjectInMap(DIRT, vec3(x, floor_shift, z), tile_size, "plane");
        break;
    }

    // Bloco de terra
    case 'D':{
        RegisterObjectInMap(DIRTBLOCK, vec3(x, cube_vertical_shift, z), cube_size, "cube");
        break;
    }

    // Jogador e piso
    case 'P':
    case 'F':{
        RegisterObjectInMap(PLANE, vec3(x, floor_shift, z), tile_size, "plane");
        break;
    }

    default:
        break;
    }
}

// Função que adiciona um objeto ao mapa
void RegisterObjectInMap(int obj_id, vec3 obj_position, vec3 obj_size, const char * obj_file_name) {
    MapObject new_object;
    new_object.object_type = obj_id;
    new_object.object_size = obj_size;
    new_object.object_position = obj_position;
    new_object.obj_file_name = obj_file_name;
    map_objects.push_back(new_object);
}

// Função que desenha os objetos na cena (com base no vetor de objetos)
void DrawMapObjects() {
    for(unsigned int i = 0; i < map_objects.size(); i++) {
        MapObject current_object = map_objects[i];
        glm::mat4 model = Matrix_Translate(current_object.object_position.x, current_object.object_position.y, current_object.object_position.z);
        DrawVirtualObject(current_object.obj_file_name, current_object.object_type, model);
    }
}

// Função que desenha o jogador usando transformações hierárquicas
// Desenha na posição dada, com escala dada e ângulo de rotação dado
void DrawPlayer(float x, float y, float z, float angle_y, float scale) {
    glm::mat4 model = Matrix_Identity();

    model = Matrix_Translate(x, y, z) * Matrix_Rotate_Y(angle_y);
    PushMatrix(model);
        model = model * Matrix_Scale(0.8f * scale, 1.1f * scale, 0.2f * scale);
        DrawVirtualObject("cube", PLAYER, model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(-0.55f * scale, 0.05f * scale, 0.0f * scale); // Translação do braço direito
        PushMatrix(model);
            model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do braço direito
            DrawVirtualObject("cube", PLAYER, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.75f * scale, 0.0f * scale); // Translação do antebraço direito
            PushMatrix(model);
                model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do antebraço direito
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.45f * scale, 0.0f * scale); // Translação da mão direita
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.2f * scale);
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(0.55f * scale, 0.05f * scale, 0.0f * scale); // Translação para o braço esquerdo
        PushMatrix(model);
            model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do braço esquerdo
            DrawVirtualObject("cube", PLAYER, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.75f * scale, 0.0f * scale); // Translação do antebraço esquerdo
            PushMatrix(model);
                model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do antebraço esquerdo
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.45f * scale, 0.0f * scale); // Translação da mão esquerda
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.2f * scale);
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(-0.2f * scale, -1.0f * scale, 0.0f * scale); // Translação para a perna direita
        PushMatrix(model);
            model = model * Matrix_Scale(0.3f * scale, 0.8f * scale, 0.3f * scale); // Escalamento da coxa direita
            DrawVirtualObject("cube", PLAYER, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.85f * scale, 0.0f * scale); // Translação para a canela direita
            PushMatrix(model);
                model = model * Matrix_Scale(0.25f * scale, 0.8f * scale, 0.25f * scale); // Escalamento da canela direita
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.5f * scale, 0.1f * scale); // Translação para o pé direito
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.4f * scale); // Escalamento do pé direito
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(0.2f * scale, -1.0f * scale, 0.0f * scale); // Translação para a perna esquerda
        PushMatrix(model);
            model = model * Matrix_Scale(0.3f * scale, 0.8f * scale, 0.3f * scale); // Escalamento da coxa esquerda
            DrawVirtualObject("cube", PLAYER, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.85f * scale, 0.0f * scale); // Translação para a canela esquerda
            PushMatrix(model);
                model = model * Matrix_Scale(0.25f * scale, 0.8f * scale, 0.25f * scale); // Escalamento da canela esquerda
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.5f * scale, 0.1f * scale); // Translação para o pé esquerdo
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.4f * scale); // Escalamento do pé esquerdo
                DrawVirtualObject("cube", PLAYER, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    model = model * Matrix_Rotate_Z(3.14);
    model = model * Matrix_Translate(0.0f * scale, -0.75f * scale, 0.0f * scale); // Translação para a cabeça
    model = model * Matrix_Scale(0.35f * scale, 0.35f * scale, 0.35f * scale); // Escalamento da cabeça
    DrawVirtualObject("cube", PLAYER, model);
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name, int object_id, glm::mat4 model) {
    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(object_id_uniform, object_id);

    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que determina o tipo de colisão em um dado ponto
int GetCollisionTypeInPosition(vec4 position) {
    unsigned int obj_index = 0;
    std::vector<int> collision_types;

    while(obj_index < map_objects.size()) {
        vec3 obj_position = map_objects[obj_index].object_position;
        vec3 obj_size = map_objects[obj_index].object_size;
        vec3 obj_start = obj_position - obj_size / 2.0f;
        vec3 obj_end = obj_position + obj_size / 2.0f;

        float solid_tol = 0.25f;
        float floor_tol = 0.05f;
        float tol;

        // Configura o valor de tolerância com base no bloco
        switch(map_objects[obj_index].object_type) {
    	case CUBE:
    	case DIRTBLOCK: {
    		tol = solid_tol;
    		break;
    	}
    	case DIRT:
    	case WATER:
    	case PLANE:
    	default: {
    		tol = floor_tol;
    		break;
    	}
        }

        bool is_x_in_collision_interval = (position.x <= obj_end.x + tol && position.x >= obj_start.x - tol);
        //bool is_y_in_collision_interval = (position.y <= obj_end.y && position.y >= obj_start.y);
        bool is_z_in_collision_interval = (position.z <= obj_end.z + tol && position.z >= obj_start.z - tol);

        if (is_x_in_collision_interval /*&& is_y_in_collision_interval*/ && is_z_in_collision_interval) {
            switch(map_objects[obj_index].object_type) {
	        case CUBE: {
	            collision_types.push_back(COLLISION_SOLID);
	            break;
	        }
	        case WATER: {
	        	collision_types.push_back(COLLISION_WATER);
	            break;
	        }
	        case DIRT: {
	        	collision_types.push_back(COLLISION_DIRT);
	            break;
	        }
	        case DIRTBLOCK: {
	        	collision_types.push_back(COLLISION_DIRTBLOCK);
	            break;
	        }
	        case PLANE: {
	        	collision_types.push_back(COLLISION_FLOOR);
	            break;
	        }
	        default:
	        	break;
	        }
        }

        obj_index++;
    }

    if (collision_types.size() == 0)
    	return COLLISION_FLOOR;
    else
    	return getSmallestValueInVector(collision_types);
}

// Função que pega o menor valor em um vetor
int getSmallestValueInVector(std::vector<int> vecN) {
	int minimum = 900;

	for (unsigned int i = 0; i < vecN.size(); i++) {
		if (vecN[i] < minimum)
			minimum = vecN[i];
	}

	return minimum;
}

void CalculatePlayerPosition() {
    vec4 target_position, target_position_xonly, target_position_zonly;

    if (key_w_pressed) {
        target_position = player_position + MOVEMENT_AMOUNT * vec4(player_direction[0], 0.0f, player_direction[2], 0.0f);
        target_position_xonly = player_position + MOVEMENT_AMOUNT * vec4(player_direction[0], 0.0f, 0.0f, 0.0f);
        target_position_zonly = player_position + MOVEMENT_AMOUNT * vec4(0.0f, 0.0f, player_direction[2], 0.0f);
        UpdatePlayerPosition(target_position, target_position_xonly, target_position_zonly);
    }
    if (key_s_pressed) {
        target_position = player_position - MOVEMENT_AMOUNT * vec4(player_direction[0], 0.0f, player_direction[2], 0.0f);
        target_position_xonly = player_position - MOVEMENT_AMOUNT * vec4(player_direction[0], 0.0f, 0.0f, 0.0f);
        target_position_zonly = player_position - MOVEMENT_AMOUNT * vec4(0.0f, 0.0f, player_direction[2], 0.0f);
        UpdatePlayerPosition(target_position, target_position_xonly, target_position_zonly);
    }
    if (key_a_pressed) {
        target_position = player_position - MOVEMENT_AMOUNT * camera_u_vector;
        target_position_xonly = player_position - MOVEMENT_AMOUNT * vec4(camera_u_vector[0], 0.0f, 0.0f, 0.0f);
        target_position_zonly = player_position - MOVEMENT_AMOUNT * vec4(0.0f, 0.0f, camera_u_vector[2], 0.0f);
        UpdatePlayerPosition(target_position, target_position_xonly, target_position_zonly);
    }
    if (key_d_pressed) {
        target_position = player_position + MOVEMENT_AMOUNT * camera_u_vector;
        target_position_xonly = player_position + MOVEMENT_AMOUNT * vec4(camera_u_vector[0], 0.0f, 0.0f, 0.0f);
        target_position_zonly = player_position + MOVEMENT_AMOUNT * vec4(0.0f, 0.0f, camera_u_vector[2], 0.0f);
        UpdatePlayerPosition(target_position, target_position_xonly, target_position_zonly);
    }
}

void UpdatePlayerPosition(vec4 target_pos, vec4 target_pos_xonly, vec4 target_pos_zonly) {
    int collision_type = GetCollisionTypeInPosition(target_pos);
    int collision_type_x = GetCollisionTypeInPosition(target_pos_xonly);
    int collision_type_z = GetCollisionTypeInPosition(target_pos_zonly);

    if (collision_type == COLLISION_SOLID) {
        if (!(collision_type_x == COLLISION_SOLID)) {
            collision_type = collision_type_x;
            target_pos = target_pos_xonly;
        }
        else if (!(collision_type_x == COLLISION_SOLID)) {
            collision_type = collision_type_z;
            target_pos = target_pos_zonly;
        }
    }

    switch(collision_type) {
    case COLLISION_WATER: {
        player_position = target_pos;
        // death animation
    }
    case COLLISION_DIRT: {
        player_position = target_pos;
        int x = round(player_position.x);
        int z = round(player_position.z);
        int target_object = GetObjectIndexInCoordinate(x, z);
        map_objects[target_object].object_type = PLANE;
    }
    default:
        break;
    }

}

int GetObjectIndexInCoordinate(int x, int z) {
    for(unsigned int i = 0; i < map_objects.size(); i++) {
        if ((map_objects[i].object_position.x == x) && (map_objects[i].object_position.z == z))
            return i;
    }
    return -1;
}

// Função que encontra as coordenadas do spawn do jogador na planta do mapa
vec4 GetPlayerSpawnCoordinates(std::vector<std::vector<char>> plant) {
    int map_height = plant.size();
    int map_width = plant[0].size();
    float center_x = (map_width-1)/2.0f;
    float center_z = (map_height-1)/2.0f;

    for(int line = 0; line < map_height; line++) {
        for(int col = 0; col < map_width; col++) {
            if(plant[line][col] == 'P') {
                float x = -(center_x - col);
                float z = -(center_z - line);

                return vec4(x, 0.0f, z, 1.0f);
            }
        }
    }

    // Caso o spawn não seja encontrado, spawna o player no centro por padrão
    return vec4(0.5f, 0.0f, 0.5f, 1.0f);
}

// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M) {
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M) {
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model) {
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gourad, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<vec4> vertex_normals(num_vertices, vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = vec4(vx,vy,vz,1.0);
            }

            const vec4  a = vertices[0];
            const vec4  b = vertices[1];
            const vec4  c = vertices[2];

            const vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model) {
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        vec3 bbox_min = vec3(maxval,maxval,maxval);
        vec3 bbox_max = vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if ( model->attrib.normals.size() >= (size_t)3*idx.normal_index )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( model->attrib.texcoords.size() >= (size_t)2*idx.texcoord_index )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = (void*)first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 2)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!

    glBindVertexArray(0);
}

// Função que carrega um nível a partir de um arquivo (parser)
Level LoadLevelFromFile(const char* filepath) {
    Level loaded_level;

    printf("Carregando nivel \"%s\"... ", filepath);

    std::ifstream level_file;
    level_file.open(filepath);
    if (!level_file.is_open()) {
        throw std::runtime_error("Erro ao abrir arquivo.");
    }
    else {
        try {
            // Salva largura e altura na estrutura do nível.
            string width, height;
            getline(level_file, width);
            getline(level_file, height);
            loaded_level.width = atoi(width.c_str());
            loaded_level.height = atoi(height.c_str());

            if (loaded_level.width < 0 || loaded_level.height < 0) {
                throw std::runtime_error("arquivo inválido.");
            }

            // Salva a planta do nível
            int line=0, col=0;
            while (line < loaded_level.height) {
                string file_line;
                getline(level_file, file_line);
                std::vector<char> map_line;

                while (col < loaded_level.width) {
                    map_line.push_back(file_line[col]);
                    col++;
                }

                loaded_level.plant.push_back(map_line);

                // Reseta variáveis para a próxima iteração
                col = 0;
                map_line.clear();

                line++;
            }

            printf("OK!\n");
            level_file.close();
            return loaded_level;
        }
        catch (...) {
            throw std::runtime_error("arquivo inválido.");
        }
    }
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename) {
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slide 160 do documento "Aula_20_e_21_Mapeamento_de_Texturas.pdf"
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura. Falaremos sobre eles em uma próxima aula.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização.
void LoadShadersFromFiles() {
    vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( program_id != 0 )
        glDeleteProgram(program_id);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    program_id = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    model_uniform           = glGetUniformLocation(program_id, "model"); // Variável da matriz "model"
    view_uniform            = glGetUniformLocation(program_id, "view"); // Variável da matriz "view" em shader_vertex.glsl
    projection_uniform      = glGetUniformLocation(program_id, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    object_id_uniform       = glGetUniformLocation(program_id, "object_id"); // Variável "object_id" em shader_fragment.glsl
    bbox_min_uniform        = glGetUniformLocation(program_id, "bbox_min");
    bbox_max_uniform        = glGetUniformLocation(program_id, "bbox_max");
    anim_timer_uniform      = glGetUniformLocation(program_id, "anim_timer");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage5"), 5);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage6"), 6);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage7"), 7);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage8"), 8);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage9"), 9);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage10"), 10);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage11"), 11);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage12"), 12);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage13"), 13);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage14"), 14);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage15"), 15);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage16"), 16);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage17"), 17);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage18"), 18);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage19"), 19);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage20"), 20);

    glUseProgram(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename) {
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename) {
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id) {
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id) {
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Callback de resizing
void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    g_ScreenRatio = (float)width / height;
}

// Callback do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        g_LeftMouseButtonPressed = false;
    }
}

// Callback de movimentação do cursor
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimentou desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.

    // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
    double dx = xpos - g_LastCursorPosX;
    double dy = ypos - g_LastCursorPosY;

    if (g_useFirstPersonCamera) {
        // Agora atualizamos o vetor view e o vetor u conforme a variação do cursor
        // Girar na horizontal (olhar pros lados) = rotação em torno do eixo UP
        camera_view_vector = Matrix_Rotate(ROTATION_SPEED_X * -dx, camera_up_vector) * camera_view_vector;
        camera_u_vector = Matrix_Rotate(ROTATION_SPEED_X * -dx, camera_up_vector) * camera_u_vector;

        // Girar na vertical (olhar pra cima) = rotação em torno do eixo U
        // Somente rotaciona se não atingiu os limites (cima/baixo)
        vec4 rotated_camera = Matrix_Rotate(ROTATION_SPEED_Y * -dy, camera_u_vector) * camera_view_vector;;
        if ((rotated_camera[1] >= -PI/1.35) && (rotated_camera[1] <= PI/1.35))
            camera_view_vector = rotated_camera;
    } else {
      // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = PI/2;
        float phimin = -phimax;

        if (g_CameraPhi >= phimax)
            g_CameraPhi = phimax - 0.01f;

        if (g_CameraPhi <= phimin)
            g_CameraPhi = phimin + 0.01f;
    }

    //SetCursorPos(g_WindowWidth / 2, g_WindowHeight / 2);

    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
    g_LastCursorPosX = xpos;
    g_LastCursorPosY = ypos;


}

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod) {
    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        g_useFirstPersonCamera = !(g_useFirstPersonCamera);

    // Verifica se as teclas de movimentação foram pressionadas/soltas
    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        key_w_pressed = true;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        key_s_pressed = true;
    }

    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        key_a_pressed = true;
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        key_d_pressed = true;
    }

    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    {
        key_w_pressed = false;
    }

    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    {
        key_s_pressed = false;
    }

    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        key_a_pressed = false;
    }

    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        key_d_pressed = false;
    }
}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description) {
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

void PrintGPUInfoInTerminal() {
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, vec4 p_model) {
    if ( !g_ShowInfoText )
        return;

    vec4 p_world = model*p_model;
    vec4 p_camera = view*p_world;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     World", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     Camera", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                   NDC", -1.0f, 1.0f-13*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-14*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window) {
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window) {
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window) {
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model) {
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<string, string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<string, string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :
