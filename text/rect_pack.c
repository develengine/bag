#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
    int x, y;
    int w, h;
    int flags;
    int index;
} bagT_Rectangle;


typedef struct
{
    int w, h;
} bagT_Size;


bagT_Size getSize(const bagT_Rectangle *rects, int count)
{
    bagT_Size output = { 0, 0 };
    for (int i = 0; i < count; i++) {
        bagT_Rectangle rect = rects[i];
        if (rect.x + rect.w > output.w)
            output.w = rect.x + rect.w;
        if (rect.y + rect.h > output.h)
            output.h = rect.y + rect.h;
    }
    return output;
}


#define BAGT_INTERSECTS(x1, w1, x2, w2) (!((x1 + w1 <= x2) || (x2 + w2 <= x1)))

static inline int bagT_doesIntersect(bagT_Rectangle r1, const bagT_Rectangle *rects, int count)
{
    for (int i = 0; i < count; i++) {
        bagT_Rectangle r2 = rects[i];

        if (BAGT_INTERSECTS(r1.x, r1.w, r2.x, r2.w) && BAGT_INTERSECTS(r1.y, r1.h, r2.y, r2.h))
            return 1;
    }

    return 0;
}


#define BAGT_SORT_TWO(longer, shorter, a, b) { \
    if (a < b) { \
        longer = b; \
        shorter = a; \
    } else { \
        longer = a; \
        shorter = b; \
    } \
}


static int bagT_compareRectangles(const void *a, const void *b)
{
    bagT_Rectangle *ar = (bagT_Rectangle*)a;
    bagT_Rectangle *br = (bagT_Rectangle*)b;

    int longerA, shorterA;
    int longerB, shorterB;

    BAGT_SORT_TWO(longerA, shorterA, ar->w, ar->h);
    BAGT_SORT_TWO(longerB, shorterB, br->w, br->h);

    if (longerA > longerB || (longerA == longerB && shorterA > shorterB)) {
        return -1;
    }
    if (longerA < longerB || (longerA == longerB && shorterA < shorterB)) {
        return 1;
    }
    return 0;
}


void bagT_packRectangles(bagT_Rectangle *rects, int count)
{
    int placedCount = 1;

    qsort(rects, count, sizeof(bagT_Rectangle), &bagT_compareRectangles);
    rects->x = 0;
    rects->y = 0;

    for (bagT_Rectangle *rect = rects + 1; rect < rects + count; rect++) {
        int bestX = -1, bestY = -1;
        int smallestLongerReach = INT_MAX;
        int smallestShorterReach = INT_MAX;
        bagT_Rectangle *bestPlaced = NULL;
        int bestPlacedSide = 0;

        for (bagT_Rectangle *placed = rects; placed < rects + placedCount; placed++) {
            bagT_Rectangle testRect = { placed->x + placed->w, placed->y, rect->w, rect->h };

            if (!(testRect.flags & 0x01)) {
                int reachX = testRect.x + rect->w;
                int reachY = testRect.y + rect->h;
                int newLongerReach, newShorterReach;
                BAGT_SORT_TWO(newLongerReach, newShorterReach, reachX, reachY);

                if ((newLongerReach < smallestLongerReach) ||
                    (newLongerReach == smallestLongerReach && newShorterReach < smallestShorterReach))
                {
                    if (!bagT_doesIntersect(testRect, rects, placedCount)) {
                        bestX = testRect.x;
                        bestY = testRect.y;
                        smallestLongerReach = newLongerReach;
                        smallestShorterReach = newShorterReach;
                        bestPlaced = placed;
                        bestPlacedSide = 0;
                    }
                }
            }

            testRect.x = placed->x;
            testRect.y = placed->y + placed->h;

            if (!(testRect.flags & 0x02)) {
                int reachX = testRect.x + rect->w;
                int reachY = testRect.y + rect->h;
                int newLongerReach, newShorterReach;
                BAGT_SORT_TWO(newLongerReach, newShorterReach, reachX, reachY);

                if ((newLongerReach < smallestLongerReach) ||
                    (newLongerReach == smallestLongerReach && newShorterReach < smallestShorterReach))
                {
                    if (!bagT_doesIntersect(testRect, rects, placedCount)) {
                        bestX = testRect.x;
                        bestY = testRect.y;
                        smallestLongerReach = newLongerReach;
                        smallestShorterReach = newShorterReach;
                        bestPlaced = placed;
                        bestPlacedSide = 1;
                    }
                }
            }
        }

        bestPlaced->flags |= (1 << bestPlacedSide);

        rect->x = bestX;
        rect->y = bestY;
        ++placedCount;
    }
}


#define BAGT_NONE 0


typedef uint64_t bagT_Span;

typedef enum {
    bagT_Left,
    bagT_Right,
    bagT_Middle,

    bagT_ChildCount,
    bagT_NoChild
} bagT_Child;


typedef struct {
    uint32_t x, y;
    uint32_t w, h;
} bagT_Info;

typedef struct {
    bagT_Info maxW;
    bagT_Info maxH;
    bagT_Info maxS;

    int next;
} bagT_Area;

typedef struct {
    int index;
    bagT_Child child;
} bagT_Place;

typedef struct {
    uint32_t x, y;
    uint32_t w, h;

    int id;
    bagT_Place parent;

    bagT_Area areas[3];
} bagT_Rect;

typedef struct {
    int fits;
    bagT_Span span;
} bagT_Score;


#if 0
void bagT_printRect(const Rect *rect)
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
#endif


static inline int bagT_max(uint32_t a, uint32_t b)
{
    return a > b ? a : b;
}

static inline bagT_Span bagT_getSpan(uint32_t w, uint32_t h)
{
    if (h > w) {
        uint32_t t = w;
        w = h;
        h = t;
    }

    return ((uint64_t)w << 32) | (uint64_t)h;
}

static inline int bagT_fits(uint32_t w1, uint32_t h1, uint32_t w2, uint32_t h2)
{
    return w1 <= w2 && h1 <= h2;
}

static inline bagT_Score bagT_scoreArea(bagT_Area area, uint32_t w, uint32_t h)
{
    bagT_Info i;
    bagT_Span span;
    bagT_Score score = { 0, 0xffffffffffffffff };

    i = area.maxW;
    if (bagT_fits(w, h, i.w, i.h) && (span = bagT_getSpan(i.x + w, i.y + h)) < score.span) {
        score.fits = 1;
        score.span = span;
    }

    i = area.maxH;
    if (bagT_fits(w, h, i.w, i.h) && (span = bagT_getSpan(i.x + w, i.y + h)) < score.span) {
        score.fits = 1;
        score.span = span;
    }

    i = area.maxS;
    if (bagT_fits(w, h, i.w, i.h) && (span = bagT_getSpan(i.x + w, i.y + h)) < score.span) {
        score.fits = 1;
        score.span = span;
    }

    return score;
}

static bagT_Place bagT_searchHelper(uint32_t w, uint32_t h, int searchee, bagT_Rect *rects)
{
    bagT_Child child = bagT_NoChild;
    bagT_Score bestScore = { 0, 0xffffffffffffffff };
    bagT_Area *areas = rects[searchee].areas;

    for (bagT_Child i = bagT_Left; i < bagT_ChildCount; i++) {
        bagT_Score score = bagT_scoreArea(areas[i], w, h);
        if (score.fits && score.span < bestScore.span) {
            child = i;
            bestScore = score;
        }
    }

    if (child == bagT_NoChild) {
        printf("bruh what the hell bruh!\n");
    }

    if (areas[child].next == BAGT_NONE) {
        bagT_Place output = { searchee, child };
        return output;
    }

    return bagT_searchHelper(w, h, areas[child].next, rects);
}

static inline bagT_Place bagT_search(int searcher, int searchee, bagT_Rect *rects)
{
    return bagT_searchHelper(rects[searcher].w, rects[searcher].h, searchee, rects);
}

static inline bagT_Info bagT_getMaxWidth(bagT_Info a, bagT_Info b, bagT_Info c)
{
    if (b.w > a.w || (b.w == a.w && b.h > a.h)) {
        a = b;
    }
    if (c.w > a.w || (c.w == a.w && c.h > a.h)) {
        a = c;
    }
    return a;
}

static inline bagT_Info bagT_getMaxHeight(bagT_Info a, bagT_Info b, bagT_Info c)
{
    if (b.h > a.h || (b.h == a.h && b.w > a.w)) {
        a = b;
    }
    if (c.h > a.h || (c.h == a.h && c.w > a.w)) {
        a = c;
    }
    return a;
}

static inline bagT_Info bagT_getMaxSquare(bagT_Info a, bagT_Info b, bagT_Info c)
{
    if ((uint64_t)(b.w) * (uint64_t)(b.h) > (uint64_t)(a.w) * (uint64_t)(a.h)) {
        a = b;
    }
    if ((uint64_t)(c.w) * (uint64_t)(c.h) > (uint64_t)(a.w) * (uint64_t)(a.h)) {
        a = c;
    }
    return a;
}

static inline bagT_Area bagT_getMaximums(const bagT_Area *areas)
{
    bagT_Area output = {
        .maxW = bagT_getMaxWidth(areas[bagT_Left].maxW, areas[bagT_Right].maxW, areas[bagT_Middle].maxW),
        .maxH = bagT_getMaxHeight(areas[bagT_Left].maxH, areas[bagT_Right].maxH, areas[bagT_Middle].maxH),
        .maxS = bagT_getMaxSquare(areas[bagT_Left].maxS, areas[bagT_Right].maxS, areas[bagT_Middle].maxS)
    };
    return output;
}

static void bagT_updateMaximums(bagT_Area maximums, bagT_Place place, bagT_Rect *rects)
{
    bagT_Area *areas = rects[place.index].areas;
    areas[place.child].maxW = maximums.maxW;
    areas[place.child].maxH = maximums.maxH;
    areas[place.child].maxS = maximums.maxS;

    if (place.index == 0) {
        return;
    }

    bagT_updateMaximums(bagT_getMaximums(areas), rects[place.index].parent, rects);
}

static inline void bagT_insert(int rectIndex, bagT_Rect *rects, bagT_Place place)
{
    bagT_Rect *parent = rects + place.index;
    bagT_Rect *rect = rects + rectIndex;

    parent->areas[place.child].next = rectIndex;
    rect->parent = place;

    rect->x = parent->x;
    rect->y = parent->y;

    if (place.child == bagT_Left || place.child == bagT_Middle) {
        rect->x += parent->w;
    }
    if (place.child == bagT_Right || place.child == bagT_Middle) {
        rect->y += parent->h;
    }

    bagT_Info space;
    switch (place.child) {
        case bagT_Left:
            space = parent->areas[bagT_Left].maxW;
            break;
        case bagT_Right:
            space = parent->areas[bagT_Right].maxW;
            break;
        case bagT_Middle:
            space = parent->areas[bagT_Middle].maxW;
            break;
        default:
            break;
    }

    bagT_Info info;
    // Left
    info.x = rect->x + rect->w;
    info.y = rect->y;
    info.w = space.w - rect->w;
    info.h = rect->h;
    rect->areas[bagT_Left].maxW = info;
    rect->areas[bagT_Left].maxH = info;
    rect->areas[bagT_Left].maxS = info;
    rect->areas[bagT_Left].next = BAGT_NONE;
    // Right
    info.x = rect->x;
    info.y = rect->y + rect->h;
    info.w = rect->w;
    info.h = space.h - rect->h;
    rect->areas[bagT_Right].maxW = info;
    rect->areas[bagT_Right].maxH = info;
    rect->areas[bagT_Right].maxS = info;
    rect->areas[bagT_Right].next = BAGT_NONE;
    // Middle
    info.x = rect->x + rect->w;
    info.y = rect->y + rect->h;
    info.w = space.w - rect->w;
    info.h = space.h - rect->h;
    rect->areas[bagT_Middle].maxW = info;
    rect->areas[bagT_Middle].maxH = info;
    rect->areas[bagT_Middle].maxS = info;
    rect->areas[bagT_Middle].next = BAGT_NONE;

    bagT_updateMaximums(bagT_getMaximums(rect->areas), place, rects);
}

static inline void bagT_packRect(int index, bagT_Rect *rects)
{
    bagT_Place place = bagT_search(index, 0, rects);
    bagT_insert(index, rects, place);
}

static int bagT_compareRects(const void *a, const void *b)
{
    bagT_Rect *rectA = (bagT_Rect*)a;
    bagT_Rect *rectB = (bagT_Rect*)b;

    bagT_Span spanA = bagT_getSpan(rectA->w, rectA->h);
    bagT_Span spanB = bagT_getSpan(rectB->w, rectB->h);

    if (spanA > spanB) {
        return -1;
    }
    if (spanA < spanB) {
        return 1;
    }
    return 0;
}

void bagT_packRects(bagT_Rect *rects, int rectCount)
{
    qsort(rects, rectCount, sizeof(bagT_Rect), &bagT_compareRects);

    rects[0].x = 0;
    rects[0].y = 0;

    bagT_Info info;
    // Left
    info.x = rects[0].w;
    info.y = 0;
    info.w = INT_MAX;
    info.h = rects[0].h;
    rects[0].areas[bagT_Left].maxW = info;
    rects[0].areas[bagT_Left].maxH = info;
    rects[0].areas[bagT_Left].maxS = info;
    rects[0].areas[bagT_Left].next = BAGT_NONE;
    // Right
    info.x = 0;
    info.y = rects[0].h;
    info.w = rects[0].w;
    info.h = INT_MAX;
    rects[0].areas[bagT_Right].maxW = info;
    rects[0].areas[bagT_Right].maxH = info;
    rects[0].areas[bagT_Right].maxS = info;
    rects[0].areas[bagT_Right].next = BAGT_NONE;
    // Middle
    info.x = rects[0].w;
    info.y = rects[0].h;
    info.w = INT_MAX;
    info.h = INT_MAX;
    rects[0].areas[bagT_Middle].maxW = info;
    rects[0].areas[bagT_Middle].maxH = info;
    rects[0].areas[bagT_Middle].maxS = info;
    rects[0].areas[bagT_Middle].next = BAGT_NONE;

    for (int i = 1; i < rectCount; i++) {
        bagT_packRect(i, rects);
    }
}


static int bagT_fastPackRectangles(bagT_Rectangle *rectangles, int count)
{
    bagT_Rect *rects = malloc(count * sizeof(bagT_Rect));
    if (!rects) {
        fprintf(stderr, "bag text: Malloc fail!\nFile: %s, Line: %d\n", __FILE__, __LINE__);
        return 1;
    }

    for (int i = 0; i < count; i++) {
        rects[i].w = rectangles[i].w;
        rects[i].h = rectangles[i].h;
        rects[i].id = rectangles[i].index;
    }

    bagT_packRects(rects, count);

    for (int i = 0; i < count; i++) {
        rectangles[i].x = rects[i].x;
        rectangles[i].y = rects[i].y;
        rectangles[i].w = rects[i].w;
        rectangles[i].h = rects[i].h;
        rectangles[i].index = rects[i].id;
    }

    return 0;
}


