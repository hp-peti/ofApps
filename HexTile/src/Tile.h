/*
 * TileImages.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */
#ifndef SRC_TILE_H_
#define SRC_TILE_H_

#include "Clock.h"

#include <ofImage.h>
#include <ofPolyline.h>

#include <cmath>
#include <vector>
#include <tuple>


struct TileImages
{
    ofImage black, grey, white;
};
enum class TileColor
{
    Black,
    Gray,
    White,
};

enum class Orientation
{
    Blank = 0,
    Odd = 1,
    Even = 2,
};

// for flood fill
using TileState = std::tuple<bool, TileColor, Orientation>;

struct Tile
{
    TileColor color = TileColor::White;
    Orientation orientation = Orientation::Blank;
    bool enabled = false;

    float alpha = 0;
    float initial_alpha = 0;
    TimeStamp alpha_start;
    TimeStamp alpha_stop;
    bool in_transition = false;

    bool isVisible() const
    {
        return enabled or in_transition;
    }

    void update_alpha(const TimeStamp &now);
    void start_enabling(const TimeStamp &now);
    void start_disabling(const TimeStamp &now);

    void fill() const;
    void fill(TileImages &) const;
    void draw() const;
    void drawCubeIllusion();
    void removeOrientation()
    {
        orientation = Orientation::Blank;
    }

    void changeOrientationUp()
    {
        orientation = (orientation == Orientation::Blank ? Orientation::Even : (Orientation) (3 - (int) orientation));
    }
    void changeOrientationDown()
    {
        orientation = (orientation == Orientation::Blank ? Orientation::Odd : (Orientation) (3 - (int) orientation));
    }

    Tile(float x, float y, float radius);
    bool isPointInside(float x, float y) const;

    void changeToRandomColor(const TimeStamp &now)
    {
        color = (TileColor) (int) roundf(ofRandom(2));
        if (!enabled)
            start_enabling(now);
    }
    void changeToRandomOrientation()
    {
        orientation = (Orientation) (int) roundf(ofRandom(2));
    }
    void changeToRandomNonBlankOrientation()
    {
        orientation = (Orientation) (1 + (int) roundf(ofRandom(1)));
    }
    void changeColorUp(const TimeStamp &now)
    {
        if (!enabled) {
            if (!in_transition) {
                color = TileColor::White;
                orientation = Orientation::Blank;
            }

            start_enabling(now);
            return;
        }
        color = (TileColor) (((int) color + 1) % 3);
    }
    void changeColorDown(const TimeStamp &now)
    {
        if (!enabled) {
            if (!in_transition) {
                color = TileColor::Black;
                orientation = Orientation::Blank;
            }
            start_enabling(now);
            return;
        }
        color = (TileColor) (((int) color + 2) % 3);
    }
    void invertColor()
    {
        if (!enabled) {
            enabled = true;
            return;
        }
        color = (TileColor) (2 - (int) color);
    }

    float squareDistanceFromVertex(const ofVec2f &pt, int i) const
    {
        return ofVec2f { vertices[i].x, vertices[i].y }.squareDistance(pt);
    }
    float squareDistanceFromCenter(const ofVec2f &pt) const
    {
        return center.squareDistance(pt);
    }

    float radiusSquared() const
    {
        return radius * radius;
    }

    TileState getStateForFloodFill()
    {
        if (!isVisible())
            return TileState(false, TileColor::White, Orientation::Blank);
        return TileState(true, color, orientation);
    }

    void connectIfNeighbour(Tile *other);
    void disconnect();

    const std::vector<Tile *>& getNeighbours()
    {
        return neighbours;
    }

    bool isInRect(const ofRectangle &rect) const
    {
        return rect.intersects(box);
    }
private:
    ofPolyline vertices;
    ofVec2f center;
    float radius;
    ofRectangle box;

    std::vector<Tile *> neighbours;

    bool isDisabling()
    {
        return in_transition && !enabled;
    }
};

#endif /* SRC_TILE_H_ */
