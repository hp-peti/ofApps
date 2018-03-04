/*
 * ViewCoords.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_VIEWCOORDS_H_
#define SRC_VIEWCOORDS_H_

#include <ofVec2f.h>
#include <ofRectangle.h>
#include <ofGraphics.h>

struct ViewCoords
{
    float zoom = 1.0;
    ofVec2f offset { 0, 0 };

    ViewCoords() = default;
    ViewCoords(float zoom, const ofVec2f &offset) noexcept
        : zoom(zoom)
        , offset(offset)
    {
    }

    bool operator ==(const ViewCoords &other)
    {
        return zoom == other.zoom && offset == other.offset;
    }

    void setZoomWithOffset(float zoom, ofVec2f center)
    {
        offset += center / this->zoom;
        this->zoom = zoom;
        offset -= center / zoom;
    }

    void roundOffsetTo(float roundX, float roundY)
    {
        static const auto roundTo = [](float &value, float to) {
            if (to != 0) {
                value = to * std::round(value / to);
            }
        };
        roundTo(offset.x, roundX);
        roundTo(offset.y, roundY);
    }


    ofRectangle getViewRect(const ofVec2f &size) const
    {
        ofRectangle result(0, 0, size.x, size.y);
        result.scale(1 / zoom, 1 / zoom);
        result.x += offset.x;
        result.y += offset.y;
        return result;
    }

    static ViewCoords blend(const ViewCoords &prevView, const ViewCoords &nextView, float alpha)
    {
        const auto beta = 1 - alpha;;
        return ViewCoords(prevView.zoom * beta + nextView.zoom * alpha, prevView.offset * beta + nextView.offset * alpha);
    }

    void applyToCurrentMatrix() const
    {
        ofScale(zoom, zoom);
        ofTranslate(-offset.x, -offset.y);
    }

};

#endif /* SRC_VIEWCOORDS_H_ */
