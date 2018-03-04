/*
 * TileView.h
 *
 *  Created on: 4 Mar 2018
 *      Author: hpp
 */

#ifndef SRC_TILEVIEW_H_
#define SRC_TILEVIEW_H_

#include "ViewCoords.h"
#include "LinearTransition.h"
#include "Tile.h"

#include <list>
#include <vector>

#include <functional>

struct TileView
{
    void initView(const ViewCoords &view, const ofVec2f &size);
    void resizeView(const ofVec2f &size);

    void createTiles();
    void createMissingTiles(const ViewCoords &view);
    void removeExtraTiles(const ViewCoords &view);

    void startMoving(const TimeStamp &now, const Duration &duration, float xoffset, float yoffset);
    void startZooming(const TimeStamp &now, const Duration &duration, float newZoom);

    Tile* findTile(float x, float y);
    void findCurrentTile(float x, float y);

    void updateSelected();
    void selectSimilarNeighbours(Tile *from);

    ViewCoords view, prevView, nextView;
    ofVec2f viewSize;

    LinearTransition viewTrans;

    std::list<Tile> tiles;
    Tile* currentTile = nullptr;
    Tile* previousTile = nullptr;

    bool enableFlood = false;
    bool freezeSelection = false;

    std::vector<Tile *> selectedTiles;
    std::vector<Tile *> viewableTiles;

    std::function<void()> resetFocusStartTime = []{};

};



#endif /* SRC_TILEVIEW_H_ */
