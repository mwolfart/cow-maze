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
#define GRASS       25
#define WOOD        26
#define SNOW        27
#define DARKFLOOR   28

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

#define SKYBOX_TOP      100
#define SKYBOX_BOTTOM   101
#define SKYBOX_EAST     102
#define SKYBOX_WEST     103
#define SKYBOX_SOUTH    104
#define SKYBOX_NORTH    105

uniform int object_id;

// Parâmetros da axis-aligned bounding box (AABB) do modelo
uniform vec4 bbox_min;
uniform vec4 bbox_max;

// Variáveis para acesso das imagens de textura
uniform sampler2D TextureImage0;
uniform sampler2D TextureImage1;
uniform sampler2D TextureImage2;

#define WALLGROUNDGRASS_W 841
#define WALLGROUNDGRASS_H 305


// Variável de controle da animação
uniform int anim_timer;

uniform int yellow_particle_color;

uniform int skytheme;

// O valor de saída ("out") de um Fragment Shader é a cor final do fragmento.
out vec4 color;

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

        U = (U + 3)/ 4;
        V = (V + 0)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        Ks = vec4(0.5, 0.5, 0.5, 0.0f);
        Ka = texture(TextureImage0, vec2(U,V)).rgba;
        q = 5.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    }
    else if ( object_id == FLOOR )
    {
        U = (texcoords.x + 0)/ 4;
        V = (texcoords.y + 3)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == WATER )
    {
        int xpos = 0;
        int ypos = 0;

        switch (anim_timer) {
        case 0: case 1: case 2: case 3: {ypos = 3; break; }
        case 4: case 5: case 6: case 7: {ypos = 2; break; }
        case 8: case 9: case 10: case 11: {ypos = 1; break; }
        case 12: case 13: case 14: case 15: {ypos = 0; break; }
        }

        switch (anim_timer) {
        case 0: case 4: case 8: case 12: {xpos = 0; break; }
        case 1: case 5: case 9: case 13: {xpos = 1; break; }
        case 2: case 6: case 10: case 14: {xpos = 2; break; }
        case 3: case 7: case 11: case 15: {xpos = 3; break; }
        }

        U = (texcoords.x + xpos)/4;
        V = (texcoords.y + ypos)/4;

        Kd = texture(TextureImage1, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == WALL )
    {
        U = (texcoords.x + 1)/4;
        V = (texcoords.y + 3)/4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == DIRT )
    {
        U = (texcoords.x + 3)/ 4;
        V = (texcoords.y + 3)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == GRASS ) 
    {
        U = (texcoords.x + 2)/ 4;
        V = (texcoords.y + 3)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == WOOD )
    {
        U = (texcoords.x + 2)/ 4;
        V = (texcoords.y + 0)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == DIRTBLOCK )
    {
        U = (texcoords.x + 0)/ 4;
        V = (texcoords.y + 2)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    }
    else if ( object_id == DOOR_RED ) {
        float x = position_model[0];
        float y = position_model[1];
        float z = position_model[2];

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        if (y < (maxy - 0.01) && y > (miny+0.01)) {
            U = (texcoords.x + 2)/ 4;
            V = (texcoords.y + 2)/ 4;
        } else {
            U = (texcoords.x + 2)/ 4;
            V = (texcoords.y + 1)/ 4;
        }

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == DOOR_GREEN ) {
        float x = position_model[0];
        float y = position_model[1];
        float z = position_model[2];

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        if (y < (maxy - 0.01) && y > (miny+0.01)) {
            U = (texcoords.x + 3)/ 4;
            V = (texcoords.y + 2)/ 4;
        } else {
            U = (texcoords.x + 2)/ 4;
            V = (texcoords.y + 1)/ 4;
        }

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == DOOR_BLUE ) {
        float x = position_model[0];
        float y = position_model[1];
        float z = position_model[2];

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        if (y < (maxy - 0.01) && y > (miny+0.01)) {
            U = (texcoords.x + 0)/ 4;
            V = (texcoords.y + 1)/ 4;
        } else {
            U = (texcoords.x + 2)/ 4;
            V = (texcoords.y + 1)/ 4;
        }

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == DOOR_YELLOW ) {
        float x = position_model[0];
        float y = position_model[1];
        float z = position_model[2];

        float minx = bbox_min.x;
        float maxx = bbox_max.x;

        float miny = bbox_min.y;
        float maxy = bbox_max.y;

        float minz = bbox_min.z;
        float maxz = bbox_max.z;

        if (y < (maxy - 0.01) && y > (miny+0.01)) {
            U = (texcoords.x + 1)/ 4;
            V = (texcoords.y + 1)/ 4;
        } else {
            U = (texcoords.x + 2)/ 4;
            V = (texcoords.y + 1)/ 4;
        }

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        color = Kd;
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

        U = (U + 3)/ 4;
        V = (V + 1)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        Ks = vec4(0.5, 0.5, 0.7, 1.0f);
        Ka = texture(TextureImage0, vec2(U,V)).rgba;
        q = 32.0;

        vec4 lambert_diffuse_term = Kd * I * max(0, dot(n, l));
        vec4 ambient_term = Ka * Ia;
        vec4 phong_specular_term  = Ks * I * pow((max(0, dot(r, v))), q);
        color = lambert_diffuse_term + ambient_term + phong_specular_term;
    } else if ( object_id == VOLLEYBALL ) {
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

        U = (U + 0)/ 4;
        V = (V + 0)/ 4;

        Kd = texture(TextureImage0, vec2(U,V)).rgba;
        Ks = vec4(0.5, 0.5, 0.7, 1.0f);
        Ka = texture(TextureImage0, vec2(U,V)).rgba;
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
    else if ( object_id == SKYBOX_TOP ) {
        U = (texcoords.x + 1)/ 3;
        V = (texcoords.y + 1)/ 2;

        if (skytheme == 1)
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == SKYBOX_BOTTOM ) {
        U = (texcoords.x + 0)/ 3;
        V = (texcoords.y + 1)/ 2;

        if (skytheme == 1)
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == SKYBOX_WEST ) {
        U = (texcoords.x + 2)/ 3;
        V = (texcoords.y + 1)/ 2;

        if (skytheme == 1)
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == SKYBOX_EAST ) {
        U = (texcoords.x + 1)/ 3;
        V = (texcoords.y + 0)/ 2;
    
        if (skytheme == 1)
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == SKYBOX_SOUTH ) {
        U = (texcoords.x + 2)/ 3;
        V = (texcoords.y + 0)/ 2;
    
        if (skytheme == 1)
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        color = Kd;
    } else if ( object_id == SKYBOX_NORTH ) {
        U = (texcoords.x + 0)/ 3;
        V = (texcoords.y + 0)/ 2;

        if (skytheme == 1)
            Kd = texture(TextureImage2, vec2(U,V)).rgba;
        color = Kd;
    }

    // Cor final com correção gamma, considerando monitor sRGB.
    // Veja https://en.wikipedia.org/w/index.php?title=Gamma_correction&oldid=751281772#Windows.2C_Mac.2C_sRGB_and_TV.2Fvideo_standard_gammas
    color = pow(color, vec4(1.0,1.0,1.0,1.0)/2.2);
}