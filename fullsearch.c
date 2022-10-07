#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h> 

#define WIDTH 640
#define HEIGHT 360
#define N_FRAMES 120

enum frame_cfg
{
    LUMA_SIZE = WIDTH * HEIGHT,
    CHROMA_SIZE = LUMA_SIZE / 2,
    FRAME_SIZE = LUMA_SIZE + CHROMA_SIZE
};

struct video
{
    struct frames *frames;
    int *disp_vectors[N_FRAMES];
};

struct frames
{
    unsigned char luma[N_FRAMES][LUMA_SIZE];
    unsigned char chroma[N_FRAMES][CHROMA_SIZE];
};

//Printando o canal luma
void print_frame_luma(unsigned char *luma)
{
    int i, j;
    printf("\nPRINTING FRAME LUMA\n");
    for (i = 0; i < HEIGHT; ++i)
    {
        for (j = 0; j < WIDTH; ++j)
        {
            printf("%u ", luma[i * WIDTH + j]);
        }
        printf("\n");
    }
    printf("\nEND OF FRAME LUMA\n");
}
//GAMBIARRAS PARA VISUALIZAÇÃO DO VÍDEO NO YUVIEW
void write_frame_chroma(unsigned char *chroma){
    int i, j;
    char url[]="chroma.yuv";
    FILE *arq;
    arq = fopen(url, "w");
    if(arq == NULL){
        printf("Erro, nao foi possivel abrir o arquivo\n");
    }
    else{
    for (i = 0; i < HEIGHT / 2; ++i)
    {
        for (j = 0; j < WIDTH; ++j)
        {
            fputc(chroma[i * WIDTH + j], arq);
        }
    }
    fclose(arq);
    }
}

void write_frame_luma(unsigned char *luma){
    int i, j;
    char url[]="luma.yuv";
    FILE *arq;
    arq = fopen(url, "w");
    if(arq == NULL){
        printf("Erro, nao foi possivel abrir o arquivo\n");
    }
    else{
    for (i = 0; i < HEIGHT / 2; ++i)
    {
        for (j = 0; j < WIDTH; ++j)
        {
            fputc(luma[i * WIDTH + j], arq);
        }
    }
    fclose(arq);
    }
}
//Printando o canal chroma 
void print_frame_chroma(unsigned char *chroma)
{   
    int i, j;
    printf("\nPRINTING FRAME CHROMA\n");
    for (i = 0; i < HEIGHT / 2; ++i)
    {
        for (j = 0; j < WIDTH; ++j)
        {
            printf("%u ", chroma[i * WIDTH + j]);
        }
        printf("\n");
    }
    printf("\nEND OF FRAME CHROMA\n");
}

//Alocando memória pro vídeo
struct video *alloc_video(struct video *video)
{
    video = malloc(sizeof(struct video));
    if (!video)
    {
        printf("\nERROR: while allocating video\n");
        exit(1);
    }
    return video;
}

//Alocando memória para os frames
struct frames *alloc_frames(struct frames *frames)
{
    frames = malloc(sizeof(struct frames));
    if (!frames)
    {
        printf("\nERROR: while allocating frame\n");
        exit(1);
    }
    return frames;
}

/*
int fs_estimation_motion(unsigned char *chroma, unsigned char *luma){
    char current_block, block_size;
    int search_area_width = 8;
    int search_area_height = 8;
    //CALCULAR O SAD -> SOMA DAS DIFERENÇAS ABSOLUTAS
    //New SAD...
    for(i = 0; i < search_area_width; i++){
        for(j = 0; i < search_area_height; j++){
            acc = 0; // acumulador dos valores de SAD. Inicialmente setado como 0
            for(k = 0; k < block_size_width; k++){
                for(l = 0; l < block_size_height; l++){
                    acc = acc + abs (search_area[i+k][j+l] - current_block[k][l]);
                }
            }
            sad_vector[search_area_width * i + j] = acc;
        }
    }
    for(n = 0; n < search_area_width*search_area_height - 1; n++){
        if(sad_vector[n] < sad_vector[n+1]){
            sad = sad_vector[n];
            mv = getArgumentPos(n);
        }
    
        else{
            sad = sad_vector[n+1];
            mv = getArgumentPos(n+1);
        }
    }
    return sad, mv;
}
*/

//Carregando o vídeo
struct video *load_file(char *file_name)
{
    struct video *video;
    FILE *ptr_file;

    ptr_file = fopen(file_name, "rb");
    size_t r;

    video = alloc_video(video);
    video->frames = alloc_frames(video->frames);

    char buffer_y[LUMA_SIZE];
    char buffer_c[CHROMA_SIZE];
    
    // Try to read first Frame
    r = fread(buffer_y, 1, LUMA_SIZE, ptr_file);
    memcpy(video->frames->luma[0], buffer_y, LUMA_SIZE);
    if (r == 0)
    {
        printf("\n ERROR LOADING FILE");
        return 0;
    }
    fread(buffer_c, 1, CHROMA_SIZE, ptr_file);
    memcpy(video->frames->chroma[0], buffer_c, CHROMA_SIZE);
   
    int i;
    for(i=1; i<N_FRAMES; ++i)
    {

        r = fread(buffer_y, 1, LUMA_SIZE, ptr_file);
        if (r == 0)
        {
            break;
        }
        fread(buffer_c, 1, CHROMA_SIZE, ptr_file);
        memcpy(video->frames->luma[i], buffer_y, LUMA_SIZE);
        memcpy(video->frames->chroma[i], buffer_c, CHROMA_SIZE);
    }

    fclose(ptr_file);
    return video;
}


int main()
{
	double time_spent = 0.0;
    clock_t begin = clock();
    struct video *video;
    
    video = load_file("video_converted_640x360.yuv");
    //printando apenas o canal de luminosidade
    print_frame_luma(video->frames->luma[0]);
 //   print_frame_chroma(video->frames->chroma[0]);
//    write_frame_chroma(video->frames->chroma[0]);
    free(video->frames);
    free(video);
    
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
 
    printf("The elapsed time is %f seconds \n", time_spent);
    return 0;
}
