#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <omp.h>
#include <mpi.h>

#define WIDTH 640
#define HEIGHT 360
#define N_FRAMES 120
#define BLOCK_H 8
#define BLOCK_W 8
#define SEARCH_AREA_S 32

enum frame_cfg
{
    LUMA_SIZE = WIDTH * HEIGHT,
    CHROMA_SIZE = LUMA_SIZE / 2,
    FRAME_SIZE = LUMA_SIZE + CHROMA_SIZE,
    MAX_H = HEIGHT - BLOCK_H + 1,
    MAX_W = WIDTH - BLOCK_W + 1,
    N_BLOCKS = LUMA_SIZE / (BLOCK_H*BLOCK_W)
};

struct pos
{
    int x;
    int y;
};

//Keep only first frame and the motion vectors(from block matching).
struct video_compacted
{   
    unsigned char luma[LUMA_SIZE];
    unsigned char chroma[CHROMA_SIZE];
    struct pos motion_vectors[N_FRAMES-1][N_BLOCKS];
};


struct frames
{
    unsigned char luma[N_FRAMES][LUMA_SIZE];
    unsigned char chroma[N_FRAMES][CHROMA_SIZE];
};

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

void write_video_compacted_bin_file(struct video_compacted *vc)
{
    FILE *video_file;
    int i;
    video_file = fopen("../data/video_compacted.yuv", "wb");
    
    fwrite(vc->luma, sizeof(char), LUMA_SIZE, video_file);
    fwrite(vc->chroma, sizeof(char), CHROMA_SIZE, video_file);
    
    for (i=0; i<N_FRAMES-1; ++i)
        fwrite(&vc->motion_vectors[i], sizeof(struct pos), N_BLOCKS, video_file);
    fclose(video_file);
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    // Find out rank, size
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    int world_size;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    
    double time_spent;
    clock_t begin;

    struct frames *frames;
    unsigned char(*frame_R)[WIDTH];
    unsigned char(*frame_A)[WIDTH];
    struct video_compacted *video_compacted;
    int i;
    
    
    //---------------------------------------------------------------------------------------
    //  Aux Variables for MPI
    //---------------------------------------------------------------------------------------
    int n = N_FRAMES - 1;
    int rest = n % world_size; // rest from division of data per node
    int slice_rest = rest/world_size; // rest division to check the size of buffers
    if(rest % world_size) // if rest != 0 add 1 (like an ceil call)
        ++slice_rest;
    
    int slice_size = n/world_size; 
    int max_slice = slice_size + slice_rest; // max slice size
    
    // printf("rank %d, max slice %d, luma size %d", rank, max_slice, LUMA_SIZE);
    unsigned char *slice = malloc(sizeof(char) * max_slice * LUMA_SIZE);


    struct pos *motion_vectors_slice[max_slice];
    
    int sendcounts[world_size];
    int displs[world_size];
    int disp = 0;
    //---------------------------------------------------------------------------------------
    //---------------------------------------------------------------------------------------
    
    if (rank == 0)
    {   
        time_spent = 0.0;
        begin = clock();
        printf("\n----BLOCK MATCHING----\n");
        printf("\nNUM. FRAMES: %d\n", N_FRAMES);
        printf("\nFRAME SIZE: %dx%d\n", WIDTH, HEIGHT);
        printf("\nNUM. BLOCKS FRAME ACTUAL: %d\n", N_BLOCKS);
        printf("\nBLOCKS SIZE: %dx%d\n", BLOCK_H, BLOCK_W);
        printf("\nAREA SIZE: %dx%d\n", SEARCH_AREA_S,SEARCH_AREA_S);

        printf("\n\nSTARTING...\n");
    
        frames = load_file("../data/video_converted_640x360.yuv");
        frame_R = (unsigned char(*)[WIDTH])frames->luma[0];
        
        video_compacted = malloc(sizeof(struct video_compacted));
        memcpy(video_compacted->luma, frames->luma[0], sizeof(char) * LUMA_SIZE);
        memcpy(video_compacted->chroma, frames->chroma[0], sizeof(char) * CHROMA_SIZE);

    }
    else
    {
        frame_R = malloc(sizeof(char)*LUMA_SIZE);
    }
    //send Reference Frame (first) to each process
    MPI_Bcast(frame_R, LUMA_SIZE, MPI_CHAR, 0, MPI_COMM_WORLD);
    
    // calculate send counts and displacements
    for (i = 0; i < world_size; ++i) 
    {   
        sendcounts[i] = slice_size * LUMA_SIZE;
        if (rest) 
        {
            sendcounts[i] += LUMA_SIZE;
            --rest;
        }

        displs[i] = disp;
        disp += sendcounts[i];
    }
    
    
    MPI_Scatterv(frames->luma[1], 
                 sendcounts, 
                 displs, 
                 MPI_CHAR,
                 slice,
                 max_slice*LUMA_SIZE,
                 MPI_CHAR,
                 0,
                 MPI_COMM_WORLD);
    

    int slice_frames = sendcounts[rank] / LUMA_SIZE;
    
    #pragma omp parallel
    {   
       #pragma omp single 
        {
            for (i = 0; i < slice_frames; ++i)
            {   
                frame_A = (unsigned char(*)[WIDTH])&slice[i * LUMA_SIZE];
                motion_vectors_slice[i] = full_search(frame_R, frame_A);
            }
        }
    }

    free(slice);
    printf("\n PROCESS RANK %d - %d PROCESSED FRAMES.\n", rank, slice_frames);
        
    disp=0;
    for (i = 0; i < world_size; ++i) 
    {   
        sendcounts[i] /= LUMA_SIZE;
        sendcounts[i] *= N_BLOCKS * 2;
        displs[i] = disp;
        disp += sendcounts[i];
    }
    
    MPI_Gatherv(motion_vectors_slice[0], 
                sendcounts[rank], 
                MPI_INT, 
                video_compacted->motion_vectors, 
                sendcounts, 
                displs, 
                MPI_INT, 
                0, 
                MPI_COMM_WORLD); 

    
    for(i = 0; i < slice_frames; ++i)
        free(motion_vectors_slice[i]);
    
    
    if(rank == 0)
    {
        printf("\nALL FRAMES PROCESSED\n");
        
        clock_t end = clock();
        time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
        printf("The elapsed time is %f seconds \n", time_spent);
        write_video_compacted_bin_file(video_compacted);
        
        free(video_compacted);
    }

    MPI_Finalize(); 
    return 0;
}
