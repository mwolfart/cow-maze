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
#include <initializer_list>

// SFML: Músicas e Sons
#include <SFML/Audio.hpp>

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

#define PI 3.14159265358979323846

typedef glm::vec3 vec3;
typedef glm::vec4 vec4;
typedef std::string string;
typedef std::vector<int> vecInt;

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
    vec4 object_position;
    vec3 object_size;
    vec3 model_size;
    int direction;
    float gravity;
    const char * obj_file_name;
};

// Estrutura que define a planta de um nível
struct Level {
	int cow_no;
    int time;
    int theme;
    int height;
    int width;
    std::vector<std::vector<string>> plant;
};

// CPU representation of a particle
struct Particle {
	vec4 position;
	float speed;
	vec3 color;
	float size;
	float life; // Remaining life of the particle. if < 0 : dead and unused.
};

struct InventoryKeys {
    int red, green, blue, yellow;
};

struct Inventory {
    InventoryKeys keys;
    int cows;
};

///////////////////////////
// DECLARAÇÃO DE FUNÇÕES //
///////////////////////////

// Auxiliares simples
float MaxFloat2(float a, float b);
vec4 VectorSetHomogeneous(vec3 nonHomogVector, bool isVectorPosVector);
constexpr unsigned int string2int(const char* str, int h = 0);

// Renderização de telas
int RenderMainMenu(GLFWwindow* window);
int RenderLevelSelection(GLFWwindow* window);
vec4 AdjustFPSCamera(vec4 position);
int RenderLevel(int level_number, GLFWwindow* window);
int ShowDeathMessage(GLFWwindow* window, const char* message);
void ShowInventory(GLFWwindow* window, int level_time);

// Controle de um nível
void ClearInventory();
int GetCowMotherPosition();
void RegisterLevelObjects(Level level);
void RegisterFloor(float x, float z, int theme);
void RegisterObjectInMapVector(string tile_type, float x, float z, int theme);
void RegisterObjectInMap(int obj_id, vec4 obj_position, vec3 obj_size, const char * obj_file_name, vec3 model_size, int direction = 0, float gravity = 0);
vec4 GetPlayerSpawnCoordinates(std::vector<std::vector<string>> plant);
void BobCow();

// Desenho
void DrawMapObjects();
void DrawPlayer(vec4 position, float angle_y, float angle_x, float scale);
void DrawVirtualObject(const char* object_name, int object_id, glm::mat4 model);
void DrawSkyboxPlanes();

// Colisões
vec4 GetObjectTopBoundary(vec4 object_position, vec3 object_size);
float GetTileToleranceValue(int object_type);
bool BBoxCollision(vec4 obj1_pos, vec4 obj2_pos, vec3 obj1_size, vec3 obj2_size, float epsilon);
vecInt GetObjectsCollidingWithObject(int target_obj_index, vec4 target_obj_pos);
vecInt GetObjectsCollidingWithPlayer(vec4 player_position);
int GetVectorObjectType(vecInt vector_objects, int type);
int GetVectorObjectType(vecInt vector_objects, vecInt types);
bool vectorHasObjectBlockingObject(vecInt vector_objects, bool is_volleyball = false);
bool vectorHasVolleyballBlockingObject(vecInt vector_objects);
bool vectorHasPlayerBlockingObject(vecInt vector_objects);
bool CollidedWithEnemy(vecInt vector_objects);
void UnlockDoors(vecInt red, vecInt green, vecInt blue, vecInt yellow);

// Movimentação
void MovePlayer(int theme);
void MoveBlock(int block_index);
void MoveEnemies();
void MoveJet(int jet_index);
void MoveBeachBall(int ball_index);
void MoveVolleyBall(int ball_index);

// Auxiliares para desenho
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);
void ComputeNormals(ObjModel* model);
void BuildTrianglesAndAddToVirtualScene(ObjModel* model);

// Sistema de partículas (não funcional)
void AnimateParticles();
void GenerateParticles(int amount, vec4 position, vec3 object_size);
void DrawParticles();

// Carregamento de arquivos
Level LoadLevelFromFile(string filepath);
void LoadTextureImage(const char* filename);
void LoadShadersFromFiles();
GLuint LoadShader_Vertex(const char* filename);
GLuint LoadShader_Fragment(const char* filename);
void LoadShader(const char* filename, GLuint shader_id);
void LoadSoundFromFile(const char* path, sf::SoundBuffer * buffer);
void LoadMusicFromFile(const char* path, sf::Music * buffer);

// Audio e música
void PlayLevelMusic(int level_number);
void PlayMenuMusic();
void StopAllMusic();
void PlaySound(sf::SoundBuffer * buffer);

// GPU
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod);
void ErrorCallback(int error, const char* description);
void PrintGPUInfoInTerminal();

// Texto
void TextRendering_ShowFramesPerSecond(GLFWwindow* window) ;
void PrintObjModelInfo(ObjModel* model);

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

/***************************************/
/** CONSTANTES ESPECÍFICAS DE OBJETOS **/
/***************************************/

// Tipos de objetos
#define COW 		1
#define WALL 		10
#define LOCK 		11
#define DIRTBLOCK 	12
#define FLOOR 		13
#define DIRT 		14
#define WATER 		15
#define FIRE 		16
#define DOOR_RED 	17
#define DOOR_GREEN 	18
#define DOOR_BLUE	19
#define DOOR_YELLOW	20
#define BABYCOW 	21
#define JET         22
#define BEACHBALL   23
#define VOLLEYBALL  24
#define GRASS       25
#define WOOD        26
#define SNOW        27
#define DARKFLOOR   28
#define SNOWBLOCK   29
#define CRYSTAL     30
#define DARKDIRT    31
#define DARKROCK    32

#define KEY_RED 	40
#define KEY_GREEN 	41
#define KEY_BLUE	42
#define KEY_YELLOW 	43

#define PLAYER_HEAD 	60
#define PLAYER_TORSO 	61
#define PLAYER_ARM 		62
#define PLAYER_HAND 	63
#define PLAYER_LEG 		64
#define PLAYER_FOOT 	65

#define PARTICLE 	80

#define MOVEMENT_AMOUNT 0.02f
#define ENEMY_SPEED 0.05f
#define ROTATION_SPEED_X 0.01f
#define ROTATION_SPEED_Y 0.004f

#define SKYBOX_TOP      100
#define SKYBOX_BOTTOM   101
#define SKYBOX_EAST     102
#define SKYBOX_WEST     103
#define SKYBOX_SOUTH    104
#define SKYBOX_NORTH    105

#define ANIMATION_SPEED 10
#define ITEM_ROTATION_SPEED 0.1

#define SCREEN_EXIT         0
#define SCREEN_MAINMENU     1
#define SCREEN_LEVELSELECT  2
#define SCREEN_GAME         3
#define SCREEN_NEXTLEVEL    4

///////////////////////
// VARIÁVEIS GLOBAIS //
///////////////////////

std::map<string, SceneObject> g_VirtualScene;
std::stack<glm::mat4>  g_MatrixStack;
// Vetor que contém dados sobre os objetos dentro do mapa (usado para tratar colisões)
std::vector<MapObject> map_objects;
// Vetor de articulas
std::vector<Particle> particles;

// Razão de proporção da janela (largura/altura).
float g_ScreenRatio = 1.0f;
int g_WindowWidth = 800, g_WindowHeight = 600;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = false;

// Variável de controle da câmera
bool g_useFirstPersonCamera = false;

// Variáveis de controle de sons
bool g_MusicOn = true;
bool g_SoundsOn = true;

// Variáveis de controle do cursor
double g_LastCursorPosX, g_LastCursorPosY;

// Variável de controle do botão esquerdo do mouse.
bool g_LeftMouseButtonPressed = false;

// Variável de controle da tela atual (menu principal, level select, jogo, etc)
int g_CurrentScreen = SCREEN_MAINMENU;

// Angulos de rotação dos itens
// utilizado para controlar a rotação dos itens
float g_ItemAngleY = 0.0f;
float g_CowAngleY = 0.0f;

// Variável de controle de encerramento de nível
bool g_MapEnded = false;
bool g_ShowingMessage = false;

// Variáveis de controle das teclas
bool key_w_pressed = false;
bool key_a_pressed = false;
bool key_s_pressed = false;
bool key_d_pressed = false;
bool key_r_pressed = false;
bool key_space_pressed = false;
bool esc_pressed = false;

// Variáveis de posição do jogador
vec4 player_position;
float straight_vector_sign = 1.0f;
float sideways_vector_sign = 0.0f;
vec4 straight_vector; // Vetor direção reto do player (diferente do vetor camera_direction)
vec4 sideways_vector; // Vetor direção lateral do player (diferente do vetor U da câmera)
vec4 player_direction = vec4(0.0f, 0.0f, 1.0f, 0.0f); // Vetor direção do player, frequentemente igual à soma de straight + sideways

// CÂMERA LOOKAT
float g_CameraTheta = PI; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = 2.5f; // Distância da câmera para a origem.
vec4 camera_lookat_l;

// CÂMERA FIRST PERSON
vec4 camera_position_c  = player_position; // Ponto "c", centro da câmera
vec4 camera_xz_direction = vec4(0.0f, 0.0f, 2.0f, 0.0f); // Vetor que representa para onde a câmera aponta (Sem levar em conta o eixo y)
vec4 camera_view_vector = camera_xz_direction; // Vetor "view", sentido para onde a câmera está virada
vec4 camera_up_vector   = vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
vec4 camera_u_vector    = crossproduct(camera_up_vector, -camera_view_vector); // Vetor U da câmera (aponta para a lateral)

// Variável de controle de mudança de câmera
bool g_ChangedCamera = false;

// Inventário do jogador
Inventory player_inventory;

// Nível atual
int g_CurrentLevel = 0;

// Número de vacas em um nível
int g_LevelCowAmount;

// Variáveis de animações de mortes
bool g_DeathByWater = false;
bool g_DeathByEnemy = false;

// Sons
sf::SoundBuffer menucursorsound;
sf::SoundBuffer menuentersound;
sf::SoundBuffer keysound;
sf::SoundBuffer cowsound;
sf::SoundBuffer doorsound;
sf::SoundBuffer splashsound;
sf::SoundBuffer ball1sound;
sf::SoundBuffer deathsound;
sf::SoundBuffer winsound;
sf::SoundBuffer bellsound;
sf::Sound       sound;

// Música
sf::Music menumusic;
sf::Music techmusic;
sf::Music watermusic;
sf::Music naturemusic;
sf::Music crystalmusic;

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
GLint yellow_particle_color_uniform;
GLint skytheme_uniform;

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

    // Carregamento de imagens
    LoadTextureImage("../../data/textures/textures.png");   		// TextureImage0
    LoadTextureImage("../../data/textures/water.png");              // TextureImage1
    LoadTextureImage("../../data/textures/abra.png");               // TextureImage2
    LoadTextureImage("../../data/textures/frozen.png");             // TextureImage3
    LoadTextureImage("../../data/textures/midnat.png");             // TextureImage4

    // Carregamento de models
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

    ObjModel keymodel("../../data/objects/key.obj");
    ComputeNormals(&keymodel);
    BuildTrianglesAndAddToVirtualScene(&keymodel);

    ObjModel jetmodel("../../data/objects/jet.obj");
    ComputeNormals(&jetmodel);
    BuildTrianglesAndAddToVirtualScene(&jetmodel);

    // Carregamento de sons
    LoadSoundFromFile("../../data/sound/menucursor.wav", &menucursorsound);
    LoadSoundFromFile("../../data/sound/menuenter.wav", &menuentersound);
    LoadSoundFromFile("../../data/sound/key.wav", &keysound);
    LoadSoundFromFile("../../data/sound/cow.wav", &cowsound);
    LoadSoundFromFile("../../data/sound/door.wav", &doorsound);
    LoadSoundFromFile("../../data/sound/splash.wav", &splashsound);
    LoadSoundFromFile("../../data/sound/ball1.wav", &ball1sound);
    LoadSoundFromFile("../../data/sound/death.wav", &deathsound);
    LoadSoundFromFile("../../data/sound/win.wav", &winsound);
    LoadSoundFromFile("../../data/sound/bell.wav", &bellsound);

    // Carregamento de música
    LoadMusicFromFile("../../data/music/velapax.ogg", &menumusic);
    LoadMusicFromFile("../../data/music/landingbase.ogg", &techmusic);
    LoadMusicFromFile("../../data/music/highway.ogg", &watermusic);
    LoadMusicFromFile("../../data/music/rock1.ogg", &naturemusic);
    LoadMusicFromFile("../../data/music/lax_here.ogg", &crystalmusic);

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

    // Habilitamos blending, para colocar transparência em objetos (em testes)
    glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Inicializações para o menu
	PlayMenuMusic();
    g_CurrentScreen = SCREEN_MAINMENU;
    g_CurrentLevel = 1;

    // Rodamos o jogo até que o jogador peça para sair
    while (g_CurrentScreen > 0) {
    	// Main menu
    	if (g_CurrentScreen == SCREEN_MAINMENU)
    		g_CurrentScreen = RenderMainMenu(window);

    	// Render level
    	if (g_CurrentScreen == SCREEN_GAME) {
            PlayLevelMusic(g_CurrentLevel);
    		g_CurrentScreen = RenderLevel(g_CurrentLevel, window);
    		if ((g_CurrentScreen != SCREEN_GAME && g_CurrentScreen != SCREEN_NEXTLEVEL) || (g_CurrentLevel != 1 && g_CurrentScreen == SCREEN_NEXTLEVEL))
                PlayMenuMusic();
        }

    	// Next level
    	if (g_CurrentScreen == SCREEN_NEXTLEVEL) {
    		g_CurrentLevel++;
    		if (g_CurrentLevel > 5) {
                g_CurrentLevel = 1;
                g_CurrentScreen = SCREEN_MAINMENU;
                PlayMenuMusic();
            }
    		else g_CurrentScreen = SCREEN_GAME;
    	}

    	// Select level
    	if (g_CurrentScreen == SCREEN_LEVELSELECT) {
    		g_CurrentLevel = RenderLevelSelection(window);
            if (g_CurrentLevel > 0)
    		  g_CurrentScreen = SCREEN_GAME;
            else {
                g_CurrentLevel = 1;
                g_CurrentScreen = SCREEN_MAINMENU;
            }
    	}
    }

    StopAllMusic();

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

////////////////////////////////
// FUNÇÕES AUXILIARES SIMPLES //
////////////////////////////////

// Função que verifica se um valor está dentro de um conjunto de valores
// Útil para simplificar ifs
template <typename T>
bool isIn(const T& val, const std::initializer_list<T>& list) {
    for (const auto& i : list) {
        if (val == i) {
            return true;
        }
    }
    return false;
}

// Retorna o maior valor (float) dentre dois valores
float MaxFloat2(float a, float b) {
    if (a < b)
        return b;
    else return a;
}

// Converte um vetor para coordenadas homogêneas
vec4 VectorSetHomogeneous(vec3 nonHomogVector, bool isVectorPosVector) {
    if (isVectorPosVector)
        return vec4(nonHomogVector.x, nonHomogVector.y, nonHomogVector.z, 1.0f);
    else return vec4(nonHomogVector.x, nonHomogVector.y, nonHomogVector.z, 0.0f);
}

// Converte uma string para inteiro
// Usada na leitura de um nível
constexpr unsigned int string2int(const char* str, int h) {
    // DJB Hash function
    // not my code but can't remember where I got it from
    return !str[h] ? 5381 : (string2int(str, h+1)*33) ^ str[h];
}

////////////////////////////
// RENDERIZAÇÕES DE TELAS //
////////////////////////////

// Renderiza menu principal
int RenderMainMenu(GLFWwindow* window) {
    // Câmera fixa e presets
	camera_lookat_l = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	camera_position_c = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	camera_view_vector = camera_lookat_l - camera_position_c;
	g_CameraDistance = 2.5f;
    key_space_pressed = false;
    int menu_position = 0;

    // Renderizamos até retornar
	while(true) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);

        // Strings
        string newgame_text = "NEW GAME";
        string selectlevel_text = "SELECT LEVEL";
        string exit_text = "EXIT GAME";

        // Rotação/animação da vaca
        g_ItemAngleY += ITEM_ROTATION_SPEED;
        if (g_ItemAngleY >= 2*PI)
            g_ItemAngleY = 0;

        // Movimentação do menu
        if (key_w_pressed and menu_position > 0) {
        	menu_position--;
        	PlaySound(&menucursorsound);
        	key_w_pressed = false;
        }

        if (key_s_pressed and menu_position < 2) {
        	menu_position++;
        	PlaySound(&menucursorsound);
        	key_s_pressed = false;
        }

        if (key_space_pressed) {
            PlaySound(&menuentersound);
        	switch(menu_position) {
        	case 0: g_CurrentLevel = 1; return SCREEN_GAME;
        	case 1: return SCREEN_LEVELSELECT;
        	case 2: return SCREEN_EXIT;
        	}
        }

        // Câmera
        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        glm::mat4 projection;
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -20.0f; // Posição do "far plane"

        // Menu utiliza projeção ortográfica (para visualizar a vaca)
        float t = 1.5f*g_CameraDistance/2.5f;
        float b = -t;
        float r = t*g_ScreenRatio;
        float l = -r;
        projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        // Desenhamos a vaca
    	glm::mat4 cowmodel = Matrix_Translate(1.0f, 0.21f - menu_position * 0.3f, -0.45f)
        	* Matrix_Scale(0.1f, 0.1f, 0.1f)
        	* Matrix_Translate(-0.2f, 0.0f, 0.0f)
        	* Matrix_Rotate_Y(g_ItemAngleY)
        	* Matrix_Translate(0.2f, 0.0f, 0.0f);
        DrawVirtualObject("cow", BABYCOW, cowmodel);

        // Imprimimos o texto
        // Caso o texto esteja selecionado, ele fica maior.
        if (menu_position == 0)
        	TextRendering_PrintString(window, newgame_text, -0.2f, 0.1f, 2.5f);
        else TextRendering_PrintString(window, newgame_text, -0.2f, 0.1f, 2.0f);

        if (menu_position == 1)
        	TextRendering_PrintString(window, selectlevel_text, -0.2f, -0.1f, 2.5f);
        else TextRendering_PrintString(window, selectlevel_text, -0.2f, -0.1f, 2.0f);

        if (menu_position == 2)
        	TextRendering_PrintString(window, exit_text, -0.2f, -0.3f, 2.5f);
        else TextRendering_PrintString(window, exit_text, -0.2f, -0.3f, 2.0f);

        // FPS
        if (g_ShowInfoText)
            TextRendering_ShowFramesPerSecond(window);

        glfwSwapBuffers(window);
        // Verificação de eventos
        glfwPollEvents();
	}
	return -1;
}

// Renderiza tela de seleção de níveis
int RenderLevelSelection(GLFWwindow* window) {
    // Câmera fixa e presets
	camera_lookat_l = vec4(1.0f, 0.0f, 0.0f, 1.0f);
	camera_position_c = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	g_CameraDistance = 2.5f;
	camera_view_vector = camera_lookat_l - camera_position_c;
	key_space_pressed = false;
    int menu_position = 0;
    int chosen_level = 1;
    bool choosing_level = false;

    // Renderizamos até retornar
	while(true) {
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);

        // Strings
        string enterlevel_text = "ENTER LEVEL: ";
        string lvtext[] = {"01", "02", "03", "04", "05"};
        string go_text = "GO!";

        // Animação da vaca
        g_ItemAngleY += ITEM_ROTATION_SPEED;
        if (g_ItemAngleY >= 2*PI)
            g_ItemAngleY = 0;

        // Se o usuário aperta esc, volta para o menu anterior
        if (esc_pressed) {
            PlaySound(&menuentersound);
        	return 0;
        }

        // Movimentação do menu
        if (key_w_pressed) {
            PlaySound(&menucursorsound);
            key_w_pressed = false;
            if (choosing_level)
                chosen_level = std::max(chosen_level-1, 1);
            else if (menu_position > 0)
                menu_position--;
        }

        if (key_s_pressed) {
            PlaySound(&menucursorsound);
            key_s_pressed = false;
            if (choosing_level)
                chosen_level = std::min(chosen_level+1, 5);
            else if (menu_position < 1)
                menu_position++;
        }

        if (key_space_pressed) {
            PlaySound(&menuentersound);
        	key_space_pressed = false;
        	switch(menu_position) {
        	case 0: {
        		choosing_level = !choosing_level;
        		break;
        	}
        	case 1: return chosen_level;
        	}
        }

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        glm::mat4 projection;
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -20.0f; // Posição do "far plane"

        // Projeção Ortográfica
        float t = 1.5f*g_CameraDistance/2.5f;
        float b = -t;
        float r = t*g_ScreenRatio;
        float l = -r;
        projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

    	glm::mat4 cowmodel = Matrix_Translate(1.0f, 0.21f - menu_position * 0.3f, -0.45f)
        	* Matrix_Scale(0.1f, 0.1f, 0.1f)
        	* Matrix_Translate(-0.2f, 0.0f, 0.0f)
        	* Matrix_Rotate_Y(g_ItemAngleY)
        	* Matrix_Translate(0.2f, 0.0f, 0.0f);
        DrawVirtualObject("cow", BABYCOW, cowmodel);

        // Escrita dos textos na tela
        if(menu_position == 0 && !choosing_level)
        	TextRendering_PrintString(window, enterlevel_text, -0.2f, 0.1f, 2.5f);
        else TextRendering_PrintString(window, enterlevel_text, -0.2f, 0.1f, 2.0f);

        if(choosing_level)
        	TextRendering_PrintString(window, lvtext[chosen_level - 1], 0.45f, 0.1f, 2.5f);
        else TextRendering_PrintString(window, lvtext[chosen_level - 1], 0.45f, 0.1f, 2.0f);

        if(menu_position == 1 && !choosing_level)
        	TextRendering_PrintString(window, go_text, -0.2f, -0.05f, 2.5f);
        else TextRendering_PrintString(window, go_text, -0.2f, -0.05f, 2.0f);

        // FPS
        if (g_ShowInfoText)
            TextRendering_ShowFramesPerSecond(window);

        glfwSwapBuffers(window);
        // Verificação de eventos
        glfwPollEvents();
	}
	return -1;
}

// Ajusta câmera FPS
vec4 AdjustFPSCamera(vec4 position) {
    return vec4(position.x, position.y + 0.5f, position.z, 1.0f);
}

// Renderiza nível dado
int RenderLevel(int level_number, GLFWwindow* window) {
	// Reset variables
    map_objects.clear();
	particles.clear();
	ClearInventory();
	g_MapEnded = false;
	g_DeathByWater = false;
	g_DeathByEnemy = false;
    g_ItemAngleY = 0;
	g_useFirstPersonCamera = false;
    straight_vector_sign = 1.0f;
    sideways_vector_sign = 0.0f;
    player_direction = vec4(0.0f, 0.0f, 1.0f, 0.0f);
    g_CameraTheta = PI;
    g_ChangedCamera = false;
    g_CameraPhi = 0.0f;
    g_CameraDistance = 3.5f;
    camera_position_c  = player_position;
    camera_xz_direction = vec4(0.0f, 0.0f, 2.0f, 0.0f);
    g_ShowingMessage = false;

    // Variável de controle da animação de morte
    int death_timer = 1000;

    // Variável de controle do tempo do nível
    int map_timer = 30;

    // Variável de controle de animação dos tiles
    int curr_anim_tile = 0;
    int anim_timer = 0;

    // Variáveis de controle da mensagem de morte
    string message = "";

    // Carrega nível um
    string levelpath = "../../data/levels/" + std::to_string(level_number);
    Level level = LoadLevelFromFile(levelpath);
    RegisterLevelObjects(level);
    g_LevelCowAmount = level.cow_no;
    player_position = GetPlayerSpawnCoordinates(level.plant);
    camera_lookat_l = player_position;

    // Ficamos em loop, renderizando
    while (true)
    {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(program_id);

        // Retorno para tela inicial
        if(esc_pressed)
        	return SCREEN_MAINMENU;
        if(g_MapEnded) {
            g_ShowingMessage = true;
            message = "Congratulations! You finished this level :)";
        }

        if (key_r_pressed) {
            key_r_pressed = false;
            return SCREEN_GAME;
        }

        // Controle do tipo de câmera
        if (g_useFirstPersonCamera) {
            // First-person
            camera_position_c = AdjustFPSCamera(player_position);
            if (g_ChangedCamera) {
                camera_view_vector = player_direction;
                g_ChangedCamera = false;
            }
        }
        else {
            // LookAt (third-person)
            float r = g_CameraDistance;
            float y = r*sin(g_CameraPhi);
            float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
            float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

            camera_position_c  = vec4(x+player_position[0],y,z+player_position[2],1.0f); // Ponto "c", centro da câmera
            camera_lookat_l    = player_position; // Ponto "l", para onde a câmera (look-at) estará sempre olhando
            camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
            camera_u_vector    = crossproduct(camera_up_vector, -camera_view_vector);
        }

        // Vetor direção da câmera do plano XZ
        camera_xz_direction = vec4(camera_view_vector[0] + 0.01f, 0.0f, camera_view_vector[2] + 0.01f, 0.0f);

        glm::mat4 view = Matrix_Camera_View(camera_position_c, camera_view_vector, camera_up_vector);
        glm::mat4 projection;
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -500.0f; // Posição do "far plane"

        // Projeção perspectiva
        float field_of_view = 3.141592 / 3.0f;
        projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        glUniformMatrix4fv(view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));

        /////////////
        // JOGADOR //
        /////////////

        // Movimentação do personagem, mensagens e animações de morte
        if (g_ShowingMessage) {
            //TextRendering_PrintString(window, message.c_str(), -0.7f, 0.3f, 2.5f);
            if (key_space_pressed && !g_MapEnded)
                return SCREEN_GAME;
            else if (key_space_pressed)
                return SCREEN_NEXTLEVEL;
        } else if (g_DeathByWater || g_DeathByEnemy) {
            death_timer -= 10;
            if (death_timer <= 0) {
                if (g_DeathByEnemy) {
                    g_ShowingMessage = true;
                    message = "Watch out for the creatures!";
                }
                else if (g_DeathByWater) {
                    g_ShowingMessage = true;
                    message = "You can't swim!";
                }
            }
            if (g_DeathByWater)
                player_position[1] -= 0.01f;
            else if (g_DeathByEnemy && death_timer >= 990)
                PlaySound(&deathsound);
        }
        else MovePlayer(level.theme);

        // Ajusta vetores de direção
        straight_vector = straight_vector_sign * camera_xz_direction;
    	sideways_vector = sideways_vector_sign * camera_u_vector;
    	player_direction = straight_vector + sideways_vector;

        // Ajusta ângulo para onde o corpo do boneco está virado
        float bodyangle_Y = acos(dotproduct(player_direction, vec4(1.0f,0.0f,0.0f,0.0f))/norm(player_direction));
        if (player_direction[2] > 0.0f)
            bodyangle_Y = -bodyangle_Y;
        float bodyangle_X = 0.0f;
        if (g_DeathByEnemy)
            bodyangle_X = MaxFloat2(death_timer * 0.002f - 2.0f, -PI/2);

        // Desenha player
        if(!g_useFirstPersonCamera)
            DrawPlayer(player_position, bodyangle_Y + PI/2, bodyangle_X, 0.3f);

        ///////////////////
        // RESTO DO MAPA //
        ///////////////////

        DrawMapObjects(); // Desenha
        if (!g_ShowingMessage)
            MoveEnemies();    // Movimenta inimigos

        BobCow();

        ////////////
        // SKYBOX //
        ////////////

        if (level.theme > 0) {
            DrawSkyboxPlanes();
            glUniform1i(skytheme_uniform, level.theme);
        }

        ///////////////
        // ANIMAÇÕES //
        ///////////////

        // Animação dos tiles (todos são animados em simetria)
        // CASO DESEJA-SE ANIMÁ-LOS DE FORMA INDEPENDENTE, deve-se
        // copiar este trecho para cada switch na função de renderização.
        anim_timer = (anim_timer + 1) % ANIMATION_SPEED;
        if (anim_timer == 0)
        	curr_anim_tile = (curr_anim_tile+1) % 16;
		glUniform1i(anim_timer_uniform, curr_anim_tile);

		// Rotação dos itens
		g_ItemAngleY += ITEM_ROTATION_SPEED;
		if (g_ItemAngleY >= 2*PI)
			g_ItemAngleY = 0;

        // Rotação da vaca mãe
        g_CowAngleY += ITEM_ROTATION_SPEED / 4;
        if (g_CowAngleY >= 2*PI)
            g_CowAngleY = 0;

		// Fogo: partículas
		AnimateParticles();

        // Tempo do nível
        map_timer--;
        if (map_timer <= 0) {
            map_timer = 40;
            level.time--;
            if (level.time == 0) {
                PlaySound(&bellsound);
                g_ShowingMessage = true;
                message = "Watch the time!";
            }
        }

        // Mostra inventário na tela
        ShowInventory(window, level.time);

        // FPS
        if (g_ShowInfoText)
            TextRendering_ShowFramesPerSecond(window);

        glfwSwapBuffers(window);
        // Verificação de eventos
        glfwPollEvents();
    }
    return -1;
}

// Mostra o inventário do jogador na tela
void ShowInventory(GLFWwindow* window, int level_time) {
	float pad = TextRendering_LineHeight(window);

	string invstring = "REQUIRED COWS: " + std::to_string(g_LevelCowAmount - player_inventory.cows) + " KEYS: ";
	if (player_inventory.keys.red) invstring += "R ";
	else invstring += "  ";
	if (player_inventory.keys.green) invstring += "G ";
	else invstring += "  ";
	if (player_inventory.keys.blue) invstring += "B ";
	else invstring += "  ";
	if (player_inventory.keys.yellow) invstring += "Y ";
	else invstring += "  ";
    invstring += "TIME: " + std::to_string(level_time);

    TextRendering_PrintString(window, invstring, -1.0f+pad/5, 1.0f-pad, 1.0f);
}

//////////////////////////////
// CONFIGURAÇÃO DE UM NÍVEL //
//////////////////////////////

void ClearInventory() {
    player_inventory.keys = {0,0,0,0};
    player_inventory.cows = 0;
}

// Retorna a posição da vaca mãe no vetor de objetos
int GetCowMotherPosition() {
    for(unsigned int i=0; i < map_objects.size(); i++) {
        if (map_objects[i].object_type == COW)
            return i;
    }
    return -1;
}

// Função que registra os objetos do nível com base na sua planta
void RegisterLevelObjects(Level level) {
    float center_x = (level.width-1)/2.0f;
    float center_z = (level.height-1)/2.0f;

    for(int line = 0; line < level.height; line++) {
        for(int col = 0; col < level.width; col++) {
            string current_tile = level.plant[line][col];
            float x = -(center_x - col);
            float z = -(center_z - line);

            RegisterObjectInMapVector(current_tile, x, z, level.theme);
        }
    }
}

// Registra um piso com base no tema do nível
void RegisterFloor(float x, float z, int theme) {
    vec3 tile_size = vec3(1.0f, 0.0f, 1.0f);
    vec3 planemodel_size = vec3(1.0f, 1.0f, 1.0f);
    float floor_shift = -1.0f;

    switch(theme) {
        case 0:
            RegisterObjectInMap(FLOOR, vec4(x, floor_shift, z, 1.0f), tile_size, "plane", planemodel_size);
            break;
        case 1:
            RegisterObjectInMap(GRASS, vec4(x, floor_shift, z, 1.0f), tile_size, "plane", planemodel_size);
            break;
        case 2:
            RegisterObjectInMap(DARKFLOOR, vec4(x, floor_shift, z, 1.0f), tile_size, "plane", planemodel_size);
            break;
        case 3:
            RegisterObjectInMap(SNOW, vec4(x, floor_shift, z, 1.0f), tile_size, "plane", planemodel_size);
            break;
        case 4:
            RegisterObjectInMap(DARKDIRT, vec4(x, floor_shift, z, 1.0f), tile_size, "plane", planemodel_size);
            break;
    }
}

// Função que registra um objeto em dada posição do mapa
// CASO SE QUEIRA ADICIONAR NOVOS OBJETOS, DEVE-SE FAZÊ-LO AQUI
void RegisterObjectInMapVector(string tile_type, float x, float z, int theme) {
    /* Propriedades de objetos (deslocamento, tamanho, etc) */

    // Cubo (genérico)
    vec3 cube_size = vec3(1.0f, 1.0f, 1.0f);
    float cube_vertical_shift = -0.5f;

    // Blocos de terra
    vec3 dirtblock_size = vec3(0.8f, 0.8f, 0.8f);
    float dirtblock_vertical_shift = -0.6f;

    // Chaves
    vec3 keymodel_size = vec3(0.1f, 0.1f, 0.1f);
    float key_vertical_shift = -1.0f;
    vec3 key_size = vec3(0.5f, 0.5f, 0.5f);

    // Vaca mãe
    vec3 cow_size = vec3(0.7f, 0.7f, 0.7f);
    float cow_vertical_shift = -0.5f;

    // Vaca bebê (a ser coletada)
    vec3 babycow_size = vec3(0.35f, 0.35f, 0.35f);
    float babycow_vertical_shift = -0.5f;

    // Jet
    vec3 jetmodel_size = vec3(0.03f, 0.03f, 0.03f);
    vec3 jet_size = vec3(0.8f, 0.8f, 0.8f);
    float jet_vertical_shift = -0.5f;

    // Esfera e bolas
    vec3 sphere_size = vec3(0.4f, 0.4f, 0.4f);
    vec3 ball_size = vec3(0.8f, 0.8f, 0.8f);
    float sphere_vertical_shift = -0.5f;

    // Tiles planos (chão, água, grama, etc)
    vec3 tile_size = vec3(1.0f, 0.0f, 1.0f);
    vec3 planemodel_size = vec3(1.0f, 1.0f, 1.0f);
    float floor_shift = -1.0f;

    /* Adicione novos tipos de objetos abaixo */
    switch(string2int(tile_type.c_str())) {
    // Parede
    case string2int("BL"): {
        RegisterObjectInMap(WALL, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        break;
    }

    // Madeira
    case string2int("WO"): {
        RegisterObjectInMap(WOOD, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        break;
    }

    // Bloco com neve
    case string2int("SB"): {
        RegisterObjectInMap(SNOWBLOCK, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        break;
    }

    // Rocha negra
    case string2int("BR"): {
        RegisterObjectInMap(DARKROCK, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        break;
    }

    // Cristal
    case string2int("CR"): {
        RegisterObjectInMap(CRYSTAL, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        break;
    }

    // Água
    case string2int("WA"):{
        RegisterObjectInMap(WATER, vec4(x, floor_shift, z, 1.0f), cube_size, "plane", planemodel_size);
        break;
    }

    // Fogo:
    case string2int("FI"):{
        RegisterObjectInMap(FIRE, vec4(x, floor_shift, z, 1.0f), cube_size, "fire", cube_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Terra
    case string2int("DI"):{
        RegisterObjectInMap(DIRT, vec4(x, floor_shift, z, 1.0f), tile_size, "plane", planemodel_size);
        break;
    }

    // Bloco de terra
    case string2int("BD"):{
        RegisterObjectInMap(DIRTBLOCK, vec4(x, dirtblock_vertical_shift, z, 1.0f), dirtblock_size, "cube", dirtblock_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Chave vermelha
    case string2int("kr"):{
    	RegisterObjectInMap(KEY_RED, vec4(x, key_vertical_shift, z, 1.0f), key_size, "key", keymodel_size);
    	RegisterFloor(x, z, theme);
    	break;
    }

    // Chave verde
    case string2int("kg"):{
    	RegisterObjectInMap(KEY_GREEN, vec4(x, key_vertical_shift, z, 1.0f), key_size, "key", keymodel_size);
    	RegisterFloor(x, z, theme);
    	break;
    }

	// Chave azul
    case string2int("kb"):{
    	RegisterObjectInMap(KEY_BLUE, vec4(x, key_vertical_shift, z, 1.0f), key_size, "key", keymodel_size);
    	RegisterFloor(x, z, theme);
    	break;
    }

	// Chave amarela
    case string2int("ky"):{
    	RegisterObjectInMap(KEY_YELLOW, vec4(x, key_vertical_shift, z, 1.0f), key_size, "key", keymodel_size);
    	RegisterFloor(x, z, theme);
    	break;
    }

    // Porta vermelha:
    case string2int("DR"):{
        RegisterObjectInMap(DOOR_RED, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Porta verde:
    case string2int("DG"):{
        RegisterObjectInMap(DOOR_GREEN, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Porta azul:
    case string2int("DB"):{
        RegisterObjectInMap(DOOR_BLUE, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Porta amarela:
    case string2int("DY"):{
        RegisterObjectInMap(DOOR_YELLOW, vec4(x, cube_vertical_shift, z, 1.0f), cube_size, "cube", cube_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Vaquinha bebê:
    case string2int("co"):{
        RegisterObjectInMap(BABYCOW, vec4(x, babycow_vertical_shift, z, 1.0f), babycow_size, "cow", babycow_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Vaca mãe:
    case string2int("CW"):{
        RegisterObjectInMap(COW, vec4(x, cow_vertical_shift, z, 1.0f), cow_size, "cow", cow_size, 1);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("J0"):{
        RegisterObjectInMap(JET, vec4(x, jet_vertical_shift, z, 1.0f), jet_size, "jet", jetmodel_size);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("J1"):{
        RegisterObjectInMap(JET, vec4(x, jet_vertical_shift, z, 1.0f), jet_size, "jet", jetmodel_size, 1);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("J2"):{
        RegisterObjectInMap(JET, vec4(x, jet_vertical_shift, z, 1.0f), jet_size, "jet", jetmodel_size, 2);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("J3"):{
        RegisterObjectInMap(JET, vec4(x, jet_vertical_shift, z, 1.0f), jet_size, "jet", jetmodel_size, 3);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("B0"):{
        RegisterObjectInMap(BEACHBALL, vec4(x, sphere_vertical_shift, z, 1.0f), ball_size, "sphere", sphere_size);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("B1"):{
        RegisterObjectInMap(BEACHBALL, vec4(x, sphere_vertical_shift, z, 1.0f), ball_size, "sphere", sphere_size, 1);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("B2"):{
        RegisterObjectInMap(BEACHBALL, vec4(x, sphere_vertical_shift, z, 1.0f), ball_size, "sphere", sphere_size, 2);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("B3"):{
        RegisterObjectInMap(BEACHBALL, vec4(x, sphere_vertical_shift, z, 1.0f), ball_size, "sphere", sphere_size, 3);
        RegisterFloor(x, z, theme);
        break;
    }

    case string2int("V0"):{
        RegisterObjectInMap(VOLLEYBALL, vec4(x, sphere_vertical_shift, z, 1.0f), ball_size, "sphere", sphere_size);
        RegisterFloor(x, z, theme);
        break;
    }

    // Jogador e piso
    case string2int("PS"):
    case string2int("FF"):
    case string2int("GR"):
    case string2int("SN"):
    case string2int("DD"):{
        RegisterFloor(x, z, theme);
        break;
    }

    default:
        break;
    }
}

// Função que adiciona um objeto ao mapa
void RegisterObjectInMap(int obj_id, vec4 obj_position, vec3 obj_size, const char * obj_file_name, vec3 model_size, int direction, float gravity) {
    MapObject new_object;
    new_object.object_type = obj_id;
    new_object.object_size = obj_size;
    new_object.object_position = obj_position;
    new_object.obj_file_name = obj_file_name;
    new_object.model_size = model_size;
    new_object.direction = direction;
    new_object.gravity = gravity;
    map_objects.push_back(new_object);
}

// Função que encontra as coordenadas do spawn do jogador na planta do mapa
vec4 GetPlayerSpawnCoordinates(std::vector<std::vector<string>> plant) {
    int map_height = plant.size();
    int map_width = plant[0].size();
    float center_x = (map_width-1)/2.0f;
    float center_z = (map_height-1)/2.0f;

    for(int line = 0; line < map_height; line++) {
        for(int col = 0; col < map_width; col++) {
            if(plant[line][col] == "PS") {
                float x = -(center_x - col);
                float z = -(center_z - line);

                return vec4(x, -0.5f, z, 1.0f);
            }
        }
    }

    // Caso o spawn não seja encontrado, spawna o player no centro por padrão
    return vec4(0.5f, 0.0f, 0.5f, 1.0f);
}

// Faz a vaca ficar levemente flutuando
void BobCow() {
    int index = GetCowMotherPosition();

    // Queda
    if (map_objects[index].direction == 0) {
        if (map_objects[index].object_position.y > -0.5f)
            map_objects[index].object_position.y -= 0.0025;
        else map_objects[index].direction = 1;
    } else {
        // Elevação
        if (map_objects[index].object_position.y < -0.2f)
            map_objects[index].object_position.y += 0.0025;
        else map_objects[index].direction = 0;
    }
}

/////////////
// DESENHO //
/////////////

// Função que desenha os objetos na cena (com base no vetor de objetos)
void DrawMapObjects() {
    for(unsigned int i = 0; i < map_objects.size(); i++) {
        MapObject current_object = map_objects[i];
        int obj_type = current_object.object_type;
        glm::mat4 model = Matrix_Translate(current_object.object_position.x, current_object.object_position.y, current_object.object_position.z)
                        * Matrix_Scale(current_object.model_size.x, current_object.model_size.y, current_object.model_size.z);

        // Aplica rotações dependendo do objeto (animações, inimigos, etc)
        if (isIn(obj_type, {KEY_RED, KEY_GREEN, KEY_BLUE, KEY_YELLOW})) {
        	model = model * Matrix_Translate(0.0f, 5.7f, 0.0f)
        		* Matrix_Rotate_Y(g_ItemAngleY)
        		* Matrix_Rotate_Z(PI/5)
        		* Matrix_Translate(0.0f, -5.7f, 0.0f);
        } else if (obj_type == BABYCOW) {
    		model = model * Matrix_Translate(-0.2f, 0.0f, 0.0f)
        		* Matrix_Rotate_Y(g_ItemAngleY)
        		* Matrix_Translate(0.2f, 0.0f, 0.0f);
        } else if (obj_type == COW) {
            model = model * Matrix_Translate(-0.2f, 0.0f, 0.0f)
                * Matrix_Rotate_Y(g_CowAngleY)
                * Matrix_Translate(0.2f, 0.0f, 0.0f);
        } else if (obj_type == FIRE) {
        	GenerateParticles(5, current_object.object_position, current_object.model_size);
        	DrawParticles();
        } else if (obj_type == JET) {
        	model = model * Matrix_Translate(-0.2f, 0.0f, 0.0f)
        		* Matrix_Rotate_Y(current_object.direction * PI/2)
        		* Matrix_Translate(0.2f, 0.0f, 0.0f);
   		}

        DrawVirtualObject(current_object.obj_file_name, current_object.object_type, model);
    }
}

// Função que desenha o jogador usando transformações hierárquicas
// Desenha na posição dada, com escala dada e ângulo de rotação dado
void DrawPlayer(vec4 position, float angle_y, float angle_x, float scale) {
    float x = position.x;
    float y = position.y + 0.2f; // Shift vertical do player
    float z = position.z;
    glm::mat4 model = Matrix_Identity();

    model = Matrix_Translate(x, y, z) * Matrix_Rotate_Y(angle_y);
    model = model * Matrix_Translate(0.0f, -0.6f, 0.0f) * Matrix_Rotate_X(angle_x) * Matrix_Translate(0.0f, 0.6f, 0.0f);
    PushMatrix(model);
        model = model * Matrix_Scale(0.8f * scale, 1.1f * scale, 0.2f * scale);
        DrawVirtualObject("cube", PLAYER_TORSO, model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(-0.55f * scale, 0.05f * scale, 0.0f * scale); // Translação do braço direito
        PushMatrix(model);
            model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do braço direito
            DrawVirtualObject("cube", PLAYER_ARM, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.75f * scale, 0.0f * scale); // Translação do antebraço direito
            PushMatrix(model);
                model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do antebraço direito
                DrawVirtualObject("cube", PLAYER_ARM, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.45f * scale, 0.0f * scale); // Translação da mão direita
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.2f * scale);
                DrawVirtualObject("cube", PLAYER_HAND, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(0.55f * scale, 0.05f * scale, 0.0f * scale); // Translação para o braço esquerdo
        PushMatrix(model);
            model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do braço esquerdo
            DrawVirtualObject("cube", PLAYER_ARM, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.75f * scale, 0.0f * scale); // Translação do antebraço esquerdo
            PushMatrix(model);
                model = model * Matrix_Scale(0.2f * scale, 0.7f * scale, 0.2f * scale); // Escalamento do antebraço esquerdo
                DrawVirtualObject("cube", PLAYER_ARM, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.45f * scale, 0.0f * scale); // Translação da mão esquerda
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.2f * scale);
                DrawVirtualObject("cube", PLAYER_HAND, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(-0.2f * scale, -1.0f * scale, 0.0f * scale); // Translação para a perna direita
        PushMatrix(model);
            model = model * Matrix_Scale(0.3f * scale, 0.8f * scale, 0.3f * scale); // Escalamento da coxa direita
            DrawVirtualObject("cube", PLAYER_LEG, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.85f * scale, 0.0f * scale); // Translação para a canela direita
            PushMatrix(model);
                model = model * Matrix_Scale(0.25f * scale, 0.8f * scale, 0.25f * scale); // Escalamento da canela direita
                DrawVirtualObject("cube", PLAYER_LEG, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.5f * scale, 0.1f * scale); // Translação para o pé direito
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.4f * scale); // Escalamento do pé direito
                DrawVirtualObject("cube", PLAYER_FOOT, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    PushMatrix(model);
        model = model * Matrix_Translate(0.2f * scale, -1.0f * scale, 0.0f * scale); // Translação para a perna esquerda
        PushMatrix(model);
            model = model * Matrix_Scale(0.3f * scale, 0.8f * scale, 0.3f * scale); // Escalamento da coxa esquerda
            DrawVirtualObject("cube", PLAYER_LEG, model);
        PopMatrix(model);
        PushMatrix(model);
            model = model * Matrix_Translate(0.0f * scale, -0.85f * scale, 0.0f * scale); // Translação para a canela esquerda
            PushMatrix(model);
                model = model * Matrix_Scale(0.25f * scale, 0.8f * scale, 0.25f * scale); // Escalamento da canela esquerda
                DrawVirtualObject("cube", PLAYER_LEG, model);
            PopMatrix(model);
            PushMatrix(model);
                model = model * Matrix_Translate(0.0f * scale, -0.5f * scale, 0.1f * scale); // Translação para o pé esquerdo
                model = model * Matrix_Scale(0.2f * scale, 0.1f * scale, 0.4f * scale); // Escalamento do pé esquerdo
                DrawVirtualObject("cube", PLAYER_FOOT, model);
            PopMatrix(model);
        PopMatrix(model);
    PopMatrix(model);
    model = model * Matrix_Rotate_Z(3.14);
    model = model * Matrix_Translate(0.0f * scale, -0.75f * scale, 0.0f * scale); // Translação para a cabeça
    model = model * Matrix_Scale(0.35f * scale, 0.35f * scale, 0.35f * scale); // Escalamento da cabeça
    DrawVirtualObject("cube", PLAYER_HEAD, model);
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name, int object_id, glm::mat4 model) {
    glUniformMatrix4fv(model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
    glUniform1i(object_id_uniform, object_id);

    // "Ligamos" o VAO.
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);

    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)g_VirtualScene[object_name].first_index
    );

    // "Desligamos" o VAO
    glBindVertexArray(0);
}

// Desenha a skybox
void DrawSkyboxPlanes() {
    float skybox_distance = 100.0f;

    glm::mat4 model = Matrix_Translate(skybox_distance, 0.0f, 0.0f)
                    * Matrix_Scale(1.0f, skybox_distance*2, skybox_distance*2)
                    * Matrix_Rotate_Z(PI/2)
                    * Matrix_Rotate_Y(-PI/2);
    DrawVirtualObject("plane", SKYBOX_WEST, model);

    model = Matrix_Translate(-skybox_distance, 0.0f, 0.0f)
                    * Matrix_Scale(1.0f, skybox_distance*2, skybox_distance*2)
                    * Matrix_Rotate_Z(-PI/2)
                    * Matrix_Rotate_Y(PI/2);
    DrawVirtualObject("plane", SKYBOX_EAST, model);

    model = Matrix_Translate(0.0f, skybox_distance, 0.0f)
                    * Matrix_Scale(skybox_distance*2, 1.0f, skybox_distance*2)
                    * Matrix_Rotate_X(PI);
    DrawVirtualObject("plane", SKYBOX_TOP, model);

    model = Matrix_Translate(0.0f, -skybox_distance, 0.0f)
                    * Matrix_Scale(skybox_distance*2, 1.0f, skybox_distance*2);
    DrawVirtualObject("plane", SKYBOX_BOTTOM, model);

    model = Matrix_Translate(0.0f, 0.0f, skybox_distance)
                    * Matrix_Scale(skybox_distance*2, skybox_distance*2, 1.0f)
                    * Matrix_Rotate_X(-PI/2)
                    * Matrix_Rotate_Y(PI);
    DrawVirtualObject("plane", SKYBOX_NORTH, model);

    model = Matrix_Translate(0.0f, 0.0f, -skybox_distance)
                    * Matrix_Scale(skybox_distance*2, skybox_distance*2, 1.0f)
                    * Matrix_Rotate_X(PI/2);
    DrawVirtualObject("plane", SKYBOX_SOUTH, model);
}

//////////////
// COLISÕES //
//////////////

// Dado um objeto e seu tamanho, retorna as coordenadas onde ele "começa"
// Necessário pois os objetos do jogo contém a posição central deles
vec4 GetObjectTopBoundary(vec4 object_position, vec3 object_size) {
    return object_position - VectorSetHomogeneous(object_size, false) / 2.0f;
}

// Dado um tipo de objeto, retorna o valor de tolerância
// Usado para a função de colisão com o player
float GetTileToleranceValue(int object_type) {
	switch(object_type) {
    	case WALL:
        case WOOD:
        case DARKROCK:
        case CRYSTAL:
        case SNOWBLOCK:
    	case DIRTBLOCK:
    	case DOOR_RED:
    	case DOOR_GREEN:
    	case DOOR_BLUE:
    	case DOOR_YELLOW: {
    		return 0.25f;
    	}
    	case KEY_RED:
    	case KEY_BLUE:
    	case KEY_GREEN:
    	case KEY_YELLOW:
    	case BABYCOW:
    		return 0.4f;
    	case COW:
    		return 0.1f;
    	case DIRT:
    	case WATER:
    	case FLOOR:
        case GRASS:
        case SNOW:
        case DARKDIRT:
    	default: {
    		return -0.1f;
    	}
    }
}

// Dado dois objetos e seus tamanhos, testa a colisão via bounding box
bool BBoxCollision(vec4 obj1_pos, vec4 obj2_pos, vec3 obj1_size, vec3 obj2_size, float epsilon) {
    // Recomputa a bounding box dos objetos
    // Isto porque a posição dos objetos está no centro, e não no canto, como deveria ser
    obj1_pos = GetObjectTopBoundary(obj1_pos, obj1_size);
    obj2_pos = GetObjectTopBoundary(obj2_pos, obj2_size);

    if (
        ((obj1_pos.x - epsilon <= obj2_pos.x && obj2_pos.x < obj1_pos.x + obj1_size.x + epsilon) || (obj2_pos.x - epsilon <= obj1_pos.x && obj1_pos.x < obj2_pos.x + obj2_size.x + epsilon)) &&
        ((obj1_pos.y <= obj2_pos.y && obj2_pos.y < obj1_pos.y + obj1_size.y) || (obj2_pos.y <= obj1_pos.y && obj1_pos.y < obj2_pos.y + obj2_size.y)) &&
        ((obj1_pos.z - epsilon <= obj2_pos.z && obj2_pos.z < obj1_pos.z + obj1_size.z + epsilon) || (obj2_pos.z - epsilon <= obj1_pos.z && obj1_pos.z < obj2_pos.z + obj2_size.z + epsilon))
        )
        return true;
    else return false;
}

// Pega todos os objetos que colidem com outro objeto, dado o índice
//  do objeto na lista de objetos do nível, e a posição à qual ele está indo
vecInt GetObjectsCollidingWithObject(int target_obj_index, vec4 target_obj_pos) {
    vecInt objects_in_position;
    vec3 target_obj_size;

    // Testa se o objeto em questão é o jogador
    if (target_obj_index == -1)
        target_obj_size = vec3(0.01f, 0.6f, 0.01f); // Tamanho do jogador considerado nas colisões
    else target_obj_size = map_objects[target_obj_index].object_size;

    // Varre todos os objetos do nível
    unsigned int obj_index = 0;
    while (obj_index < map_objects.size()) {
    	// O próprio objeto está na lista e deve ser ignorado
        // (se for o player, o index é -1 e este teste sempre falha)
    	if (obj_index == target_obj_index) {
    		obj_index++;
    		continue;
    	}

        vec4 obj_position = map_objects[obj_index].object_position;
        vec3 obj_size = map_objects[obj_index].object_size;

        // Computa valor de tolerância (quanto o objeto pode andar "dentro" do outro objeto, ou quanto ele deve ficar longe)
        float tol = 0.0f;
        if (target_obj_index == -1)
            tol = GetTileToleranceValue(map_objects[obj_index].object_type);

        if (BBoxCollision(target_obj_pos, obj_position, target_obj_size, obj_size, tol))
        	objects_in_position.push_back(obj_index); // Acrescenta o objeto na lista

        obj_index++;
    }

    return objects_in_position;
}

// Pega todos os objetos colidindo com o jogador, dado sua posição
vecInt GetObjectsCollidingWithPlayer(vec4 player_position) {
    return GetObjectsCollidingWithObject(-1, player_position);
}

// Dado um vetor de objetos, retorna o primeiro objeto de um dado tipo
int GetVectorObjectType(vecInt vector_objects, int type) {
    unsigned int curr_index = 0;

    while(curr_index < vector_objects.size()) {
        int current_obj_index = vector_objects[curr_index];

        if (map_objects[current_obj_index].object_type == type)
            return current_obj_index;
        curr_index++;
    }

    return -1;
}

// Alternativa para procurar mais de um tipo de objeto
int GetVectorObjectType(vecInt vector_objects, vecInt types) {
    unsigned int curr_index = 0;

    while(curr_index < vector_objects.size()) {
        int current_obj_index = vector_objects[curr_index];

        for(unsigned int i = 0; i < types.size(); i++)
            if (map_objects[current_obj_index].object_type == types[i])
                return current_obj_index;
        curr_index++;
    }

    return -1;
}

// Dado um vetor, testa se existe um objeto que possa bloquear
//  o movimento de outro objeto (cubo de sujeira ou inimigos p. ex.)
bool vectorHasObjectBlockingObject(vecInt vector_objects, bool is_volleyball) {
    unsigned int curr_index = 0;
	while (curr_index < vector_objects.size()) {
		int curr_obj_index = vector_objects[curr_index];
		int colliding_obj_type = map_objects[curr_obj_index].object_type;

		// Adicione novos objetos bloqueantes aqui
        if ((is_volleyball && isIn(colliding_obj_type, {FLOOR,GRASS,SNOW,DARKDIRT})) ||
            isIn(colliding_obj_type, {WALL, DIRTBLOCK, DIRT, DOOR_RED, DOOR_GREEN, DOOR_YELLOW, DOOR_BLUE,
                                        COW, JET, BEACHBALL, VOLLEYBALL, WOOD, SNOWBLOCK, DARKROCK,CRYSTAL}))
			return true;

        // Caso seja uma bola de volei (quicando), deve-se levar em conta o chão também.

		curr_index++;
	}

	return false;
}

// Função auxiliar, similar acima, mas para a bola de vôlei
bool vectorHasVolleyballBlockingObject(vecInt vector_objects) {
	return vectorHasObjectBlockingObject(vector_objects, true);
}

// Testa se um vetor de objetos possui algum objeto que possa bloquear
//  o movimento do jogador
bool vectorHasPlayerBlockingObject(vecInt vector_objects) {
    unsigned int curr_index = 0;

    // Primeiro verificamos se existem paredes
    while (curr_index < vector_objects.size()) {
        int curr_obj_index = vector_objects[curr_index];
        if (isIn(map_objects[curr_obj_index].object_type, {WALL, WOOD, SNOWBLOCK, DARKROCK, CRYSTAL})) {
            if (map_objects[curr_obj_index].object_type == CRYSTAL)
                g_DeathByEnemy = true;
            return true;
        }
        curr_index++;
    }

    // Depois verificamos portas
    // Só podemos destrancar portas que tiverem se o player não colidir com
    //   NENHUM outro sólido bloqueante
    // Por isso, armazenamos todas as que ele pode trancar e, se não existir,
    //   destranca posteriormente.
    vecInt unlocked_red_doors;
    vecInt unlocked_green_doors;
    vecInt unlocked_blue_doors;
    vecInt unlocked_yellow_doors;

    curr_index = 0;
    while (curr_index < vector_objects.size()) {
        int curr_obj_index = vector_objects[curr_index];

        if (map_objects[curr_obj_index].object_type == DOOR_RED) {
            if (player_inventory.keys.red == 0)
                return true;
            else unlocked_red_doors.push_back(curr_obj_index);
        }
        else if (map_objects[curr_obj_index].object_type == DOOR_GREEN) {
            if (player_inventory.keys.green == 0)
                return true;
            else unlocked_green_doors.push_back(curr_obj_index);
        }
        else if (map_objects[curr_obj_index].object_type == DOOR_BLUE) {
            if (player_inventory.keys.blue == 0)
                return true;
            else unlocked_blue_doors.push_back(curr_obj_index);
        }
        else if (map_objects[curr_obj_index].object_type == DOOR_YELLOW) {
            if (player_inventory.keys.yellow == 0)
                return true;
            else unlocked_yellow_doors.push_back(curr_obj_index);
        }

        curr_index++;
    }

    // Depois verificamos se atingiu a vaca do final do jogo
    curr_index = 0;
    while (curr_index < vector_objects.size()) {
        int curr_obj_index = vector_objects[curr_index];
        if (map_objects[curr_obj_index].object_type == COW) {
            if (player_inventory.cows == g_LevelCowAmount) {
                PlaySound(&winsound);
                g_MapEnded = true;
                return false;
            } else
                return true;
        }

        curr_index++;
    }

    // Depois verificamos se existem blocos que podem ser movimentados
    bool found_dirtblocks = false;
    curr_index = 0;
    while (curr_index < vector_objects.size()) {
        int curr_obj_index = vector_objects[curr_index];
        if (map_objects[curr_obj_index].object_type == DIRTBLOCK) {
            MoveBlock(curr_obj_index);
            found_dirtblocks = true;
        }
        curr_index++;
    }

    // Se existem blocos, o player não pode ser movimentado (embora a chamada para mover os blocos tenha ocorrido)
    if (found_dirtblocks)
        return true;
    else {
        // Se o player não foi bloqueado, destranca quaisquer portas existentes
        UnlockDoors(unlocked_red_doors, unlocked_green_doors, unlocked_blue_doors, unlocked_yellow_doors);
        return false;
    }
}

// Testa se colidiu com inimigos
bool CollidedWithEnemy(vecInt vector_objects) {
    vecInt enemies = {JET, BEACHBALL, VOLLEYBALL};
    return (GetVectorObjectType(vector_objects, enemies) >= 0);
}

// Destranca portas
void UnlockDoors(vecInt red, vecInt green, vecInt blue, vecInt yellow) {
    // Se não existem portas, retorna
    if(red.size() + green.size() + blue.size() + yellow.size() == 0)
        return;

    PlaySound(&doorsound);

    // Remove os itens dos vetores do fim pro início, pra não ter problema de remover coisas erradas

	for (unsigned int i = 0; i < yellow.size(); i++) {
        // Chave amarela é permanente
		map_objects.erase(map_objects.begin() + (yellow[yellow.size() - 1 - i]));
	}

	for (unsigned int i = 0; i < blue.size(); i++) {
		player_inventory.keys.blue--;
		map_objects.erase(map_objects.begin() + (blue[blue.size() - 1 - i]));
	}

	for (unsigned int i = 0; i < green.size(); i++) {
		player_inventory.keys.green--;
		map_objects.erase(map_objects.begin() + (green[green.size() - 1 - i]));
	}

	for (unsigned int i = 0; i < red.size(); i++) {
		player_inventory.keys.red--;
		map_objects.erase(map_objects.begin() + (red[red.size() - 1 - i]));
	}
}

/////////////////////////////
// MOVIMENTAÇÃO DE OBJETOS //
/////////////////////////////

// Função que calcula a posição nova do jogador (ao se movimentar)
void MovePlayer(int theme) {

	/*
	   ALERTA DE GAMBIARRA: as funções de verificação das teclas WASD
	   alteram o módulo dos vetores de direção. Isso é usado para que o
	   player sempre olhe para a direção que está andando, mas que a
	   câmera não necessariamente faça o mesmo (bem típico de jogos em
	   terceira pessoa).
	   Sempre que a tecla relativa à uma direção é pressionada, testa-se se
	   a outra direção está nula.
	 */

    if (key_w_pressed) {
    	straight_vector_sign = 1.0f;
    	if (!key_a_pressed && !key_d_pressed)
    		sideways_vector_sign = 0.0f;
    }
    else if (key_s_pressed) {
    	straight_vector_sign = -1.0f;
    	if (!key_a_pressed && !key_d_pressed)
    		sideways_vector_sign = 0.0f;
    }

    if (key_a_pressed) {
    	sideways_vector_sign = -1.0f;
    	if (!key_s_pressed && !key_w_pressed)
    		straight_vector_sign = 0.0f;
    }
    else if (key_d_pressed) {
    	sideways_vector_sign = 1.0f;
	    if (!key_s_pressed && !key_w_pressed)
	    	straight_vector_sign = 0.0f;
	}

	// Caso alguma tecla tenha sido pressionada, altera a posição do jogador e se testa colisões
    if (key_w_pressed || key_s_pressed || key_d_pressed || key_a_pressed) {

    	// Primeiro testamos se existe uma colisão com sólidos na direção direta do player.
	    vec4 target_pos = player_position + MOVEMENT_AMOUNT * player_direction;
	    vecInt collided_objects = GetObjectsCollidingWithPlayer(target_pos);
	    bool position_blocked = vectorHasPlayerBlockingObject(collided_objects);

	    if (position_blocked) {
	    	// Caso exista, vamos testar na posição reta.
	    	target_pos = player_position + MOVEMENT_AMOUNT * vec4(player_direction.x, 0.0f, 0.0f, 0.0f);
	    	collided_objects = GetObjectsCollidingWithPlayer(target_pos);
	    	position_blocked = vectorHasPlayerBlockingObject(collided_objects);

	    	if (position_blocked) {
	    		// Caso exista, finalmente, testamos a posição lateral.
	    		target_pos = player_position + MOVEMENT_AMOUNT * vec4(0.0f, 0.0f, player_direction.z, 0.0f);
	    		collided_objects = GetObjectsCollidingWithPlayer(target_pos);
	    		position_blocked = vectorHasPlayerBlockingObject(collided_objects);
	    	}
	    }

	    // Se nenhuma delas está livre, o player ficará preso.
	    if (!position_blocked) {
		    player_position = target_pos;

		    int collided_dirt_index = GetVectorObjectType(collided_objects, DIRT);
		    int collided_redkey_index = GetVectorObjectType(collided_objects, KEY_RED);
		    int collided_greenkey_index = GetVectorObjectType(collided_objects, KEY_GREEN);
		    int collided_bluekey_index = GetVectorObjectType(collided_objects, KEY_BLUE);
		    int collided_yellowkey_index = GetVectorObjectType(collided_objects, KEY_YELLOW);
		    int collided_baby_index = GetVectorObjectType(collided_objects, BABYCOW);

		    if (CollidedWithEnemy(collided_objects)) {
		    	g_DeathByEnemy = true;
		    }
		    else if (GetVectorObjectType(collided_objects, WATER) >= 0) {
		        g_DeathByWater = true;
		    } else if (collided_dirt_index >= 0) {
                switch(theme){
                    case 0:
                        map_objects[collided_dirt_index].object_type = FLOOR;
                        break;
                    case 1:
                        map_objects[collided_dirt_index].object_type = GRASS;
                        break;
                    case 2:
                        map_objects[collided_dirt_index].object_type = DARKFLOOR;
                        break;
                    case 3:
                        map_objects[collided_dirt_index].object_type = SNOW;
                        break;
                    case 4:
                        map_objects[collided_dirt_index].object_type = DARKDIRT;
                        break;
                    default:
                        map_objects[collided_dirt_index].object_type = FLOOR;
                }
		    } else if (collided_redkey_index >= 0) {
		        sound.setBuffer(keysound);
                sound.play();
		    	map_objects.erase(map_objects.begin() + collided_redkey_index);
		    	player_inventory.keys.red++;
		    } else if (collided_greenkey_index >= 0) {
		        sound.setBuffer(keysound);
                sound.play();
		    	map_objects.erase(map_objects.begin() + collided_greenkey_index);
		    	player_inventory.keys.green++;
		    } else if (collided_bluekey_index >= 0) {
		        sound.setBuffer(keysound);
                sound.play();
		    	map_objects.erase(map_objects.begin() + collided_bluekey_index);
		    	player_inventory.keys.blue++;
		    } else if (collided_yellowkey_index >= 0) {
		        sound.setBuffer(keysound);
                sound.play();
		    	map_objects.erase(map_objects.begin() + collided_yellowkey_index);
		    	player_inventory.keys.yellow++;
		    } else if (collided_baby_index >= 0) {
		        sound.setBuffer(cowsound);
                sound.play();
		    	map_objects.erase(map_objects.begin() + collided_baby_index);
		    	player_inventory.cows++;
		    }
		}
	}
}

// Função que move um bloco
// Ao mover o bloco, testa-se colisão também.
void MoveBlock(int block_index) {
    vec4 direction = map_objects[block_index].object_position - player_position;
    direction.y = 0.0f;

    vec4 target_pos = map_objects[block_index].object_position;

    // Computa a direção para onde se movimentar o bloco
    float angle = acos(dotproduct(direction, vec4(1.0f,0.0f,0.0f,0.0f))/norm(direction));
    if (direction.z > 0.0f)
        angle = - angle;
    angle = angle + PI;

    if (angle >= PI/4 && angle < 3*PI/4) {
        target_pos.z += MOVEMENT_AMOUNT;
    } else if (angle >= 3*PI/4 && angle < 5*PI/4) {
        target_pos.x += MOVEMENT_AMOUNT;
    } else if (angle >= 5*PI/4 && angle < 7*PI/4) {
        target_pos.z -= MOVEMENT_AMOUNT;
    } else {
        target_pos.x -= MOVEMENT_AMOUNT;
    }

    vecInt collided_objects = GetObjectsCollidingWithObject(block_index, target_pos);

    if (!vectorHasObjectBlockingObject(collided_objects)) {
        map_objects[block_index].object_position = target_pos;

        // Testa se atingiu água. Se atingiu, transforma-a em sujeira.
        int water_index = GetVectorObjectType(collided_objects, WATER);
        if (water_index >= 0) {
            PlaySound(&splashsound);
            map_objects[block_index].object_position = map_objects[water_index].object_position;
            map_objects[water_index].object_type = DIRT;
            map_objects.erase(map_objects.begin() + block_index);
        }
    }
}

// Movimenta todos os inimigos em um nível
void MoveEnemies() {
    // Varre o vetor de objetos, movimentando inimigos existentes
    for (unsigned int i = 0; i < map_objects.size(); i++) {
        if (map_objects[i].object_type == JET)
            MoveJet(i);
        else if (map_objects[i].object_type == BEACHBALL)
            MoveBeachBall(i);
        else if (map_objects[i].object_type == VOLLEYBALL)
            MoveVolleyBall(i);
    }
}

// Função de movimentação do jato
void MoveJet(int jet_index) {
    // Calcula para onde o jet deve andar
    vec4 target_pos = map_objects[jet_index].object_position;
    switch(map_objects[jet_index].direction) {
        case 0: {
            target_pos.z += MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
        case 1: {
            target_pos.x += MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
        case 2: {
            target_pos.z -= MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
        case 3: {
            target_pos.x -= MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
    }

    // Testa colisões
    vecInt collided_objects = GetObjectsCollidingWithObject(jet_index, target_pos);
    if (!vectorHasObjectBlockingObject(collided_objects)) {
        map_objects[jet_index].object_position = target_pos;

        // Se colidiu com o player, mata ele
        vec3 player_size = vec3(0.0f, 0.6f, 0.0f);
        if (BBoxCollision(player_position, target_pos, player_size, map_objects[jet_index].object_size, 0.0f))
            g_DeathByEnemy = true;

        // Testa se atingiu fogo. Se atingiu, morre
        int fire_index = GetVectorObjectType(collided_objects, FIRE);
        if (fire_index >= 0) {
            map_objects.erase(map_objects.begin() + jet_index);
        }
    }
    // Se colidiu, recomputa direção
    else {
        map_objects[jet_index].direction = (map_objects[jet_index].direction + 1) % 4;
    }
}

// Função de movimentação da bola de praia
// Muito similar acima, mas possui algumas modificações e por isso
//  não foi refatorada (direção, morte em água)
void MoveBeachBall(int ball_index) {
    vec4 target_pos = map_objects[ball_index].object_position;

    switch(map_objects[ball_index].direction) {
        case 0: {
            target_pos.z += MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
        case 1: {
            target_pos.x += MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
        case 2: {
            target_pos.z -= MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
        case 3: {
            target_pos.x -= MOVEMENT_AMOUNT + ENEMY_SPEED;
            break;
        }
    }

    vecInt collided_objects = GetObjectsCollidingWithObject(ball_index, target_pos);

    if (!vectorHasObjectBlockingObject(collided_objects)) {
        map_objects[ball_index].object_position = target_pos;

        // Se colidiu com o player, mata ele
        vec3 player_size = vec3(0.0f, 0.6f, 0.0f);
        if (BBoxCollision(player_position, target_pos, player_size, map_objects[ball_index].object_size, 0.0f))
            g_DeathByEnemy = true;

        // Testa se atingiu fogo ou água. Se atingiu, morre.
        int hazard_index = GetVectorObjectType(collided_objects, vecInt(FIRE,WATER));
        if (hazard_index >= 0) {
            map_objects.erase(map_objects.begin() + ball_index);
        }
    }
    else map_objects[ball_index].direction = (map_objects[ball_index].direction + 2) % 4;
}

// Função de movimentação da bola de vôlei
void MoveVolleyBall(int ball_index) {
    vec4 target_pos = map_objects[ball_index].object_position;

    // Ajuste da aceleração de gravidade para a bola
    target_pos.y -= map_objects[ball_index].gravity;
    if (map_objects[ball_index].gravity < 0.2f)
        map_objects[ball_index].gravity += 0.005f;

    vecInt collided_objects = GetObjectsCollidingWithObject(ball_index, target_pos);

    if (!vectorHasVolleyballBlockingObject(collided_objects)) {
        map_objects[ball_index].object_position = target_pos;

        // Se colidiu com o player, mata ele
        vec3 player_size = vec3(0.0f, 0.6f, 0.0f);
        if (BBoxCollision(player_position, target_pos, player_size, map_objects[ball_index].object_size, 0.0f))
            g_DeathByEnemy = true;

        // Testa se atingiu fogo ou água. Se atingiu, morre.
        int hazard_index = GetVectorObjectType(collided_objects, vecInt(FIRE,WATER));
        if (hazard_index >= 0) {
            map_objects.erase(map_objects.begin() + ball_index);
        }
    }
    else {
        // Quica
        PlaySound(&ball1sound);
        map_objects[ball_index].gravity = -0.2f;
    }
}

/////////////////////////////
// AUXILIARES PARA DESENHO //
/////////////////////////////

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

    vecInt num_triangles_per_vertex(num_vertices, 0);
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

///////////////////////////
// SISTEMA DE PARTÍCULAS //
///////////////////////////

// Animação
void AnimateParticles() {
    vecInt particles_to_delete;

    for(unsigned int i = 0; i < particles.size(); i++) {
        particles[i].position.y = particles[i].position.y + particles[i].speed;
        particles[i].life =  particles[i].life - particles[i].speed;
        if (particles[i].life <= 0)
            particles_to_delete.push_back(i);
    }

    for(unsigned int i = 0; i < particles_to_delete.size(); i++) {
        particles.erase(particles.begin() + particles_to_delete[i]);
    }
}

// Geração
void GenerateParticles(int amount, vec4 position, vec3 object_size) {
    for (int i = 0; i < amount; i++) {
        Particle new_particle;
        float x_start = position.x - object_size.x / 2;
        float x_end = position.x + object_size.x / 2;
        float z_start = position.z - object_size.z / 2;
        float z_end = position.z + object_size.z / 2;
        float pos_x = x_start + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(x_end-x_start)));
        float pos_z = z_start + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(z_end-z_start)));
        float pos_y = position.y;
        float yellow = 0.0 + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(0.6-0.0)));
        new_particle.position = vec4(pos_x, pos_y, pos_z, 1.0f);
        new_particle.speed = 0.02f;
        new_particle.color = vec3(1.0f, yellow, 0.0f);
        new_particle.life = 1.0f;
        new_particle.size = 0.01f + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(0.05f-0.01f)));
        particles.push_back(new_particle);
    }
}

// Desenho
// Talvez precisamos mudar isso.
void DrawParticles() {
    glDisable(GL_CULL_FACE);
    for(unsigned int i = 0; i < particles.size(); i++) {
        glUniform1i(yellow_particle_color_uniform, particles[i].color.y * 10);

        glm::mat4 model = Matrix_Translate(particles[i].position.x, particles[i].position.y, particles[i].position.z)
                        * Matrix_Scale(particles[i].size, particles[i].size, particles[i].size);

        DrawVirtualObject("sphere", PARTICLE, model);
    }
}

///////////////////
// CARREGAMENTOS //
///////////////////

// Função que carrega um nível a partir de um arquivo (parser)
Level LoadLevelFromFile(string filepath) {
    Level loaded_level;

    printf("Carregando nivel \"%s\"... ", filepath.c_str());

    std::ifstream level_file;
    level_file.open(filepath);
    if (!level_file.is_open()) {
        throw std::runtime_error("Erro ao abrir arquivo.");
    }
    else {
        try {
            // Salva largura e altura na estrutura do nível.
            string width, height, cow_no, time, theme;
            getline(level_file, cow_no);
            getline(level_file, time);
            getline(level_file, theme);
            getline(level_file, width);
            getline(level_file, height);
            loaded_level.cow_no = atoi(cow_no.c_str());
            loaded_level.time = atoi(time.c_str());
            loaded_level.theme = atoi(theme.c_str());
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
                std::vector<string> map_line;

                while (col < loaded_level.width) {
                    string tile;
                    for(int tilesize = 0; tilesize < 2; tilesize++) {
                        tile += file_line[col * 3 + tilesize];
                    }
                    map_line.push_back(tile);
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
    skytheme_uniform        = glGetUniformLocation(program_id, "skytheme");
    yellow_particle_color_uniform = glGetUniformLocation(program_id, "yellow_particle_color");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(program_id);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage0"), 0);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage1"), 1);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage2"), 2);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage3"), 3);
    glUniform1i(glGetUniformLocation(program_id, "TextureImage4"), 4);

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

// Carrega um arquivo de som
void LoadSoundFromFile(const char* path, sf::SoundBuffer * buffer) {
    printf("Carregando som \"%s\"... ", path);
    if (!buffer->loadFromFile(path)) {
        printf("Falha ao carregar som!\n");
        std::exit(EXIT_FAILURE);
    }
    else printf(" OK!\n");
}

// Carrega um arquivo de música
void LoadMusicFromFile(const char* path, sf::Music * buffer) {
    printf("Carregando música \"%s\"... ", path);
    if (!buffer->openFromFile(path)) {
        printf("Falha ao carregar música!\n");
        std::exit(EXIT_FAILURE);
    }
    else {
        printf(" OK!\n");
        buffer->setLoop(true);
    }
}

///////////
// AUDIO //
///////////

// Toca a música de um nível
void PlayLevelMusic(int level_number) {
    if (!g_MusicOn)
        return;

    if(menumusic.getStatus() == 2)
        menumusic.stop();
    switch(level_number){
        case 1:
        case 2:{
            if(techmusic.getStatus() != 2)
                techmusic.play();
            break;
        }
        case 3:{
            if(naturemusic.getStatus() != 2)
                naturemusic.play();
            break;
        }
        case 4:{
            if(watermusic.getStatus() != 2)
                watermusic.play();
            break;
        }
        case 5:{
            if(crystalmusic.getStatus() != 2)
                crystalmusic.play();
            break;
        }
    }
}

// Toca a música do menu
void PlayMenuMusic() {
    if (!g_MusicOn)
        return;

    if (techmusic.getStatus() == 2)
        techmusic.stop();
    if (naturemusic.getStatus() == 2)
        naturemusic.stop();
    if (watermusic.getStatus() == 2)
        watermusic.stop();
    if (crystalmusic.getStatus() == 2)
        crystalmusic.stop();
    menumusic.play();
}

// Para todas as músicas
void StopAllMusic() {
    if (techmusic.getStatus() == 2)
        techmusic.stop();
    if (naturemusic.getStatus() == 2)
        naturemusic.stop();
    if (watermusic.getStatus() == 2)
        watermusic.stop();
    if (crystalmusic.getStatus() == 2)
        crystalmusic.stop();
    if (menumusic.getStatus() == 2)
        menumusic.stop();
}

// Toca um som
void PlaySound(sf::SoundBuffer * buffer) {
    if (!g_SoundsOn)
        return;

    sound.setBuffer(*buffer);
    sound.play();
}

//////////////////
// CONTROLE GPU //
//////////////////

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

	if (g_CurrentScreen == SCREEN_MAINMENU || g_CurrentScreen == SCREEN_LEVELSELECT || g_ShowingMessage)
		return;

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
        float phimax = PI/2 - 0.2f;
        float phimin = 0;

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

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    if (key == GLFW_KEY_C && action == GLFW_PRESS && g_CurrentScreen == SCREEN_GAME) {
        g_ChangedCamera = true;
        g_useFirstPersonCamera = !(g_useFirstPersonCamera);
    }

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

    if (key == GLFW_KEY_R && action == GLFW_PRESS) {
        key_r_pressed = true;
    }

    if (key == GLFW_KEY_M && action == GLFW_PRESS) {
        g_MusicOn = !g_MusicOn;
        if (!g_MusicOn)
            StopAllMusic();
        else {
            if (g_CurrentScreen != SCREEN_GAME)
                PlayMenuMusic();
            else PlayLevelMusic(g_CurrentLevel);
        }
    }

    if (key == GLFW_KEY_N && action == GLFW_PRESS) {
        g_SoundsOn = !g_SoundsOn;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
    	esc_pressed = true;
    }

    if (key == GLFW_KEY_ESCAPE && action == GLFW_RELEASE)
    {
    	esc_pressed = false;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
    	key_space_pressed = true;
    }

    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
    {
    	key_space_pressed = false;
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

///////////////////////////
// RENDERIZAÇÃO DE TEXTO //
///////////////////////////

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
