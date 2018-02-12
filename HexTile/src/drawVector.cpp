/*
 * drawVector.cpp
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#include "drawVector.h"
#include "FloatConsts.h"

#include <ofGraphics.h>

#include <ofVec2f.h>
#include <ofVec3f.h>

using std::complex;

template <typename T>
inline static ofVec2f toVec2f(const complex<T> &vec)
{
    return ofVec2f(vec.real(), vec.imag());
}

template <typename T>
inline static ofVec3f toVec3f(const complex<T> &vec)
{
    return ofVec3f(vec.real(), vec.imag());
}

inline static ofVec3f toVec3f(const ofVec2f &vec)
{
    return ofVec3f(vec.x, vec.y);
}

void drawVector(const ofVec2f &pos, complex<float> direction, float length, float arrowhead)
{
    static constexpr float sin_60_deg = SQRT_3 / 2;
    static constexpr auto u150deg = complex<float>(0, 2 * M_PI / 3 + M_PI / 6);

    static const auto rotateP150 = exp(u150deg);
    static const auto rotateM150 = exp(-u150deg);

    auto lvector = (length - arrowhead * sin_60_deg) * direction;
    auto hvector = length *  direction;
    const auto start = ofVec2f(pos.x, pos.y);
    const auto end = start + toVec2f(lvector);

    const auto tript0 = start + toVec2f(hvector);
    const auto tript1 = tript0 + toVec2f(arrowhead  * (direction * rotateP150));
    const auto tript2 = tript0 + toVec2f(arrowhead  * (direction * rotateM150));

    ofDrawLine(start, end);
    ofPushStyle();
    ofFill();
    ofBeginShape();
    ofVec3f triangle[] = {toVec3f(tript0), toVec3f(tript1) , toVec3f(tript2)};
    for (auto &vertex : triangle)
        ofVertex(vertex);

    ofEndShape();
    ofPopStyle();
}



