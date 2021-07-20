#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#define NONE 0


typedef uint64_t Span;

typedef enum {
    Left,
    Right,
    Middle,

    ChildCount,
    NoChild
} Child;


typedef struct {
    uint32_t x, y;
    uint32_t w, h;
} Info;

typedef struct {
    Info maxW;
    Info maxH;
    Info maxS;

    int next;
} Area;

typedef struct {
    int index;
    Child child;
} Place;

typedef struct {
    uint32_t x, y;
    uint32_t w, h;

    int id;
    Place parent;

    Area areas[3];
} Rect;

typedef struct {
    int fits;
    Span span;
} Score;



void printRect(const Rect *rect)
{
    Info info;
    printf("rect:\n");
    printf("  id: %d\n", rect->id);
    printf("  x: %u, y: %u, w: %u, h: %u\n", rect->x, rect->y, rect->w, rect->h);
    printf("  parent:\n");
    printf("    index: %d\n", rect->parent.index);
    printf("    child: %d\n", rect->parent.child);

    printf("  area: Left\n");
    printf("    maxW:\n");
    info = rect->areas[Left].maxW;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    maxH:\n");
    info = rect->areas[Left].maxH;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    maxS:\n");
    info = rect->areas[Left].maxS;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    next: %d\n", rect->areas[Left].next);

    printf("  area: Right\n");
    printf("    maxW:\n");
    info = rect->areas[Right].maxW;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    maxH:\n");
    info = rect->areas[Right].maxH;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    maxS:\n");
    info = rect->areas[Right].maxS;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    next: %d\n", rect->areas[Right].next);

    printf("  area: Middle\n");
    printf("    maxW:\n");
    info = rect->areas[Middle].maxW;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    maxH:\n");
    info = rect->areas[Middle].maxH;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    maxS:\n");
    info = rect->areas[Middle].maxS;
    printf("      x: %u, y: %u, w: %u, h: %u\n", info.x, info.y, info.w, info.h);
    printf("    next: %d\n", rect->areas[Middle].next);
}



static inline int max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

static inline Span getSpan(uint32_t w, uint32_t h)
{
    if (h > w) {
        uint32_t t = w;
        w = h;
        h = t;
    }

    return ((uint64_t)w << 32) | (uint64_t)h;
}

static inline int fits(uint32_t w1, uint32_t h1, uint32_t w2, uint32_t h2)
{
    return w1 <= w2 && h1 <= h2;
}

static inline Score scoreArea(Area area, uint32_t w, uint32_t h)
{
    Info i;
    Span span;
    Score score = { 0, 0xffffffffffffffff };

    i = area.maxW;
    if (fits(w, h, i.w, i.h) && (span = getSpan(i.x + w, i.y + h)) < score.span) {
        score.fits = 1;
        score.span = span;
    }

    i = area.maxH;
    if (fits(w, h, i.w, i.h) && (span = getSpan(i.x + w, i.y + h)) < score.span) {
        score.fits = 1;
        score.span = span;
    }

    i = area.maxS;
    if (fits(w, h, i.w, i.h) && (span = getSpan(i.x + w, i.y + h)) < score.span) {
        score.fits = 1;
        score.span = span;
    }

    return score;
}

static Place searchHelper(uint32_t w, uint32_t h, int searchee, Rect *rects)
{
    Child child = NoChild;
    Score bestScore = { 0, 0xffffffffffffffff };
    Area *areas = rects[searchee].areas;

    for (Child i = Left; i < ChildCount; i++) {
        Score score = scoreArea(areas[i], w, h);
        if (score.fits && score.span < bestScore.span) {
            child = i;
            bestScore = score;
        }
    }

    if (child == NoChild) {
        printf("bruh what the hell bruh!\n");
    }

    if (areas[child].next == NONE) {
        Place output = { searchee, child };
        return output;
    }

    return searchHelper(w, h, areas[child].next, rects);
}

static inline Place search(int searcher, int searchee, Rect *rects)
{
    return searchHelper(rects[searcher].w, rects[searcher].h, searchee, rects);
}

static inline Info getMaxWidth(Info a, Info b, Info c)
{
    if (b.w > a.w || (b.w == a.w && b.h > a.h)) {
        a = b;
    }
    if (c.w > a.w || (c.w == a.w && c.h > a.h)) {
        a = c;
    }
    return a;
}

static inline Info getMaxHeight(Info a, Info b, Info c)
{
    if (b.h > a.h || (b.h == a.h && b.w > a.w)) {
        a = b;
    }
    if (c.h > a.h || (c.h == a.h && c.w > a.w)) {
        a = c;
    }
    return a;
}

static inline Info getMaxSquare(Info a, Info b, Info c)
{
    if ((uint64_t)(b.w) * (uint64_t)(b.h) > (uint64_t)(a.w) * (uint64_t)(a.h)) {
        a = b;
    }
    if ((uint64_t)(c.w) * (uint64_t)(c.h) > (uint64_t)(a.w) * (uint64_t)(a.h)) {
        a = c;
    }
    return a;
}

static inline Area getMaximums(const Area *areas)
{
    Area output = {
        .maxW = getMaxWidth(areas[Left].maxW, areas[Right].maxW, areas[Middle].maxW),
        .maxH = getMaxHeight(areas[Left].maxH, areas[Right].maxH, areas[Middle].maxH),
        .maxS = getMaxSquare(areas[Left].maxS, areas[Right].maxS, areas[Middle].maxS)
    };
    return output;
}

static void updateMaximums(Area maximums, Place place, Rect *rects)
{
    Area *areas = rects[place.index].areas;
    areas[place.child].maxW = maximums.maxW;
    areas[place.child].maxH = maximums.maxH;
    areas[place.child].maxS = maximums.maxS;

    if (place.index == 0) {
        return;
    }

    updateMaximums(getMaximums(areas), rects[place.index].parent, rects);
}

static inline void insert(int rectIndex, Rect *rects, Place place)
{
    Rect *parent = rects + place.index;
    Rect *rect = rects + rectIndex;

    parent->areas[place.child].next = rectIndex;
    rect->parent = place;

    rect->x = parent->x;
    rect->y = parent->y;

    if (place.child == Left || place.child == Middle) {
        rect->x += parent->w;
    }
    if (place.child == Right || place.child == Middle) {
        rect->y += parent->h;
    }

    Info space;
    switch (place.child) {
        case Left:
            space = parent->areas[Left].maxW;
            break;
        case Right:
            space = parent->areas[Right].maxW;
            break;
        case Middle:
            space = parent->areas[Middle].maxW;
            break;
        default:
            break;
    }

    Info info;
    // Left
    info.x = rect->x + rect->w;
    info.y = rect->y;
    info.w = space.w - rect->w;
    info.h = rect->h;
    rect->areas[Left].maxW = info;
    rect->areas[Left].maxH = info;
    rect->areas[Left].maxS = info;
    rect->areas[Left].next = NONE;
    // Right
    info.x = rect->x;
    info.y = rect->y + rect->h;
    info.w = rect->w;
    info.h = space.h - rect->h;
    rect->areas[Right].maxW = info;
    rect->areas[Right].maxH = info;
    rect->areas[Right].maxS = info;
    rect->areas[Right].next = NONE;
    // Middle
    info.x = rect->x + rect->w;
    info.y = rect->y + rect->h;
    info.w = space.w - rect->w;
    info.h = space.h - rect->h;
    rect->areas[Middle].maxW = info;
    rect->areas[Middle].maxH = info;
    rect->areas[Middle].maxS = info;
    rect->areas[Middle].next = NONE;

    updateMaximums(getMaximums(rect->areas), place, rects);
}

static inline void packRect(int index, Rect *rects)
{
    // for (int i = 0; i < index; i++) {
        // printRect(rects + i);
    // }
    Place place = search(index, 0, rects);
    // printf("place: index: %d, child: %d\n", place.index, place.child);
    insert(index, rects, place);
}

static int compareRects(const void *a, const void *b)
{
    Rect *rectA = (Rect*)a;
    Rect *rectB = (Rect*)b;

    Span spanA = getSpan(rectA->w, rectA->h);
    Span spanB = getSpan(rectB->w, rectB->h);

    if (spanA > spanB) {
        return -1;
    }
    if (spanA < spanB) {
        return 1;
    }
    return 0;
}

void packRects(Rect *rects, int rectCount)
{
    qsort(rects, rectCount, sizeof(Rect), &compareRects);

    rects[0].x = 0;
    rects[0].y = 0;

    Info info;
    // Left
    info.x = rects[0].w;
    info.y = 0;
    info.w = INT_MAX;
    info.h = rects[0].h;
    rects[0].areas[Left].maxW = info;
    rects[0].areas[Left].maxH = info;
    rects[0].areas[Left].maxS = info;
    rects[0].areas[Left].next = NONE;
    // Right
    info.x = 0;
    info.y = rects[0].h;
    info.w = rects[0].w;
    info.h = INT_MAX;
    rects[0].areas[Right].maxW = info;
    rects[0].areas[Right].maxH = info;
    rects[0].areas[Right].maxS = info;
    rects[0].areas[Right].next = NONE;
    // Middle
    info.x = rects[0].w;
    info.y = rects[0].h;
    info.w = INT_MAX;
    info.h = INT_MAX;
    rects[0].areas[Middle].maxW = info;
    rects[0].areas[Middle].maxH = info;
    rects[0].areas[Middle].maxS = info;
    rects[0].areas[Middle].next = NONE;

    for (int i = 1; i < rectCount; i++) {
        // printf("%d\n", i);
        packRect(i, rects);
    }
}

#if 0
int main()
{
    const int rectCount = 1000;
    const uint32_t maxSpan = 20;
    const uint32_t minSpan = 5;

    Rect *rects = malloc(rectCount * sizeof(Rect));
    for (int i = 0; i < rectCount; i++) {
        rects[i].w = minSpan + (rand() % (maxSpan - minSpan));
        rects[i].h = minSpan + (rand() % (maxSpan - minSpan));
        rects[i].id = i;
    }

    packRects(rects, rectCount);

    for (int i = 0; i < rectCount; i++) {
        // printRect(rects + i);
        printf("%d %d %d %d\n", rects[i].x, rects[i].y, rects[i].w, rects[i].h);
    }

    free(rects);
    return 0;
}
#endif
