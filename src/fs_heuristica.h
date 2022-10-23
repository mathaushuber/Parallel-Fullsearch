#define WIDTH 640
#define HEIGHT 360
#define N_FRAMES 120
#define BLOCK_S 8
#define SEARCH_AREA_S 32

enum frame_cfg
{
    LUMA_SIZE = WIDTH * HEIGHT,
    CHROMA_SIZE = LUMA_SIZE / 2,
    FRAME_SIZE = LUMA_SIZE + CHROMA_SIZE,
    MAX_H = HEIGHT - BLOCK_S + 1,
    MAX_W = WIDTH - BLOCK_S + 1,
    N_BLOCKS = MAX_H * MAX_W
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

struct frame_vectors
{
    int *Rv;
    int *Ra;
};

int print_frame_luma(unsigned char *luma);
int print_frame_chroma(unsigned char *chroma);
struct video *alloc_video(struct video *video);
struct frames *alloc_frames(struct frames *frames);
struct video *load_file(char *file_name);
int *get_search_area_pos(int x, int y);
int *block_matching(int *block_pos, int *search_A_pos, unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH]);
struct frame_vectors *full_search(unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH]);
int write_file_out(float time);
