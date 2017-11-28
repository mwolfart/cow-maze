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
#define PLANE  0
#define COW    1
#define BUNNY  2
#define SPHERE 3
#define CUBE   4
uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec3 color;

// Constantes
#define M_PI   3.14159265358979323846
#define M_PI_2 1.57079632679489661923

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
    vec4 l = normalize(vec4(1.0,1.0,0.0,0.0));

    // Vetor que define o sentido da câmera em relação ao ponto atual.
    vec4 v = normalize(camera_position - p);

    // Coordenadas de textura U e V
    float U = 0.0;
    float V = 0.0;

    // Equação de Iluminação
    float lambert = max(0,dot(n,l));

    if ( object_id == SPHERE )
    {
        vec4 bbox_center = (bbox_min + bbox_max) / 2.0;
        vec4 p_vector = position_model - bbox_center;

        float px = position_model[0];
        float py = position_model[1];
        float pz = position_model[2];

        float rho = sqrt(pow(px,2) + pow(py,2) + pow(pz, 2));
        float theta = atan(px, pz);
        float phi = asin(py/rho);

        U = (theta + M_PI) / (2 * M_PI);
        V = (phi + M_PI/2) / M_PI;

        vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        color = Kd0 * (lambert + 0.01);
    }
    else if ( object_id == BUNNY || object_id == COW )
    {
        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        U = (position_model[0] - minx)/(maxx - minx);
        V = (position_model[1] - miny)/(maxy - miny);

        vec3 Kd0 = texture(TextureImage2, vec2(U,V)).rgb;
        color = Kd0;
    }
    else if ( object_id == PLANE )
    {
        int number_of_repetitions = 32;
        float period = 1.0f/number_of_repetitions;
        U = mod(texcoords.x, period)*number_of_repetitions;
        V = mod(texcoords.y, period)*number_of_repetitions;

        vec3 Kd0 = texture(TextureImage0, vec2(U,V)).rgb;
        color = Kd0 * (lambert + 0.01);
    }
    else if ( object_id == CUBE )
    {
        U = texcoords.x;
        V = texcoords.y;

        vec3 Kd1 = texture(TextureImage1, vec2(U,V)).rgb;
        color = Kd1;
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec3(1.0,1.0,1.0)/2.2);
}
