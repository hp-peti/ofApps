/*
 * TileParams.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_TILEPARAMS_H_
#define SRC_TILEPARAMS_H_

#include "AppConsts.h"
#include "FloatConsts.h"


namespace TileParams {

static constexpr float sin_60_deg = SQRT_3 / 2;
static constexpr float cos_60_deg = 0.5;

static constexpr float radius = TILE_RADIUS_PIX;
static constexpr float row_height = radius *  sin_60_deg;
static constexpr float col_width = 3 * radius;
static constexpr float col_offset[2] = { radius, 2 * radius + radius * cos_60_deg};
static constexpr float row_offset = float(row_height / 2);

inline ofVec2f center(int row, int col)
{
    return ofVec2f(col_width * col + col_offset[row & 1],
                   row_height * row + row_offset);
}

struct IntRange
{
    int begin;
    int end;
};

inline
float rowf(float y)
{
    return ((y - row_offset ) / row_height);
}

inline
float colf(float x)
{
    return ((x - col_offset[0]) / col_width);
}

inline
IntRange row_range(float begin_y, float end_y)
{
    return IntRange { (int)std::floor(rowf(begin_y) - .5), (int)std::ceil(rowf(end_y) + .5) };
}

inline
IntRange col_range(float begin_x, float end_x)
{
    return IntRange { (int) std::floor(colf(begin_x) - .5), (int) std::ceil(colf(end_x) + .5) };
}

struct TileRange
{
    IntRange rows;
    IntRange cols;
};

inline
TileRange tile_range(const ofVec2f &size, float zoom = 1, const ofVec2f &offset = ofVec2f{0,0})
{
    return TileRange {
        row_range(offset.y, size.y / zoom + offset.y),
        col_range(offset.x, size.x / zoom + offset.x)
    };
}

} // namespace TileParams


#endif /* SRC_TILEPARAMS_H_ */
