
#define WIDTH 640
#define HEIGHT 360
#define N_FRAMES 120
#define BLOCK_S 8
#define SEARCH_AREA_S 64



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

int *block_matching(int *block_pos, int *search_A_pos, unsigned char **frame_R, unsigned char **frame_A);
int print_frame_luma(unsigned char *luma);
int print_frame_chroma(unsigned char *chroma);
struct video *alloc_video(struct video *video);
struct video *load_file(char *file_name);
int *get_search_area_pos(int x, int y, unsigned char **frame_R);
int full_search(unsigned char **frame_R, unsigned char **frame_A);
struct frames *alloc_frames(struct frames *frames);
