/*
 * Sticky.cpp
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#include "Sticky.h"
#include "Tile.h"

#include "FloatConsts.h"
#include "AppConsts.h"
#include "drawVector.h"

#include <ofGraphics.h>

using std::complex;
using std::exp;

void Sticky::drawArrow(float length, const float arrowhead)
{
    if (direction < 0)
        return;

    drawVector(pos, getDirectionVector(), length, arrowhead);
}

void Sticky::drawNormal(float length, const float arrowhead)
{
    if (direction < 0)
        return;

    static constexpr complex<float> rot90 {0, 1};
    drawVector(pos, getDirectionVector() * rot90, length, arrowhead);
}

void Sticky::adjustDirection(const Tile& tile)
{
    auto adjust_by_closest_vertex_index = [this, &tile](std::initializer_list<int> indices) {
        auto dist2min = tile.radiusSquared() * 4;
        int imin = -1;
        for (auto i : indices) {
            auto dist2 = tile.squareDistanceFromVertex(pos, i);
            if (dist2 <= dist2min) {
                dist2min = dist2;
                imin = i;
            }
        }
        if (imin < 0) {
            direction = -1;
            return;
        }
        direction = imin;
        flip = dist2min <= tile.squareDistanceFromCenter(pos);
    };

    if (tile.isVisible()) {
        if (tile.orientation == Orientation::Even) {
            adjust_by_closest_vertex_index({1,3,5});

        } else if(tile.orientation == Orientation::Odd) {
            adjust_by_closest_vertex_index({0,2,4});
        } else {
            direction = -1;
        }
    } else {
        direction = -1;
    }
}

void Sticky::updateStep(const TimeStamp& now)
{
    if ((now - lastStep) < STEP_DURATION)
        return;
    lastStep = now;
    ++stepIndex;
    if (stepIndex >= (int) images.size())
        stepIndex = 0;
}

inline static void of_rotate_degrees(float degrees)
{
#if OF_VERSION_MAJOR > 0 || OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 10
    ofRotateDeg(degrees);
#else
    ofRotate(degrees);
#endif
}

void Sticky::draw()
{
    static constexpr float sin_60_deg = SQRT_3 / 2;

    ofPushMatrix();
    ofTranslate(pos.x, pos.y);
    if (direction >= 0) {
        of_rotate_degrees(90 + 60 * direction);
        if (flip)
            ofScale(-1, -1);
        ofScale(sin_60_deg, sin_60_deg);
    }
    const auto &image = images[stepIndex];
    const float w = image.getWidth() * PIX_PER_MM;
    const float h = image.getHeight() * PIX_PER_MM;
    image.draw( -w / 2, h * .125 - h, w, h);
    ofPopMatrix();
}

complex<float> Sticky::getDirectionVector() const
{
    return direction < 0 ? complex<float>(0) :
        exp(complex<float>(0, M_PI / 2 + 2 * M_PI * direction / 6)) * (flip ? 1.f : -1.f);
}



