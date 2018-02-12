/*
 * Sticky.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_STICKY_H_
#define SRC_STICKY_H_

#include "Clock.h"

#include <ofImage.h>
#include <ofVec2f.h>

#include <vector>
#include <complex>

struct Tile;

static constexpr auto STEP_DURATION = 200ms;

struct Sticky
{
    bool visible = false;
    ofVec2f pos;
    int direction = -1;
    bool flip = false;
    int step()
    {
        return stepIndex;
    }
    std::vector<ofImage> images;
    bool show_arrow = false;

    void updateStep(const TimeStamp &now);
    void adjustDirection(const Tile &tile);

    std::complex<float> getDirectionVector() const;

    void draw();
    void drawArrow(float length, const float arrowhead);
    void drawNormal(float length, const float arrowhead);

private:
    int stepIndex = 0;
    TimeStamp lastStep;
};

#endif /* SRC_STICKY_H_ */
