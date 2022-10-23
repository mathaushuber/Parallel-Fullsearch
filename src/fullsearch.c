#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include "fullsearch.h"

struct frames *alloc_frames()
{
    struct frames *frames = malloc(sizeof(struct frames));
    if (!frames)
    {
        printf("\nERROR: while allocating frame\n");
        exit(1);
    }
    return frames;
}


struct frames *load_file(char *file_name)
{
    /*
        Get video frame data
        args:
            file_name - char* - string with file location/name
        return:
            video - struct video* - struct video filled with the video data
    */
    FILE *ptr_file;

    ptr_file = fopen(file_name, "rb");

    struct frames *frames = alloc_frames();

    char buffer_y[LUMA_SIZE];
    char buffer_c[CHROMA_SIZE];

    int i;
    for (i = 0; i < N_FRAMES; ++i)
    {   
        fread(buffer_y, 1, LUMA_SIZE, ptr_file);
        memcpy(frames->luma[i], buffer_y, LUMA_SIZE);
        fread(buffer_c, 1, CHROMA_SIZE, ptr_file);
        memcpy(frames->chroma[i], buffer_c, CHROMA_SIZE);
    }

    fclose(ptr_file);
    return frames;
}



int *block_matching(int *block_pos, unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH])
{
    /*
    Return the position of the best matching block inside an Search Area.
        args:
            block - unsigned char** - matrix of the block
            search_area - unsigned char** - matrix of the Search Area
        return:
            best_pos - int* - array with x,y position of best matching block
    */
    int best_SAD = 16321; // 64*255+1 always worst than any posibility
    int SAD;
    int *best_pos = malloc(2 * sizeof(int));
    int i, j, k, l;
    int block_x, block_y;

    block_x = block_pos[0];
    block_y = block_pos[1];
   
    #pragma omp for collapse(2) schedule(static, 260)
    for (i = 0; i < MAX_H; ++i)
    {
        for (j = 0; j < MAX_W; ++j)
        {
            SAD = 0;
            for (k = 0; k < BLOCK_H; ++k)
            {
		//#pragma omp parallel for reduction(+:SAD)
                for (l = 0; l < BLOCK_W; ++l)
                {
                    SAD += abs(frame_R[i + k][j + l] - frame_A[block_x + k][block_y + l]);
                }
            }
            
            #pragma omp critical 
            if (SAD < best_SAD)
            {
                best_SAD = SAD;
                best_pos[0] = i;
                best_pos[1] = j;
            }
        }
    }
    
    return best_pos;
}


struct pos *full_search(unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH])
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
    int block_pos[2];
    int *best_pos;

    struct pos *motion_vectors = malloc(N_BLOCKS * sizeof(struct pos));
    
    //#pragma omp for collapse(2) schedule(static, 180)
    for(i = 0; i < HEIGHT; i+=BLOCK_H)
    {
        for (j = 0; j < WIDTH; j+=BLOCK_W)
        {
            block_pos[0] = j;
            block_pos[1] = i;
            best_pos = block_matching(block_pos, frame_R, frame_A);
            motion_vectors[i*BLOCK_W + j].x = best_pos[0];
            motion_vectors[i*BLOCK_W + j].y = best_pos[1];
            free(best_pos);
            
        }
       printf("\nLinha: %d\n", i);
    }
    
    return motion_vectors;
}

int main()
{
    double time_spent = 0.0;
    clock_t begin = clock();
    printf("\nN_BLOCKS: %d\n", N_BLOCKS);
 
    struct frames *frames;
    frames = load_file("../data/video_converted_640x360.yuv");
    
    unsigned char(*frame_R)[WIDTH];
    unsigned char(*frame_A)[WIDTH];
    
    struct video_compacted video_compacted;
    int i;

    frame_R = (unsigned char(*)[WIDTH])frames->luma[0];
    
    memcpy(video_compacted.luma, frames->luma[0], sizeof(char) * LUMA_SIZE);
    memcpy(video_compacted.chroma, frames->chroma[0], sizeof(char) * CHROMA_SIZE);
    
    for (i = 1; i < N_FRAMES; ++i)
    {
        frame_A = (unsigned char(*)[WIDTH])frames->luma[i];
        video_compacted.motion_vectors[i - 1] = full_search(frame_R, frame_A);
        
        if (i%20 == 0)
        {
            printf("\nFRAMES PROCESSED: %d\n", i);
        }
        
        
    }
    
    printf("\nFRAMES PROCESSED: %d\n", i);
    
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;

    printf("The elapsed time is %f seconds \n", time_spent);
    
    for (i=0; i<N_BLOCKS-1; ++i)
        free(video_compacted.motion_vectors[i]);
       

    return 0;
}