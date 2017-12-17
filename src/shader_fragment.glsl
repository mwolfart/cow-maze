#version 330 core

// Atributos de fragmentos recebidos como entrada ("in") pelo Fragment Shader.
// Neste exemplo, este atributo foi gerado pelo rasterizador como a
// interpolação da cor de cada vértice, definidas em "shader_vertex.glsl" e
// "main.cpp".
in vec4 position_world;
in vec4 normal;

// Posição do vértice atual no sistema de coordenadas local do modelo.
in vec4 position_model;

// Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
in vec2 texcoords;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Identificador que define qual objeto está sendo desenhado no momento
#define COW         1
#define WALL        10
#define LOCK        11
#define DIRTBLOCK   12
#define FLOOR       13
#define DIRT        14
#define WATER       15
#define LAVA        16
#define DOOR_RED    17
#define DOOR_GREEN  18
#define DOOR_BLUE   19
#define DOOR_YELLOW 20
#define BABYCOW     21
#define JET         22
#define BEACHBALL   23
#define VOLLEYBALL  24

#define KEY_RED     30
#define KEY_GREEN   31
#define KEY_BLUE    32
#define KEY_YELLOW  33

#define PLAYER_HEAD     60
#define PLAYER_TORSO    61
#define PLAYER_ARM      62
#define PLAYER_HAND     63
#define PLAYER_LEG      64
#define PLAYER_FOOT     65

#define PARTICLE    80

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;
uniform sampler2D TextureImage3;
uniform sampler2D TextureImage4;
uniform sampler2D TextureImage5;
uniform sampler2D TextureImage6;
uniform sampler2D TextureImage7;
uniform sampler2D TextureImage8;
uniform sampler2D TextureImage9;

#define WALLGROUNDGRASS_W 841
#define WALLGROUNDGRASS_H 305


// Variável de controle da animação
uniform int anim_timer;

uniform int yellow_particle_color;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

int getPx(int indexX, int image_width, int n_tiles_horiz)
{
    int tile_width = image_width / n_tiles_horiz;
    int pos_x = indexX * tile_width;
    return pos_x;
}

int getPy(int indexY, int image_height, int n_tiles_vert)
{
    int tile_height = image_height / n_tiles_vert;
    int pos_y = indexY * tile_height;
    return pos_y;
}

void main()
{
    // Obtemos a posição da câmera utilizando a inversa da matriz que define o
    // sistema de coordenadas da câmera.
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;


    // O fragmento atual é coberto por um ponto que percente à superfície de um
    // dos objetos virtuais da cena. Este ponto, p, possui uma posição no
    // sistema de coordenadas global (World coordinates). Esta posição é obtida
    // através da interpolação, feita pelo rasterizador, da posição de cada
    // vértice.
    vec4 p = position_world;

    // Normal do fragmento atual, interpolada pelo rasterizador a partir das
    // normais de cada vértice.
    vec4 n = normalize(normal);

    // Vetor que define o sentido da fonte de luz em relação ao ponto atual.
    vec4 l = normalize(vec4(1.0,1.0,1.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Vetor que define o sentido da reflexão especular ideal.
    vec4 r = -l + 2*n*(dot(n, l));

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

    // Fonte de luz - meio do nível
    //  usada nos objetos
    vec4 light = normalize(vec4(16.0,10.0,16.0,0.0));

    // Parâmetros que definem as propriedades espectrais da superfície
    vec4 Kd; // Refletância difusa
    vec4 Ks; // Refletância especular
    vec4 Ka; // Refletância ambiente
    float q; // Expoente especular para o modelo de iluminação de Phong

    vec4 I = vec4(1.0,1.0,1.0,1.0f);
    vec4 Ia = vec4(0.5,0.5,0.5,1.0f);

    if ( object_id == COW )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model[0] - minx)/(maxx - minx);
        V = (position_model[1] - miny)/(maxy - miny);

        Kd = texture(TextureImage2, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == FLOOR )
    {
        U = texcoords.x;
        V = texcoords.y;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == WATER )
    {
        U = texcoords.x;
        V = texcoords.y;

        switch (anim_timer) {
        case 0: { Kd = texture(TextureImage3, vec2(U,V)).rgba; break; }
        case 1: { Kd = texture(TextureImage4, vec2(U,V)).rgba; break; }
        case 2: { Kd = texture(TextureImage5, vec2(U,V)).rgba; break; }
        case 3: { Kd = texture(TextureImage6, vec2(U,V)).rgba; break; }
        }

        color = Kd;
    }
    else if ( object_id == WALL )
    {
        U = texcoords.x;
        V = texcoords.y;

        Kd = texture(TextureImage1, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == DIRT )
    {
        U = texcoords.x;
        V = texcoords.y;

        Kd = texture(TextureImage7, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == DIRTBLOCK )
    {
        U = texcoords.x;
        V = texcoords.y;

        Kd = texture(TextureImage8, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == DOOR_RED ) {
        color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    } else if ( object_id == DOOR_GREEN ) {
        color = vec4(0.0f, 1.0f, 0.0f, 1.0f);
    } else if ( object_id == DOOR_BLUE ) {
        color = vec4(0.0f, 0.0f, 1.0f, 1.0f);
    } else if ( object_id == DOOR_YELLOW ) {
        color = vec4(1.0f, 1.0f, 0.0f, 1.0f);
    }
    else if ( object_id == KEY_RED ) {
        Kd = vec4(0.8f, 0.0f, 0.0f, 1.0f);
        Ks = vec4(0.5,0.5,0.5, 1.0f);
        Ka = vec4(0.4f, 0.0f, 0.0f, 1.0f);
        q = 64.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    }
    else if ( object_id == KEY_GREEN ) {
        Kd = vec4(0.0f, 0.8f, 0.0f, 1.0f);
        Ks = vec4(0.5,0.5,0.5, 1.0f);
        Ka = vec4(0.0f, 0.4f, 0.0f, 1.0f);
        q = 64.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    }
    else if ( object_id == KEY_BLUE ) {
        Kd = vec4(0.0f, 0.0f, 0.8f, 1.0f);
        Ks = vec4(0.5,0.5,0.5, 1.0f);
        Ka = vec4(0.0f, 0.0f, 0.4f, 1.0f);
        q = 64.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    }
    else if ( object_id == KEY_YELLOW ) {
        Kd = vec4(0.8f, 0.8f, 0.0f, 1.0f);
        Ks = vec4(0.5,0.5,0.5, 1.0f);
        Ka = vec4(0.4f, 0.2f, 0.0f, 1.0f);
        q = 64.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    }
    else if ( object_id == BABYCOW ) {
        Kd = vec4(0.3f, 0.5f, 0.8f, 1.0f);
        Ks = vec4(0.5,0.5,0.5, 1.0f);
        Ka = vec4(0.1f, 0.2f, 0.3f, 1.0f);
        q = 32.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    } else if ( object_id == PARTICLE ) {
        color = vec4(1.0f, yellow_particle_color/10.0f, 0.0f, 0.1f);
    } else if ( object_id == JET ) {
        Kd = vec4(0.2f, 0.3f, 0.4f, 1.0f);
        Ks = vec4(0.5, 0.5, 0.7, 1.0f);
        Ka = vec4(0.1f, 0.1f, 0.1f, 1.0f);
        q = 32.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    } else if ( object_id == BEACHBALL ) {
        U = texcoords.x;
        V = texcoords.y;

        Kd = texture(TextureImage7, vec2(U,V)).rgba;
        Ks = vec4(0.5, 0.5, 0.7, 1.0f);
        Ka = vec4(0.4f, 0.4f, 0.4f, 1.0f);
        q = 32.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    } else if ( object_id == VOLLEYBALL ) {
        U = texcoords.x;
        V = texcoords.y;

        Kd = texture(TextureImage6, vec2(U,V)).rgba;
        Ks = vec4(0.5, 0.5, 0.7, 1.0f);
        Ka = vec4(0.4f, 0.4f, 0.4f, 1.0f);
        q = 32.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    }
    else if ( object_id == PLAYER_HEAD || object_id == PLAYER_FOOT || object_id == PLAYER_HAND) {
        color = vec4(0.85f, 0.8f, 0.5f, 1.0f);
    }
    else if ( object_id == PLAYER_ARM || object_id == PLAYER_TORSO ) {
        color = vec4(0.05f, 0.4f, 0.1f, 1.0f);
    }
    else if ( object_id == PLAYER_LEG ) {
        color = vec4(0.4f, 0.3f, 0.1f, 1.0f);
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec4(1.0,1.0,1.0,1.0)/2.2);
}