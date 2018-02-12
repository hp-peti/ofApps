/*
 * getrandom.cpp
 *
 *  Created on: 26 Mar 2016
 *      Author: hpp
 */

#include <ofMath.h>
#include <ofMathConstants.h>
#include <complex>

#include "getrandom.h"

float getRandomLongInterval() {
    return ofRandom(5000, 15000);
}

float getRandomInterval() {
    return ofRandom(2500, 5000);
}

ofColor getRandomColor() {
    return ofColor::fromHex((int) ofRandom(0, 0x1000000), ofRandom(128, 255));
}

ofPoint getRandomDisplacement() {
    const auto pt = ofRandom(0, 3) * std::exp(std::complex<float>(0, ofRandom(0, TWO_PI)));
    return ofPoint(pt.real(), pt.imag(), 0);
}

float getRandomDisplacementInterval() {
    return ofRandom(200, 1000);
}

float getRandomWidth() {
    return ofRandom(2, 40);
}

float getRandomWidthInterval() {
    return ofRandom(2500, 5000);
}

