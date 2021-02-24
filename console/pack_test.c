#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct
{
    int x, y, w, h;
    int flags;
    int index;
} Rectangle;

#include "rect-tile_packer.c"

void fprintRects(FILE *stream, Rectangle *rects, int count)
{
    int width = 0, height = 0;
    for (int i = 0; i < count; i++) {
        Rectangle rect = rects[i];
        if (rect.x + rect.w > width)
            width = rect.x + rect.w;
        if (rect.y + rect.h > height)
            height = rect.y + rect.h;
    }

    printf("width: %d, height: %d\n", width, height);

    char *canvas = malloc(height * (width + 1) * 2 + 1);
    memset(canvas, ' ', height * (width + 1) * 2);

    for (int r = 0; r < count; r++) {
        Rectangle rect = rects[r];
        for (int y = rect.y; y < rect.y + rect.h; y++) {
            for (int x = rect.x; x < rect.x + rect.w; x++) {
                canvas[y * (width + 1) * 2 + x * 2] = 'A' + (r % 50);
                canvas[y * (width + 1) * 2 + x * 2 + 1] = 'A' + (r % 50);
            }
        }
    }

    for (int i = 0; i < height; i++)
        canvas[(width + 1) * i * 2 + width * 2 + 1] = '\n';

    canvas[height * (width + 1) * 2] = 0;
    fprintf(stream, "%s\n", canvas);

    free(canvas);
}

#define RECT_COUNT 1213
#define MAX_SIDE   20
#define MIN_SIDE   5

int main(int argc, char *argv[])
{
    Rectangle rects[RECT_COUNT];

    for (int i = 0; i < RECT_COUNT; i++) {
        rects[i].x = 0;
        rects[i].y = 0;
        rects[i].w = MIN_SIDE + rand() % (MAX_SIDE - MIN_SIDE);
        rects[i].h = MIN_SIDE + rand() % (MAX_SIDE - MIN_SIDE);
        rects[i].flags = 0;
        rects[i].index = i;
    }

    clock_t t = clock();
    packRectsFA(rects, RECT_COUNT);
    printf("Took %lf\n", (double)(clock() - t) / CLOCKS_PER_SEC);
    
    FILE* rectFile = fopen("RECTS.txt", "w");
    fprintRects(rectFile, rects, RECT_COUNT);
    fclose(rectFile);

    return 0;
}
