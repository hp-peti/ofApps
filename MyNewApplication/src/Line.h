/*
 * Line.h
 *
 *  Created on: 26 Mar 2016
 *      Author: hpp
 */

#ifndef SRC_LINE_H_
#define SRC_LINE_H_

#include "getrandom.h"
#include "Transition.h"


#include <vector>
#include <memory>

struct Line {
    struct Properties {
        using Ptr = std::shared_ptr<Properties>;
        void update(TransitionBase::Timestamp now) {
            color.update(now);
            width.update(now);
        }
        static Ptr create() {
            return Ptr(new Properties { });
        }
        static Ptr clone(const Ptr &properties) {
            return Ptr(new Properties { *properties });
        }

    private:
        Properties() = default;
        Properties(const Properties &other) = default;

        friend struct Line;

        Transition<ofColor> color = { getRandomColor, getRandomInterval };
        Transition<float> width = { getRandomWidth, getRandomWidthInterval };
    };

    Properties::Ptr getProperties() {
        return properties;
    }

    Line(float x, float y, Properties::Ptr properties) :
        points { Point { x, y } },
        properties { properties } {
    }

    void add(float x, float y) {
        points.emplace_back(x, y);
    }

    size_t pointCount() const {
        return points.size();
    }

    float lastDistance() const {
        if (points.size() < 2)
            return 0;
        return points.end()[-1].point.distance(points.end()[-2].point);
    }

    void update(TransitionBase::Timestamp now) {
        properties->update(now);
        for (auto &pt : points)
            pt.update(now);
    }

    void removeLast() {
        points.erase(points.end() - 1);
    }

    void resize(ofVec2f proportion) {
        for (auto &pt : points) {
            pt.point.x *= proportion.x;
            pt.point.y *= proportion.y;
        }
    }

    void move(ofVec2f offset) {
        for (auto &pt : points) {
            pt.point.x += offset.x;
            pt.point.y += offset.y;
        }
    }

    void rotate(const ofVec2f &center, float degrees) {
 
        for (auto &pt : points) {
            auto p = pt.point - center;
            p.rotate(degrees, {0,0,1});
            pt.point = p + center;
        }
    }

    float width() const {
        return properties->width.get();
    }

    void draw();

    bool contains(const ofPoint &pt);

    struct Point {
        Point() = default;

        Point(float x, float y) :
            point { x, y } {
        }
        void update(TransitionBase::Timestamp now) {
            displacement->update(now);
            vertex = point + displacement->get();
        }
        ofPoint &get() {
            return vertex;
        }
        const ofPoint &get() const {
            return vertex;
        }

        operator ofPoint &() {
            return get();
        }

    private:
        friend struct Line;
        ofPoint point;
        ofPoint vertex;
        std::shared_ptr<Transition<ofPoint>> displacement = std::make_shared<Transition<ofPoint>>(getRandomDisplacement,
                                                                                                  getRandomDisplacementInterval);
    };

    auto empty() const {
        return points.empty();
    }
    auto size() const {
        return points.size();
    }

    void cloneNewProperties() {
        properties = Properties::clone(properties);
    }

private:
    struct ContourGenerator;
    friend struct ContourGenerator;

    std::vector<Point> points;
    Properties::Ptr properties;

    bool isClosedCurve() const {
        return points.front().get().distance(points.back().get()) <= properties->width.get();
    }
};

#endif /* SRC_LINE_H_ */
