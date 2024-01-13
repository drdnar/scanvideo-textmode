#ifndef COORD_2D_H
#define COORD_2D_H

/** Type of coordinate column */
typedef signed short coord_x;
/** Type of coordinate row */
typedef signed short coord_y;

/** Simple 2D coordinate */
typedef struct Coord
{
    /** Column */
    coord_x x;
    /** Row */
    coord_y y;
} coord;

/**
 * Vector addition
 * @return { a.x + b.x, a.y + b.y }
 */
static inline coord coord_add(const coord a, const coord b)
{
    coord r = { a.x + b.x, a.y + b.y };
    return r;
};

/**
 * Vector subtraction
 * @return { a.x - b.x, a.y - b.y }
 */
static inline coord coord_subtract(const coord a, const coord b)
{
    coord r = { a.x - b.x, a.y - b.y };
    return r;
};

/**
 * @return true if both x and y components are equal
 */
static inline bool coord_equals(const coord a, const coord b)
{
    return a.x == b.x && a.y == b.y;
}

/**
 * @return true if the length of a is greater than the length of b
 */
static inline bool coord_longer(const coord a, const coord b)
{
    // Note: For the purposes of inequality testing,
    // we can just factor out the square root from the Pythagorean theorem.
    return a.x*a.x + a.y*a.y > b.x*b.x + b.y*b.y;
}

/**
 * @return true if the length of a is greater than or equal to the length of b
 */
static inline bool coord_longer_equal(const coord a, const coord b)
{
    return a.x*a.x + a.y*a.y >= b.x*b.x + b.y*b.y;
}

/**
 * @return true if the length of a is less than the length of b
 */
static inline bool coord_shorter(const coord a, const coord b)
{
    return a.x*a.x + a.y*a.y < b.x*b.x + b.y*b.y;
}

/**
 * @return true if the length of a is less than or equal to the length of b
 */
static inline bool coord_shorter_equal(const coord a, const coord b)
{
    return a.x*a.x + a.y*a.y <= b.x*b.x + b.y*b.y;
}

/**
 * Bounds check function.
 * @return true if x and y in c are greater-than-or-equal-to min, and less-than-BUT-NOT-equal-to max.
 */
static inline bool coord_in_bounds(const coord c, const coord min, const coord max)
{
    return c.x >= min.x && c.x < max.x 
        && c.y >= min.y && c.y < max.y;
}

#endif /* COORD_2D_H */
