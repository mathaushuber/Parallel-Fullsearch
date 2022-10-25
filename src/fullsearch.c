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

int *get_search_area_pos(int x, int y)
{
    /*
        Get an Search Area centered around an block.
        args:
            x - int - column position of the block
            y - int - line position of the block
        return:
            best_pos - int* - array with x,y position of best matching block
    */
    int area_x, area_y;
    int bound_h = (SEARCH_AREA_S - BLOCK_H) / 2;
    int bound_w = (SEARCH_AREA_S - BLOCK_W) / 2;

    // Vertical bounds
    area_y = y - bound_h;
    if (area_y < 0)
        area_y = 0;
    else
    {
        int lower_bound = y + BLOCK_H - 1 + bound_h;
        if (lower_bound > HEIGHT - 1)
            area_y -= lower_bound - HEIGHT;
    }

    // Horizontal bounds
    area_x = x - bound_w;
    if (area_x < 0)
        area_x = 0;
    else
    {
        int right_bound = x + BLOCK_W - 1 + bound_w;
        if (right_bound > WIDTH - 1)
            area_x -= right_bound - WIDTH;
    }

    int *search_area_pos = malloc(2 * sizeof(int));
    search_area_pos[0] = area_x;
    search_area_pos[1] = area_y;
    return search_area_pos;
}

int *block_matching(int *block_pos, int *area_pos, unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH])
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
    int area_x, area_y;

    block_x = block_pos[0];
    block_y = block_pos[1];
    area_x = area_pos[0];
    area_y = area_pos[1];

 // #pragma omp parallel for collapse(2) schedule(dynamic, SEARCH_AREA_S*SEARCH_AREA_S/16)
    for (i = 0; i < SEARCH_AREA_S; ++i)
    {
        for (j = 0; j < SEARCH_AREA_S; ++j)
        {
            SAD = 0;
            for (k = 0; k < BLOCK_H; ++k)
            {
                for (l = 0; l < BLOCK_W; ++l)
                {
                    SAD += abs(frame_R[area_y + i + k][area_x + j + l] - frame_A[block_y + k][block_x + l]);
                }
            }

            #pragma omp critical 
            if (SAD < best_SAD)
            {
                best_SAD = SAD;
                best_pos[0] = j; //x
                best_pos[1] = i; //y
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
    int block_idx;
    int b_width = WIDTH/BLOCK_W;
    int *area_pos;

    struct pos *motion_vectors = malloc(N_BLOCKS * sizeof(struct pos));
    
   #pragma omp parallel for collapse(2) //schedule(dynamic, (MAX_H*MAX_W)/4 )
    for(i = 0; i < HEIGHT; i+=BLOCK_H)
    {
        for (j = 0; j < WIDTH; j+=BLOCK_W)
        {
            block_idx = (i/BLOCK_H)* b_width + (j/BLOCK_W);
            block_pos[0] = j; //x
            block_pos[1] = i; //y
            area_pos = get_search_area_pos(block_pos[0], block_pos[1]);
            best_pos = block_matching(block_pos, area_pos, frame_R, frame_A);
            motion_vectors[block_idx].x = best_pos[0];
            motion_vectors[block_idx].y = best_pos[1];
            free(best_pos);
            free(area_pos);
        }
    }
    
    return motion_vectors;
}

int write_file_out(float time){
    FILE *out;
    out = fopen("../out/log.csv", "a"); 
    if(out == NULL)
    {
        printf("Error write out file!");
        return 1;
    }
    fprintf(out, "\n");
    fprintf(out, "\t\t%f", time);
    fclose(out);
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
    
    #pragma omp parallel
    {   
       #pragma omp single 
        {
            for (i = 1; i < N_FRAMES; ++i)
            {   
                frame_A = (unsigned char(*)[WIDTH])frames->luma[i];
                video_compacted.motion_vectors[i - 1] = full_search(frame_R, frame_A);
                if (i%20 == 0)
                {
                    printf("\nFRAMES PROCESSED: %d\n", i);
                }
            }
        }
    }

    printf("\nALL FRAMES PROCESSED\n");
    
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("The elapsed time is %f seconds \n", time_spent);
    
    for (i=0; i<N_FRAMES-1; ++i)
        free(video_compacted.motion_vectors[i]);
       
    return 0;
}