/*
 * getrandom.cpp
 *
 *  Created on: 26 Mar 2016
 *      Author: hpp
 */

#include <ofMath.h>

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
    return {ofRandom(-3, 3), ofRandom(-3, 3), 0};
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

