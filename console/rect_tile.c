typedef struct
{
    int w, h;
} Span;


Span getSpan(const Rectangle *rects, int count)
{
    Span output = { 0, 0 };
    for (int i = 0; i < count; i++) {
        Rectangle rect = rects[i];
        if (rect.x + rect.w > output.w)
            output.w = rect.x + rect.w;
        if (rect.y + rect.h > output.h)
            output.h = rect.y + rect.h;
    }
    return output;
}


#define INTERSECTS(x1, w1, x2, w2) (!((x1 + w1 <= x2) || (x2 + w2 <= x1)))

static int doesIntersect(Rectangle r1, const Rectangle *rects, int count)
{
    for (int i = 0; i < count; i++) {
        Rectangle r2 = rects[i];

        if (INTERSECTS(r1.x, r1.w, r2.x, r2.w) && INTERSECTS(r1.y, r1.h, r2.y, r2.h))
            return 1;
    }

    return 0;
}


#define SORT_TWO(longer, shorter, a, b) { \
    if (a < b) { \
        longer = b; \
        shorter = a; \
    } else { \
        longer = a; \
        shorter = b; \
    } \
}

static void sortRects(Rectangle *rects, int count)
{
    int pivotLonger, pivotShorter;
    int pivotIndex;
    int leftPointer = -1;
    int rightPointer = count;
    int longer, shorter;

    if (count <= 1)
        return;

    {
        int p1, p2, p3;
        p1 = rand() % count;
        p2 = rand() % count;
        p3 = rand() % count;
        if (p1 > p2) {
            if (p3 > p1) {
                pivotIndex = p1;
            } else if (p2 > p3) {
                pivotIndex = p2;
            } else {
                pivotIndex = p3;
            }
        } else {
            if (p3 < p1) {
                pivotIndex = p1;
            } else if (p2 < p3) {
                pivotIndex = p2;
            } else {
                pivotIndex = p3;
            }
        }
    }

    SORT_TWO(pivotLonger, pivotShorter, rects[pivotIndex].w, rects[pivotIndex].h);

    for (;;) {
        do {
            ++leftPointer;
            SORT_TWO(longer, shorter, rects[leftPointer].w, rects[leftPointer].h);
        } while (longer > pivotLonger || (longer == pivotLonger && shorter > pivotShorter));

        do {
            --rightPointer;
            SORT_TWO(longer, shorter, rects[rightPointer].w, rects[rightPointer].h);
        } while (longer < pivotLonger || (longer == pivotLonger && shorter < pivotShorter));

        if (leftPointer >= rightPointer)
            break;

        Rectangle temp = rects[leftPointer];
        rects[leftPointer] = rects[rightPointer];
        rects[rightPointer] = temp;
    }

    sortRects(rects, rightPointer + 1);
    sortRects(rects + (rightPointer + 1), count - rightPointer - 1);
}


void packRects(Rectangle *rects, int count)
{
    int placedCount = 1;

    sortRects(rects, count);
    rects->x = 0;
    rects->y = 0;

    for (Rectangle *rect = rects + 1; rect < rects + count; rect++) {
        int bestX = -1, bestY = -1;
        int smallestLongerReach = INT_MAX;
        int smallestShorterReach = INT_MAX;

        for (Rectangle *placed = rects; placed < rects + placedCount; placed++) {
            Rectangle testRect = { placed->x + placed->w, placed->y, rect->w, rect->h };
            if (!doesIntersect(testRect, rects, placedCount)) {
                int reachX = testRect.x + rect->w;
                int reachY = testRect.y + rect->h;
                int newLongerReach, newShorterReach;
                SORT_TWO(newLongerReach, newShorterReach, reachX, reachY);

                if ((newLongerReach < smallestLongerReach) ||
                    (newLongerReach == smallestLongerReach && newShorterReach < smallestShorterReach))
                {
                    bestX = testRect.x;
                    bestY = testRect.y;
                    smallestLongerReach = newLongerReach;
                    smallestShorterReach = newShorterReach;
                }
            }

            testRect.x = placed->x;
            testRect.y = placed->y + placed->h;
            if (!doesIntersect(testRect, rects, placedCount)) {
                int reachX = testRect.x + rect->w;
                int reachY = testRect.y + rect->h;
                int newLongerReach, newShorterReach;
                SORT_TWO(newLongerReach, newShorterReach, reachX, reachY);

                if ((newLongerReach < smallestLongerReach) ||
                    (newLongerReach == smallestLongerReach && newShorterReach < smallestShorterReach))
                {
                    bestX = testRect.x;
                    bestY = testRect.y;
                    smallestLongerReach = newLongerReach;
                    smallestShorterReach = newShorterReach;
                }
            }
        }

        rect->x = bestX;
        rect->y = bestY;
        ++placedCount;
    }
}



