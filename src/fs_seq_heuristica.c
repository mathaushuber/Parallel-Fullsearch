#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "fs_heuristica.h"

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
    for (i = 1; i < N_FRAMES; ++i)
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
    int bound_s = (SEARCH_AREA_S - BLOCK_S) / 2;

    // Vertical bounds
    area_y = y - bound_s;
    if (area_y < 0)
        area_y = 0;
    else
    {
        int lower_bound = y + BLOCK_S - 1 + bound_s;
        if (lower_bound > HEIGHT - 1)
            area_y -= lower_bound - HEIGHT;
    }

    // Horizontal bounds
    area_x = x - bound_s;
    if (area_x < 0)
        area_x = 0;
    else
    {
        int right_bound = x + BLOCK_S - 1 + bound_s;
        if (right_bound > WIDTH - 1)
            area_x -= right_bound - WIDTH;
    }

    int *search_area_pos = malloc(2 * sizeof(int));
    search_area_pos[0] = area_x;
    search_area_pos[1] = area_y;
    return search_area_pos;
}

int *block_matching(int *block_pos, int *search_A_pos, unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH])
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
    int area_x, area_y, block_x, block_y;
  //  int flag = -1;
    int abort = 0;
    area_x = search_A_pos[0];
    area_y = search_A_pos[1];
    block_x = block_pos[0];
    block_y = block_pos[1];
	for (i = area_x; i < area_x + SEARCH_AREA_S - BLOCK_S; ++i)
	if(abort < 1){
	{
		for (j = area_y; j < area_y + SEARCH_AREA_S - BLOCK_S; ++j)
		{
		SAD = 0;
		for (k = block_x; k < block_x + BLOCK_S; ++k)
		{
			for (l = block_y; l < block_y + BLOCK_S; ++l)
			{
			SAD += abs(frame_R[i + k][j + l] - frame_A[k][l]);
			}
		}

		if (SAD < best_SAD)
		{
			best_SAD = SAD;
			best_pos[0] = i;
			best_pos[1] = j;
		}
		}
		if(i > SEARCH_AREA_S)
		abort = 1;
	}
	}
    return best_pos;
}



struct frame_vectors *full_search(unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH])
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
    int block_pos[2], *search_area_pos; // x, y positions

    int *Rv = malloc(2*N_BLOCKS * sizeof(int));
    int *Ra = malloc(2*N_BLOCKS * sizeof(int));
    int *p, k;
    for (i = 0; i < MAX_H; ++i)
    {   
        k=0;
        for (j = 0; j < MAX_W; ++j)
        {
            block_pos[0] = j;
            block_pos[1] = i;
            search_area_pos = get_search_area_pos(j, i);
            p = block_matching(block_pos, search_area_pos, frame_R, frame_A);
            Rv[i * MAX_W + k] = p[0];
            Ra[i * MAX_W + k] = j;
            ++k;
            Rv[i * MAX_W + k] = p[1];
            Ra[i * MAX_W + j] = i;
            ++k;
            free(p); free(search_area_pos);
        }
    }
    
    struct frame_vectors *fv = malloc(sizeof(struct frame_vectors));
    fv->Ra = Ra;
    fv->Rv = Rv;

    return fv;
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
    fprintf(out, "%f", time);
    fclose(out);
}

int main()
{
    double time_spent = 0.0;
    clock_t begin = clock();
    struct video *video;
    unsigned char(*frame_R)[WIDTH];
    unsigned char(*frame_A)[WIDTH];
    int i;
    video = load_file("../data/video_converted_640x360.yuv");
    struct frame_vectors **video_frame_vectors;
    video_frame_vectors = malloc(N_FRAMES * sizeof(struct frame_vectors) - 1);

    for (i = 1; i < N_FRAMES; ++i)
    {

        frame_R = (unsigned char(*)[WIDTH])video->frames->luma[i - 1];
        frame_A = (unsigned char(*)[WIDTH])video->frames->luma[i];
        video_frame_vectors[i - 1] = full_search(frame_R, frame_A);
 
    //    PRINTA PRIMEIRO RA(0,0) e Rv de cada frame *FICA MAIS LENTO*
    /*        printf("\n%d %d %d %d\n", 
               video_frame_vectors[i - 1]->Ra[0],
               video_frame_vectors[i - 1]->Ra[0],
               video_frame_vectors[i - 1]->Rv[0],
               video_frame_vectors[i - 1]->Rv[0]);  
        
   */   
        if (i%20 == 0)
        {
            printf("\nFRAMES PROCESSED: %d\n", i);
        }
        
        
    }
    
    printf("\nFRAMES PROCESSED: %d\n", i);
    
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;

    printf("The elapsed time is %f seconds \n", time_spent);
    

    free(video->frames);
    free(video);
    
    for(i=0; i<N_FRAMES-1;++i)
    {
        free(video_frame_vectors[i]->Ra);
        free(video_frame_vectors[i]->Rv);
        free(video_frame_vectors[i]);
    }
    free(video_frame_vectors);

    write_file_out(time_spent);

    return 0;
}
