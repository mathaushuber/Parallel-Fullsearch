#define WIDTH 640
#define HEIGHT 360
#define N_FRAMES 120
#define BLOCK_H 5
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

//Keep only first frame and the motion vectors(from block matching).
struct video_compacted
{   
    unsigned char luma[LUMA_SIZE];
    unsigned char chroma[CHROMA_SIZE];
    struct pos *motion_vectors[N_FRAMES-1];
};

struct pos
{
    int x;
    int y;
};

struct frames
{
    unsigned char luma[N_FRAMES][LUMA_SIZE];
    unsigned char chroma[N_FRAMES][CHROMA_SIZE];
};

struct frames *alloc_frames();
struct frames *load_file(char *file_name);
int *get_search_area_pos(int x, int y);
int *block_matching(int *block_pos, int *area_pos, unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH]);
struct pos *full_search(unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH]);
int write_file_out(float time);
