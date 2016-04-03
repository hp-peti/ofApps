/*
 * Line.cpp
 *
 *  Created on: 26 Mar 2016
 *      Author: hpp
 */

#include "Line.h"
#include "myAlgo.h"

#include <ofGraphics.h>

#include <ofPolyline.h>

#include <vector>
#include <functional>

float crossProduct(const ofVec2f &a, const ofVec2f &b) {
    return a.x * b.y - a.y * b.x;
}

template <class Container>
void drawClosedCurve(Container & points) {
    ofBeginShape();
    for (auto& pt : points) {
        ofCurveVertex(pt);
    }
    int n { 3 };
    for (auto& pt : points) {
        if (!n--)
            break;

        ofCurveVertex(pt);
    }
    ofEndShape();
}

template <typename Container>
void drawClosedPoly(Container & points) {
    ofBeginShape();
    for (auto& pt : points) {
        ofVertex(pt);
    }
    ofEndShape();
}

struct Line::Drawer {
    explicit Drawer(Line &line) :
        line(line)

    {
        drawPoints.reserve(4 * line.points.size());
        R = line.properties->width.get() / 2;
    }

    auto& generate() {
        using namespace std;
        using namespace std::placeholders;
        drawPoints.clear();
        my::for_each_consecutive_pair(line.points, std::bind(&Drawer::add, this, _1, _2));
        lastDiff = {0, 0};
        lastDiffLength = 0;
        my::for_each_consecutive_pair(line.points.rbegin(), line.points.rend(), std::bind(&Drawer::add, this, _1, _2));

        return *this;
    }

    auto &getPoints() {
        return drawPoints;
    }

private:
    void add(const Point &a, const Point &b) {
        const auto &A = a.get();
        const auto &B = b.get();

        ofVec2f diff(B - A);

        auto length = diff.length();
        if (length < 1) {
            return;
        }
        const auto direction = diff / length;
        const ofVec2f orthogonal { direction.y, -direction.x };
        const auto offset = R * orthogonal;

        if (crossProduct(lastDiff, diff) >= 0) {
            drawPoints.emplace_back(A + offset);
        } else {
            const auto midPoint = (drawPoints.back() + A + offset) / 2;
            const auto midDiff = midPoint - A;

            const auto midDiffLength = midDiff.length();
            const auto midNorm = midDiff / midDiffLength;

            const auto projectedLength = (R / orthogonal.dot(midNorm));

            const auto newMidPoint = A + midNorm * projectedLength;
            if (projectedLength <= 0 || (projectedLength > lastDiffLength && projectedLength > length)) {
                drawPoints.back() = midPoint;
            } else {
                drawPoints.back() = newMidPoint;
            }
        }
        drawPoints.emplace_back(B + offset);
        lastDiff = diff;
        lastDiffLength = length;
    }

    Line &line;
    float R;
    float lastDiffLength = 0;
    ofVec2f lastDiff { 0, 0 };
    std::vector<ofPoint> drawPoints;
};

void Line::draw() {
    if (points.empty())
        return;
    ofSetColor(properties->color.get());

    if (points.size() == 1) {
        auto W = properties->width.get();
        auto R = W / 2;
        auto center = points[0].get();
        ofFill();
        ofSetPolyMode(OF_POLY_WINDING_ODD);
        float R2 = R * M_SQRT2;
        ofPoint points[] = { { R2, 0 }, { 0, R2 }, { -R2, 0 }, { 0, -R2 } };
        for (auto &pt : points) {
            pt += center;
        }

        drawClosedCurve(points);
        return;
    }

    if (isClosedCurve()) {
        ofFill();
        ofSetPolyMode(OF_POLY_WINDING_NONZERO);
        drawClosedCurve(points);
        return;
    }

    Drawer drawer(*this);
    drawer.generate();

    ofFill();
    ofSetPolyMode(OF_POLY_WINDING_NONZERO);
    drawClosedPoly(drawer.getPoints());

}

