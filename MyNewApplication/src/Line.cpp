/*
 * Line.cpp
 *
 *  Created on: 26 Mar 2016
 *      Author: hpp
 */
#define _USE_MATH_DEFINES

#include "Line.h"
#include "myAlgo.h"

#include <ofGraphics.h>

#include <ofPolyline.h>

#include <vector>
#include <functional>

#include <ciso646>

inline
float crossProduct(const ofVec2f &a, const ofVec2f &b) {
    return a.x * b.y - a.y * b.x;
}

template <typename Container, typename PointPolyline>
void makeClosedCurvePolyLine(Container & points, PointPolyline *polyLine) {
    polyLine->clear();
    for (auto& pt : points) {
        polyLine->curveTo(pt);
    }
    int n { 3 };
    for (auto& pt : points) {
        if (!n--)
            break;
        polyLine->curveTo(pt);
    }
    polyLine->close();
}

template <typename Container>
void drawPoly(Container & points) {
    ofBeginShape();
    for (auto& pt : points) {
        ofVertex(pt);
    }
    ofEndShape();
}

using PointPolyline = ofPolyline_<ofPoint>;

template <class Container>
void drawClosedCurve(Container & points) {

    PointPolyline poly { };
    makeClosedCurvePolyLine(points, &poly);
    drawPoly(poly);
}

struct Line::ContourGenerator {
    explicit ContourGenerator(Line &line) :
        line(line) {
    }

    void resetLastDiff() {
        lastSegmentVector = {0, 0};
        lastSegmentLength = 0;
    }

    std::vector<ofPoint> generate() {
        using namespace std;
        using namespace std::placeholders;

        std::vector<ofPoint> contourPoints;
        contourPoints.reserve(4 * line.points.size());
        this->R = line.properties->width.get() / 2;

        auto addSegment = [this, &contourPoints] (const Point &a, const Point &b) {
            this->addSegmentToContour(contourPoints, a.get(), b.get());
        };

        resetLastDiff();
        my::for_each_consecutive_pair(line.points, addSegment);
        resetLastDiff();
        my::for_each_consecutive_pair(line.points.rbegin(), line.points.rend(), addSegment);

        return contourPoints;
    }

private:
    void addSegmentToContour(std::vector<ofPoint> &contourPoints, const ofPoint &A, const ofPoint &B) {

        static const auto epsilon = 1e-2f;

        ofVec2f segmentVector {B - A};

        auto segmentLength = segmentVector.length();
        if (segmentLength < 1) {
            return;
        }
        const auto direction = segmentVector / segmentLength;

        const ofVec2f orthogonal {direction.y, -direction.x};
        const auto orthoOffset = R * orthogonal;

        if (crossProduct(lastSegmentVector, segmentVector) >= 0) {
            contourPoints.emplace_back(A + orthoOffset);
        } else {
            auto &lastContourPoint = contourPoints.back();
            auto midPoint = (lastContourPoint + A + orthoOffset) / 2;

            const auto midVector = midPoint - A;
            const auto midLength = midVector.length();
            if (midLength > 0) {
                const auto midNorm = midVector / midLength;
                const auto projectedLength = (R / orthogonal.dot(midNorm));
                if (projectedLength > 0
                    and ( projectedLength <= segmentLength + epsilon
                        or projectedLength <= lastSegmentLength + epsilon)
                ) {
                    midPoint = A + midNorm * projectedLength;
                }
            }
            lastContourPoint = midPoint;
        }

        contourPoints.emplace_back(B + orthoOffset);
        lastSegmentVector = segmentVector;
        lastSegmentLength = segmentLength;
    }

    Line &line;

    float R = 0;
    float lastSegmentLength = 0;
    ofVec2f lastSegmentVector {0, 0};
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

    auto points = ContourGenerator { *this }.generate();

    ofFill();
    ofSetPolyMode(OF_POLY_WINDING_NONZERO);

    drawPoly(points);
}

bool Line::contains(const ofPoint &pt) {

    if (points.empty())
        return false;

    if (points.size() == 1) {
        return points.back().get().distance(pt) <= properties->width.get() / 2;
    }

    PointPolyline polyLine { };

    if (isClosedCurve()) {
        makeClosedCurvePolyLine(points, &polyLine);
    } else {
        polyLine.addVertices(ContourGenerator {*this}.generate());
        polyLine.close();
    }

    return polyLine.inside(pt);
}
