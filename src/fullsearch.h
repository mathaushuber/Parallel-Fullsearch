#define WIDTH 640
#define HEIGHT 360
#define N_FRAMES 120
#define BLOCK_W 8
#define BLOCK_H 5
#define LUMA_SIZE (WIDTH * HEIGHT)
#define CHROMA_SIZE (LUMA_SIZE / 2)
#define FRAME_SIZE (LUMA_SIZE + CHROMA_SIZE)
#define MAX_H (HEIGHT - BLOCK_H + 1)
#define MAX_W (WIDTH - BLOCK_W + 1)
#define N_BLOCKS (LUMA_SIZE / (BLOCK_W*BLOCK_H))

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
int *block_matching(int *block_pos, unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH]);
struct pos *full_search(unsigned char (*frame_R)[WIDTH], unsigned char (*frame_A)[WIDTH]);