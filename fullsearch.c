#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define WIDTH 640
#define HEIGHT 360
#define N_FRAMES 120
#define BLOCK_S 8
#define SEARCH_AREA_H 64
#define SEARCH_AREA_W 36

enum frame_cfg
{
    LUMA_SIZE = WIDTH * HEIGHT,
    CHROMA_SIZE = LUMA_SIZE / 2,
    FRAME_SIZE = LUMA_SIZE + CHROMA_SIZE,
    N_BLOCKS = LUMA_SIZE / (BLOCK_S * BLOCK_S)

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

int print_frame_luma(unsigned char *luma)
{
    int i, j;
    printf("\nPRINTING FRAME LUMA\n");
    for (i = 0; i < HEIGHT; ++i)
    {
        for (j = 0; j < WIDTH; ++j)
        {
            printf("%x ", luma[i * WIDTH + j]);
        }
        printf("\n");
    }
    printf("\nEND OF FRAME LUMA\n");
    return 0;
}

int print_frame_chroma(unsigned char *chroma)
{   
    int i, j;
    printf("\nPRINTING FRAME CHROMA\n");
    for (i = 0; i < HEIGHT / 2; ++i)
    {
        for (j = 0; j < WIDTH; ++j)
        {
            printf("%x ", chroma[i * WIDTH + j]);
        }
        printf("\n");
    }
    printf("\nEND OF FRAME CHROMA\n");
    return 0;
}

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

struct video *load_file(char *file_name)
{
/* 
    Get video frame data
    
    args:
        file_name - char* - string with file location/name
    return: 
        video - struct video* - struct video filled with the video data
    
*/
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

unsigned char *get_search_area(int x, int y, unsigned char **frame_R)
{
/* 
    Get an Search Area centered around an block.
    
    args:
        frame_R - unsigned char** - matrix of reference frame
        x - int - column position of the block
        y - int - line position of the block
    return: 
        best_pos - int* - array with x,y position of best matching block
    
*/
    
    unsigned char *search_area = malloc(SEARCH_AREA_H*SEARCH_AREA_W*sizeof(char));
    //---
    return search_area;
}

int full_search(unsigned char **frame_R, unsigned char **frame_A)
{   
/* 
    Return Rv and Ra arrays of each block.
    
    args:
        frame_R - unsigned char** - matrix of reference frame
        frame_A - unsigned char** - matrix of current frame
    return: 
        best_pos - int* - array with x,y position of best matching block
    
*/
    int i, j;
    unsigned char **block;
    unsigned char **search_area;
    int max_h = HEIGHT - BLOCK_S + 1;
    int max_w = WIDTH - BLOCK_S + 1;
    int n_blocks = max_h * max_w;
    int Rv[n_blocks][2], Ra[n_blocks][2];
    
    Rv = pos;
   
    for(i = 0; i < max_h; ++i)
    {
        for(j = 0; j < max_w; ++j)
        {
            block = frame_A[i][j];
            search_area = get_search_area(j, i, frame_R);
            Rv[i*max_w + j] = block_matching(block, search_area);
            Ra[i*max_w + j][0] = j;
            Ra[i*max_w + j][1] = i;
        }
    }

    return Rv, Ra;
}

int *block_matching(unsigned char **block, unsigned char **search_area)
{  
/* 
Return the position of the best matching block inside an Search Area.
    
    args:
        block - unsigned char** - matrix of the block
        search_area - unsigned char** - matrix of the Search Area
    return: 
        best_pos - int* - array with x,y position of best matching block
    
*/
    int best_SAD=16321; //64*255+1 always worst than any posibility
    int SAD;
    int *best_pos = malloc(2*sizeof(int));
    int i,j, k, l;

    for(i=0; i<SEARCH_AREA_H - BLOCK_S+1; ++i)
    {
        for(j=0; j<SEARCH_AREA_W - BLOCK_S+1; ++j)
        {
            SAD = 0;
            for(k=0; k<BLOCK_S; ++k)
            {
                for(l=0; l<BLOCK_S; ++l)
                    SAD += abs(search_area[i+k][j+l] - block[k][l]);
            }
                
            if(SAD < best_SAD)
            {
                best_SAD = SAD;
                best_pos[0] = i;
                best_pos[1] = j;
            }
        }
    }
    
    return best_pos;
}



int main()
{
    double time_spent = 0.0;
    clock_t begin = clock();	
    struct video *video;
    video = load_file("video_converted_640x360.yuv");
    free(video->frames);
    free(video);
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
 
    printf("The elapsed time is %f seconds \n", time_spent);
    return 0;
}
