#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"


const int width = 256*4;
const int height = 256*3;
const int scale = 1;
uint ticks = 0;
application app = {NULL, NULL};
const Uint8* keyboard_state_array;
float ms_frame = 16.666;
float z_buffer[width*height];
uint8_t scene[3*width*height];
SDL_Texture* texture;


typedef struct {
    float x;
    float y;
    float z;
    float w;
} vector_4d; //homogeneous vector

typedef struct {
    vector_4d cols[4];
} sq_mat_4d;

typedef struct {
    vector_4d look_from;
    vector_4d look_dir;
    vector_4d up_dir;
    vector_4d right_dir;
    
    float speed;
} camera;

camera cam;
sq_mat_4d view_mat;
sq_mat_4d pers_proj_mat;
sq_mat_4d pipeline_mat;



typedef struct {
    vector_4d v_0;
    vector_4d v_1;
    vector_4d v_2; 
    vector_4d normal; 
} triangle; //surface normals for each vector?

typedef struct {
    triangle* tri;
    int n_tri;
} mesh;

//object space (.stl coords)
//world space (world coord system) NOT DONE

//then view transform to get to view (camera) space (looking negative z) 
//last two steps combined to modelview matrix, view matrix is really just
//translation + change of basis to the camera's basis

//then projection matrix to get to clip space (implement both perspective and orthographic) 

//clip
//then perspective divide (by w) to get normalized device space (3d vector)

//z buffer
//finally window transform

//calculate inverses for all matrix

void print_vector_4d(vector_4d a){
    printf("%f %f %f %f\n",a.x,a.y,a.z,a.w);
}

float dotprod_4d(vector_4d a, vector_4d b){
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}
float dotprod_3d(vector_4d a, vector_4d b){
    return a.x * b.x + a.y * b.y + a.z * b.z ;
}

float distance_hom(vector_4d a){
    return sqrt( a.x*a.x +a.y*a.y + a.z*a.z);
}

vector_4d scale_hom_vector(vector_4d a, float s){
    vector_4d b ={a.x*s, a.y*s, a.z*s, a.w};
    return b;
}

void inplace_scale_hom_vector(vector_4d* a, float s){
    a->x*=s;
    a->y*=s;
    a->z*=s;
}

void unit_hom_vector(vector_4d* a){
    float s = 1.0f / distance_hom(*a);
    inplace_scale_hom_vector(a, s);
}

void inplace_add_hom_vec(vector_4d* a, vector_4d* b){
    a->x+=b->x;
    a->y+=b->y;
    a->z+=b->z;
}

void inplace_sub_hom_vec(vector_4d* a, vector_4d* b){
    a->x-=b->x;
    a->y-=b->y;
    a->z-=b->z;
}

vector_4d add_hom_vec(vector_4d a, vector_4d b){
    vector_4d c = {a.x+b.x, a.y+b.y, a.z+b.z, 0};
    return c;
}

vector_4d sub_hom_vec(vector_4d a, vector_4d b){
    vector_4d c = {a.x-b.x, a.y-b.y, a.z-b.z, 0};
    return c;
}

vector_4d get_unit_hom_vector(float x, float y, float z){
    vector_4d a = {x,y,z,0};
    unit_hom_vector(&a);
    return a;
}

vector_4d cross_product_hom_4d(vector_4d a, vector_4d b){
    vector_4d c;
    c.x = a.y*b.z - a.z*b.y;
    c.y = a.z*b.x - a.x*b.z;
    c.z = a.x*b.y - a.y*b.x;
    c.w = 0;
    return c;
}

sq_mat_4d transpose_mat_4d(sq_mat_4d A){
    sq_mat_4d T;
    T.cols[0] = (vector_4d) {A.cols[0].x, A.cols[1].x, A.cols[2].x, A.cols[3].x};
    T.cols[1] = (vector_4d) {A.cols[0].y, A.cols[1].y, A.cols[2].y, A.cols[3].y};
    T.cols[2] = (vector_4d) {A.cols[0].z, A.cols[1].z, A.cols[2].z, A.cols[3].z};
    T.cols[3] = (vector_4d) {A.cols[0].w, A.cols[1].w, A.cols[2].w, A.cols[3].w};
    return T;
}

void inplace_matvec_mul_4d(sq_mat_4d A, vector_4d* b){
    sq_mat_4d A_t = transpose_mat_4d(A);
    vector_4d v;
    v.x = dotprod_4d(A_t.cols[0], *b);
    v.y = dotprod_4d(A_t.cols[1], *b);
    v.z = dotprod_4d(A_t.cols[2], *b);
    v.w = dotprod_4d(A_t.cols[3], *b);
    *b = v;
};

vector_4d matvec_mul_4d(sq_mat_4d A, vector_4d b){
    sq_mat_4d A_t = transpose_mat_4d(A);
    vector_4d v;
    v.x = dotprod_4d(A_t.cols[0], b);
    v.y = dotprod_4d(A_t.cols[1], b);
    v.z = dotprod_4d(A_t.cols[2], b);
    v.w = dotprod_4d(A_t.cols[3], b);
    return v;
};

sq_mat_4d matmul_4d(sq_mat_4d A, sq_mat_4d B){
    sq_mat_4d A_t = transpose_mat_4d(A);
    sq_mat_4d C;
    for (int i =0 ; i<4;i ++){
        C.cols[i].x = dotprod_4d(A_t.cols[0], B.cols[i]);
        C.cols[i].y = dotprod_4d(A_t.cols[1], B.cols[i]);
        C.cols[i].z = dotprod_4d(A_t.cols[2], B.cols[i]);
        C.cols[i].w = dotprod_4d(A_t.cols[3], B.cols[i]);
    }
    return C;
}

sq_mat_4d get_identity_mat_4d(){
    sq_mat_4d I;
    I.cols[0] = (vector_4d){1,0,0,0};
    I.cols[1] = (vector_4d){0,1,0,0};
    I.cols[2] = (vector_4d){0,0,1,0};
    I.cols[3] = (vector_4d){0,0,0,1};
    return I;
}

sq_mat_4d get_hom_rot_mat_4d(float theta, int axis){
    sq_mat_4d R = get_identity_mat_4d();
    switch (axis){
        case 0:
            R.cols[1].y = cosf(theta);
            R.cols[1].z = sinf(theta);
            R.cols[2].y = -sinf(theta);
            R.cols[2].z = cosf(theta);
            break;
        case 1:
            R.cols[0].x = cosf(theta);
            R.cols[0].z = -sinf(theta);
            R.cols[2].x = sinf(theta);
            R.cols[2].z = cosf(theta);
            break;
        case 2:
            R.cols[0].x = cosf(theta);
            R.cols[0].y = sinf(theta);
            R.cols[1].x = - sinf(theta);
            R.cols[1].y = cosf(theta);
            break;
        default:
            break;
    }
    return R;
}

sq_mat_4d get_hom_translation_mat_4d(float d_x, float d_y, float d_z){
    sq_mat_4d M = get_identity_mat_4d();
    M.cols[3].x = d_x;
    M.cols[3].y = d_y;
    M.cols[3].z = d_z;
    return M;
}

sq_mat_4d get_hom_scaling_mat_4d(float s_x, float s_y, float s_z){
    sq_mat_4d S = get_identity_mat_4d();
    S.cols[0].x = s_x;
    S.cols[1].y = s_y;
    S.cols[2].z = s_z;
    return S;
}


void print_matrix_4d(sq_mat_4d A){
    printf("%f %f %f %f\n", A.cols[0].x, A.cols[1].x, A.cols[2].x, A.cols[3].x);
    printf("%f %f %f %f\n", A.cols[0].y, A.cols[1].y, A.cols[2].y, A.cols[3].y);
    printf("%f %f %f %f\n", A.cols[0].z, A.cols[1].z, A.cols[2].z, A.cols[3].z);
    printf("%f %f %f %f\n", A.cols[0].w, A.cols[1].w, A.cols[2].w, A.cols[3].w);
}

void print_triangle(triangle t){
    printf("vertex %f %f %f\n", t.v_0.x, t.v_0.y, t.v_0.z);
    printf("vertex %f %f %f\n", t.v_1.x, t.v_1.y, t.v_1.z);
    printf("vertex %f %f %f\n", t.v_2.x, t.v_2.y, t.v_2.z);
}

void print_mesh(mesh* m){
    for (int i = 0; i < m->n_tri; i++){
        print_triangle(m->tri[i]);
        puts("");
    }
}

void print_cam(){
    print_vector_4d(cam.look_from);
    print_vector_4d(cam.look_dir);
    print_vector_4d(cam.up_dir);
}


void window_transform(vector_4d* a){
    a->x+=1;
    a->y+=1;
    a->z+=1;
    a->x*=((float)width)*0.5;
    a->y*=((float)height)*0.5;
    a->z*=0.5;
}

vector_4d pipeline_vertex(vector_4d a){

   inplace_matvec_mul_4d(pipeline_mat, &a); 

   //clipp

   inplace_scale_hom_vector(&a, 1.0f/a.w);

   //z buffer
   window_transform(&a);

   return a;
}

float lerp(float a, float b, float f){
    return a + f * (b - a);
}

void update_z_buffer(int x, int y, float z){
    if (x < 0 || x >= width || y < 0 || y >= height) { //remove when clipping is done
        return;  
    }
    int index = y * width + x;
    int scene_index = 3 * index;
    if (z<z_buffer[index] ){
        z_buffer[index] = z;
        scene[scene_index] = (1.0f-z)*155 +100;
        scene[scene_index+1] = (1.0f-z)*155+100;
        scene[scene_index +2] = (1.0f-z)*155+100;
    }
}

void line_bresenham_low(int x_0, int y_0, float z_0, int x_1, int y_1, float z_1){
    int dx = x_1 - x_0;
    int dy = y_1 - y_0;
    float dz =1.0f/dx;
    float tz = 0;
    int yi = 1;
    if (dy < 0){
        yi = -1;
        dy = -dy;
    }
    int D = (2 * dy) - dx;
    int y = y_0;
    for (int x = x_0; x!=x_1; x++){
        update_z_buffer(x, y, lerp(z_0,z_1,tz));
        if (D > 0){
            y = y + yi;
            D = D + (2 * (dy - dx));
        }
        else{
            D = D + 2*dy;
        }
        tz+=dz;
    }
}

void line_bresenham_high(int x_0, int y_0, float z_0, int x_1, int y_1, float z_1){
    int dx = x_1 - x_0;
    int dy = y_1 - y_0;
    float dz =1.0f/dy;
    float tz = 0;
    int xi = 1;
    if (dx < 0){
        xi = -1;
        dx = -dx;
    }
    int D = (2 * dx) - dy;
    int x = x_0;
    for (int y = y_0; y!=y_1; y++){
        update_z_buffer(x, y, lerp(z_0,z_1,tz));
        if (D > 0){
            x = x + xi;
            D = D + (2 * (dx - dy));
        }
        else{
            D = D + 2*dx;
        }
        tz+=dz;
    }
}

void line_bresenham(vector_4d a, vector_4d b){
    int x_0 = roundf(a.x);
    int x_1 = roundf(b.x);
    int y_0 = roundf(a.y);
    int y_1 = roundf(b.y);
    float z_0 = a.z;
    float z_1 = b.z;
    if (abs(y_1 - y_0) < abs(x_1 - x_0)){
        if (x_0 > x_1){
            line_bresenham_low(x_1, y_1, z_1, x_0, y_0, z_0);
        }
        else{
            line_bresenham_low(x_0, y_0, z_0, x_1, y_1, z_1);
        }
    }
    else{
        if (y_0 > y_1){
            line_bresenham_high(x_1, y_1, z_1, x_0, y_0, z_0);
        }
        else {
            line_bresenham_high(x_0, y_0, z_0, x_1, y_1, z_1);
        }
    }
}

void swap_vector_4d(vector_4d* a, vector_4d* b) {
    vector_4d temp = *a;
    *a = *b;
    *b = temp;
}

void raster_triangle(vector_4d v_0, vector_4d v_1, vector_4d v_2){
    
}

void pipeline_triangle(triangle t){
    //backface culling

    vector_4d culling_vec = sub_hom_vec(t.v_0,cam.look_from);
    vector_4d normal = cross_product_hom_4d(sub_hom_vec(t.v_2,t.v_0),sub_hom_vec(t.v_1,t.v_0));
    if(dotprod_3d(culling_vec,normal)>0){
        vector_4d v_0 = pipeline_vertex(t.v_0);
        vector_4d v_1 = pipeline_vertex(t.v_1);
        vector_4d v_2 = pipeline_vertex(t.v_2);
       
       
        if(v_0.z<0||v_0.z>1||v_1.z<0||v_1.z>1||v_2.z<0||v_2.z>1){
            return;
        }
        raster_triangle(v_0,v_1,v_2);
        line_bresenham(v_0,v_1);
        line_bresenham(v_1,v_2);
        line_bresenham(v_2,v_0);

    }
}


void prep_mesh_render(mesh* m, int n_mesh){
    memset(z_buffer,FLT_MAX, width*height*sizeof(float));
    for (int i = 0; i < width*height; i++)
    {
       z_buffer[i] = 1;
    }
    
    memset(scene, 0, 3*width*height*sizeof(uint8_t));
    for (int i = 0; i < n_mesh; i++){
       for (int j = 0; j < m->n_tri; j++){
            pipeline_triangle(m[i].tri[j]);
       }
    }
}

void update_view_matrix(){
    unit_hom_vector(&cam.look_dir);
    view_mat = get_identity_mat_4d();
    vector_4d z_c = scale_hom_vector(cam.look_dir,-1.0f);
    vector_4d x_c = cross_product_hom_4d(cam.up_dir,z_c);
    unit_hom_vector(&x_c);
    cam.right_dir = x_c;
    vector_4d y_c = cross_product_hom_4d(z_c,x_c);
    view_mat.cols[0] = x_c;
    view_mat.cols[1] = y_c;
    view_mat.cols[2] = z_c;
    sq_mat_4d T = get_identity_mat_4d();
    T.cols[3] = (vector_4d) {-cam.look_from.x,-cam.look_from.y,-cam.look_from.z,1};
    view_mat = matmul_4d(view_mat,T);
}

void update_pipeline_mat(){
    update_view_matrix();
    pipeline_mat = matmul_4d(pers_proj_mat,view_mat);
}

int handle_input(){
    SDL_Event event;
    while (SDL_PollEvent(&event)){
        switch (event.type){
            case SDL_QUIT:
                return 1;
                break;
            default:
                break;
        }
    }
    int update = 0;
    if (keyboard_state_array[SDL_SCANCODE_W]&&!keyboard_state_array[SDL_SCANCODE_S]){
       vector_4d d = scale_hom_vector(cam.look_dir,cam.speed);
       inplace_add_hom_vec(&cam.look_from, &d);
       update = 1;
    }
    else if (keyboard_state_array[SDL_SCANCODE_S]&&!keyboard_state_array[SDL_SCANCODE_W]){
       vector_4d d = scale_hom_vector(cam.look_dir,-cam.speed);
       inplace_add_hom_vec(&cam.look_from, &d);
       update = 1;    
    }
    if (keyboard_state_array[SDL_SCANCODE_Q]&&!keyboard_state_array[SDL_SCANCODE_E]){
       cam.look_from.y += cam.speed;
       update = 1;
    }
    else if (keyboard_state_array[SDL_SCANCODE_E]&&!keyboard_state_array[SDL_SCANCODE_Q]){
       cam.look_from.y -= cam.speed;
       update = 1;
    }
    if (keyboard_state_array[SDL_SCANCODE_A]&&!keyboard_state_array[SDL_SCANCODE_D]){
       vector_4d d = scale_hom_vector(cam.right_dir,-cam.speed);
       inplace_add_hom_vec(&cam.look_from, &d);
       update = 1;
    }
    else if (keyboard_state_array[SDL_SCANCODE_D]&&!keyboard_state_array[SDL_SCANCODE_A]){
       vector_4d d = scale_hom_vector(cam.right_dir,cam.speed);
       inplace_add_hom_vec(&cam.look_from, &d);
       update = 1;
    }

    if (update){
        update_pipeline_mat();
    }

    return 0;
}

void render(){
    SDL_UpdateTexture(texture, NULL, scene, width * 3);
    SDL_RenderClear(app.renderer);
    SDL_RenderCopy(app.renderer, texture, NULL, NULL);
    SDL_RenderPresent(app.renderer);
}

int render_loop(mesh* m, int n_mesh){
    int quit = 0;
    int ms_start, ms_time, ms_sleep;

    int n_ticks_fps_count = 256;
    float spf_start, spf_end;

    while (quit ==0){
        ms_start = SDL_GetTicks();
        quit = handle_input();
        prep_mesh_render(m, n_mesh);
        render();
        ticks++;

        if (ticks%n_ticks_fps_count == 0){
            spf_end = SDL_GetTicks();
            printf("fps: %f\n", 1000.0f*n_ticks_fps_count/(spf_end-spf_start));
            spf_start = SDL_GetTicks();
        }

        ms_time = SDL_GetTicks() - ms_start;
        if (ms_time < 0) continue;
        ms_sleep = ms_frame - ms_time;
        if (ms_sleep > 0 ) SDL_Delay(ms_sleep); 

    }
    return 0;
}

void set_pers_proj_mat(float aspect_ratio, float vertical_field_of_view, float z_near, float z_far ){
   float fov = vertical_field_of_view * M_PI / 360.0f;
   float t = 1.0f / tanf(fov);
   memset(&pers_proj_mat, 0, sizeof(pers_proj_mat));
   pers_proj_mat.cols[0].x = t / aspect_ratio ;
   pers_proj_mat.cols[1].y = t;
   pers_proj_mat.cols[2].z =  -1.0f*(z_far+z_near)/(z_far-z_near);
   pers_proj_mat.cols[2].w = -1.0f;
   pers_proj_mat.cols[3].z = (-2.0f*z_far*z_near)/(z_far-z_near);
}



void set_camera_default(){
    cam.look_from = (vector_4d){0,0,-30,0};
    cam.up_dir = get_unit_hom_vector(0,1,0);
    cam.look_dir = (vector_4d){0,0,1,0};
    cam.speed = 0.5;
    update_pipeline_mat();
}

void init_others(){
    keyboard_state_array = SDL_GetKeyboardState(NULL);

    srand((unsigned)time(NULL));
    set_pers_proj_mat(((float) width)/((float)height), 90.0f, 5.0f, 50.0f);
    set_camera_default();
    
    texture = SDL_CreateTexture(app.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING, width, height);
    printf("camera\n");
    print_cam();
    printf("view matrix\n");
    print_matrix_4d(view_mat);
    printf("projection matrix\n");
    print_matrix_4d(pers_proj_mat);
}

void load_stl(char* path, mesh* m, int verbose){
    FILE* file = fopen(path, "r");

    if (file==NULL){
        printf("Unable to open file %s\n", path);
        return;
    }

    char line[320];
    int total_triangles = 0;
    int t_list_size = 4;
    triangle* t_list = (triangle*) malloc (t_list_size*sizeof(triangle));
    char s0[64], s1[64], s2[64], s3[64], s4[64];
    char model_name[64] = {0};
    int loop_counter = 0;
    int facet_counter = 0;
    int vertex_counter = 0;
    int line_num = 0;
    int matches;
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        matches = sscanf(line, "%s %s %s %s %s",s0, s1, s2, s3, s4);
        switch (matches){
            case 1:
                if (strcmp(s0,"endloop")==0){
                    if (vertex_counter!=3){
                        printf("erroneous file format, case (vertex_counter != 3) %d\n", matches);
                        goto close_file;
                    }
                    loop_counter--;
                    vertex_counter = 0;
                    if (verbose) puts("");
                    
                }
                else if (strcmp(s0,"endfacet")==0){
                    if (vertex_counter != 0){
                        printf("erroneous file format, case (endloop not closed) %d\n", matches);
                        goto close_file;
                    }
                    facet_counter--;
                    total_triangles++;
                    if (total_triangles == t_list_size){
                        triangle* t_list_temp = (triangle*) malloc(2*t_list_size*sizeof(triangle));
                        memcpy(t_list_temp,t_list,t_list_size*sizeof(triangle));
                        free(t_list); 
                        t_list_size*=2;
                        t_list = (triangle*) malloc (t_list_size*sizeof(triangle));
                         memcpy(t_list,t_list_temp,t_list_size*sizeof(triangle));
                        free(t_list_temp);
                    }
                }
                else {
                    printf("erroneous file format, case %d\n", matches);
                    goto close_file;
                }
                break;
            case 2:
                if (strcmp(s0,"solid")==0){
                    if (model_name[0]==0){
                        strcpy(model_name, s1);
                    }
                    else printf("model name repeated, case %d\n", matches);
                }
                else if (strcmp(s0,"endsolid")==0){
                    if (strcmp(s1,model_name)!=0){
                        printf("model name repeated, case (endsolid) %d\n", matches);
                        goto close_file;
                    }
                }
                else if ((strcmp(s0,"outer")==0)&&(strcmp(s1,"loop")==0)){
                    loop_counter++;
                }
                else { 
                    printf("erroneous file format, case %d\n", matches);
                    goto close_file;
                }
                break;
            case 4:
                if (strcmp(s0,"vertex")==0){
                    switch (vertex_counter){
                    case 0:
                        t_list[total_triangles].v_0.x = atof(s1);
                        t_list[total_triangles].v_0.y = atof(s2);
                        t_list[total_triangles].v_0.z = atof(s3);
                        t_list[total_triangles].v_0.w = 1;
                        break;
                    case 1:
                        t_list[total_triangles].v_1.x = atof(s1);
                        t_list[total_triangles].v_1.y = atof(s2);
                        t_list[total_triangles].v_1.z = atof(s3);
                        t_list[total_triangles].v_1.w = 1;
                        break;
                    case 2:
                        t_list[total_triangles].v_2.x = atof(s1);
                        t_list[total_triangles].v_2.y = atof(s2);
                        t_list[total_triangles].v_2.z = atof(s3);
                        t_list[total_triangles].v_2.w = 1;
                        break;
                    default:
                        printf("erroneous file format, case %d (vertex counter)\n", matches);
                        goto close_file; 
                    }
                    vertex_counter++;
                    if (verbose) printf("vertex %s %s %s\n", s1,s2,s3);
                }
                else{ 
                    printf("erroneous file format, case %d (vertex)\n", matches);
                    goto close_file; 
                } 
                break;
            case 5:
                if ((strcmp(s0,"facet")==0)&&(strcmp(s1,"normal")==0)){
                    facet_counter++;
                    if (verbose) printf("normal %s %s %s\n",s2,s3,s4);
                    t_list[total_triangles].normal.x = atof(s2);
                    t_list[total_triangles].normal.y = atof(s3);
                    t_list[total_triangles].normal.z = atof(s4);
                    t_list[total_triangles].normal.w = 0;

                }
                else {
                    printf("erroneous file format, case (facet normal) %d\n", matches);
                    goto close_file; 
                    } 
                break;
            default:
                printf("erroneous file format, case (default) %d\n", matches);
                return;
        }

    }
    if ((loop_counter!=0) || (facet_counter!=0)){
        printf("unable to open file %s\n", path);
    }

    m->n_tri = total_triangles;
    m->tri = (triangle*)malloc(m->n_tri*sizeof(triangle));
    memcpy(m->tri,t_list,m->n_tri*sizeof(triangle));
    free(t_list);
    printf("EOF, loaded mesh \"%s\" ", model_name);
    close_file:
        printf("reached line %d\n", line_num);
        fclose(file);
        return;
}

void load_world(mesh* m, int n){
    //load_stl("solids/cube.stl", m, 0);
    load_stl("solids/cat10k.stl", m, 0);
    //apply world matrix
}

int main (){ 

    init_SDL(scale);
    init_others();
    int n_mesh = 1;
    mesh* m = (mesh*) malloc(n_mesh*sizeof(mesh));
    load_world(m, n_mesh);
    //print_mesh(m);
    if (render_loop(m, n_mesh)<0){
        printf("Render loop crash\n");
    }

    quit_SDL();
    //free all objects here remember to check with valgrind

    return 0;
}
