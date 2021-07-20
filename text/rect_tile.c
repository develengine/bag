typedef struct
{
    int w, h;
} Size;


Size getSize(const Rectangle *rects, int count)
{
    Size output = { 0, 0 };
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

static inline int doesIntersect(Rectangle r1, const Rectangle *rects, int count)
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


static int compareRectangles(const void *a, const void *b)
{
    Rectangle *ar = (Rectangle*)a;
    Rectangle *br = (Rectangle*)b;

    int longerA, shorterA;
    int longerB, shorterB;

    SORT_TWO(longerA, shorterA, ar->w, ar->h);
    SORT_TWO(longerB, shorterB, br->w, br->h);

    if (longerA > longerB || (longerA == longerB && shorterA > shorterB)) {
        return -1;
    }
    if (longerA < longerB || (longerA == longerB && shorterA < shorterB)) {
        return 1;
    }
    return 0;
}


void packRectangles(Rectangle *rects, int count)
{
    int placedCount = 1;

    // sortRects(rects, count);
    qsort(rects, count, sizeof(Rectangle), &compareRectangles);
    rects->x = 0;
    rects->y = 0;

    for (Rectangle *rect = rects + 1; rect < rects + count; rect++) {
        int bestX = -1, bestY = -1;
        int smallestLongerReach = INT_MAX;
        int smallestShorterReach = INT_MAX;
        Rectangle *bestPlaced = NULL;
        int bestPlacedSide = 0;

        for (Rectangle *placed = rects; placed < rects + placedCount; placed++) {
            Rectangle testRect = { placed->x + placed->w, placed->y, rect->w, rect->h };

            if (!(testRect.flags & 0x01)) {
                int reachX = testRect.x + rect->w;
                int reachY = testRect.y + rect->h;
                int newLongerReach, newShorterReach;
                SORT_TWO(newLongerReach, newShorterReach, reachX, reachY);

                if ((newLongerReach < smallestLongerReach) ||
                    (newLongerReach == smallestLongerReach && newShorterReach < smallestShorterReach))
                {
                    if (!doesIntersect(testRect, rects, placedCount)) {
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
                SORT_TWO(newLongerReach, newShorterReach, reachX, reachY);

                if ((newLongerReach < smallestLongerReach) ||
                    (newLongerReach == smallestLongerReach && newShorterReach < smallestShorterReach))
                {
                    if (!doesIntersect(testRect, rects, placedCount)) {
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

