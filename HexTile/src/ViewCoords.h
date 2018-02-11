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

struct ViewCoords
{
    ofVec2f size { 0, 0 };
    float zoom = 1.0;
    ofVec2f offset { 0, 0 };

    bool operator ==(const ViewCoords &other)
    {
        return size == other.size && zoom == other.zoom && offset == other.offset;
    }

    void setZoomWithOffset(float zoom, ofVec2f center)
    {
        offset += center / this->zoom;
        this->zoom = zoom;
        offset -= center / zoom;
    }

    ofRectangle getViewRect()
    {
        ofRectangle result(0, 0, size.x, size.y);
        result.scale(1 / zoom, 1 / zoom);
        result.x += offset.x;
        result.y += offset.y;
        return result;
    }
};

#endif /* SRC_VIEWCOORDS_H_ */
