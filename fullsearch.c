#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
    struct video *video;
    printf("\nTESTE\n");
    video = load_file("video_converted_640x360.yuv");

    print_frame_luma(video->frames->luma[0]);
    print_frame_chroma(video->frames->chroma[0]);
    
    free(video->frames);
    free(video);
    return 0;
}
