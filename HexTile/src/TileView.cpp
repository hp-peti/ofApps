/*
 * TileView.cpp
 *
 *  Created on: 4 Mar 2018
 *      Author: hpp
 */

#include "TileView.h"

#include "TileParams.h"

#include <ciso646>

void TileView::createTiles()
{
    auto range = TileParams::tile_range(viewSize, view.zoom, view.offset);

    for (int row = range.rows.begin; row <= range.rows.end; ++row) {
        for (int col = range.cols.begin; col <= range.cols.end; col++) {
            auto center = TileParams::center(row, col);
            tiles.emplace_back(center.x, center.y, TileParams::radius);
        }
    }

    viewableTiles.reserve(tiles.size());

    for (auto tile = tiles.begin(); tile != tiles.end(); ++tile) {
        for (auto other_tile = tiles.begin(); other_tile != tile; ++other_tile)
            tile->connectIfNeighbour(&*other_tile);
        viewableTiles.push_back(&*tile);
    }
}

void TileView::createMissingTiles(const ViewCoords &view)
{
    auto findTile = [this](const ofVec2f &center) {
        auto found = std::find_if(tiles.begin(), tiles.end(), [&center](const Tile &tile) {
            return tile.squareDistanceFromCenter(center) < tile.radiusSquared();
        });
        return found != tiles.end() ? &*found : nullptr;
    };

    auto range = TileParams::tile_range(viewSize, view.zoom, view.offset);

    for (int row = range.rows.begin; row <= range.rows.end; ++row) {
        for (int col = range.cols.begin; col <= range.cols.end; col++) {
            auto center = TileParams::center(row, col);
            auto existingTile = findTile(center);
            if (existingTile != nullptr) {
                if (std::find(viewableTiles.begin(), viewableTiles.end(), existingTile) == viewableTiles.end())
                    viewableTiles.push_back(existingTile);
                continue;
            }
            tiles.emplace_back(center.x, center.y, TileParams::radius);
            auto last = std::prev(tiles.end());
            viewableTiles.push_back(&*last);
            for (auto tile = tiles.begin(); tile != last; ++tile)
                tile->connectIfNeighbour(&*last);
        }
    }
}

void TileView::initView(const ViewCoords& vw, const ofVec2f &size)
{
    view = vw;
    prevView = vw;
    nextView = vw;
    viewSize = size;
}

void TileView::removeExtraTiles(const ViewCoords &view)
{
    auto windowRect = view.getViewRect(viewSize);

    viewableTiles.clear();

    auto tile = tiles.begin();
    while (tile != tiles.end()) {
        if (not tile->isInRect(windowRect)) {
            if (not tile->isVisible()) {
                tile->disconnect();
                if (currentTile == &*tile)
                    currentTile = nullptr;
                if (previousTile == &*tile)
                    previousTile = nullptr;
                tile = tiles.erase(tile);
                continue;
            }
        } else {
            viewableTiles.push_back(&*tile);
        }
        ++tile;
    }
    viewableTiles.shrink_to_fit();
}

void TileView::startMoving(const TimeStamp& now, const Duration &duration, float xoffset, float yoffset)
{
    nextView = view;
    prevView = view;
    nextView.offset.x += xoffset;
    nextView.offset.y += yoffset;
    nextView.roundOffsetTo(TileParams::X_STEP, TileParams::Y_STEP);

    createMissingTiles(nextView);
    viewTrans.stop().start(now, duration);
}

void TileView::startZooming(const TimeStamp &now, const Duration &duration, float newZoom)
{
    nextView = view;
    prevView = view;
    nextView.setZoomWithOffset(newZoom, ofVec2f(ofGetMouseX(), ofGetMouseY()));
    nextView.roundOffsetTo(TileParams::X_STEP, TileParams::Y_STEP);


    createMissingTiles(nextView);
    viewTrans.stop().start(now, duration);
}

Tile* TileView::findTile(float x, float y)
{
    x /= view.zoom;
    y /= view.zoom;
    x += view.offset.x;
    y += view.offset.y;

    if (currentTile != nullptr)
        if (currentTile->isPointInside(x, y))
            return currentTile;

    auto found = std::find_if(tiles.begin(), tiles.end(), [x,y](auto &tile) {
        return tile.isPointInside(x,y);
    });

    if (found == tiles.end())
        return nullptr;

    return &*found;
}

void TileView::resizeView(const ofVec2f &size)
{
    currentTile = nullptr;
    viewSize = size;
    createMissingTiles(view);
    if (not viewTrans.isActive())
        removeExtraTiles(view);

}

void TileView::findCurrentTile(float x, float y)
{
    currentTile = findTile(x, y);
    if (currentTile != nullptr and currentTile != previousTile)
    {
        if (enableFlood and freezeSelection) {
            if (std::find(selectedTiles.begin(), selectedTiles.end(), currentTile) == selectedTiles.end()) {
                freezeSelection = false;
            }
        }
        if (not enableFlood or not freezeSelection)
            resetFocusStartTime();
    }
    previousTile = currentTile;

}

void TileView::updateSelected()
{
    if (not enableFlood) {
        selectedTiles.clear();
        if (currentTile != nullptr) {
            selectedTiles.push_back(currentTile);
        }
    } else {
        if (not freezeSelection)
            selectSimilarNeighbours(currentTile);
    }
}

void TileView::selectSimilarNeighbours(Tile *from)
{
    if (from == nullptr)
        return;

    auto *found = &selectedTiles;
    found->clear();

    std::set<Tile *> visited;
    std::deque<Tile *> queue;

    const auto state = from->getStateForFloodFill();

    auto is_same = [&state](Tile *tile) {
        return tile->getStateForFloodFill() == state;
    };

    auto visit = [&visited](Tile *tile) {
        return visited.insert(tile).second == true;
    };

    auto push = [&queue, found](Tile *tile) {
        found->push_back(tile);
        queue.push_back(tile);
    };

    auto pop = [&queue]() -> Tile * {
        if (queue.empty())
            return nullptr;
        auto *p = queue.front();
        queue.pop_front();
        return p;
    };

    push(from);
    visit(from);

    while (auto *tile = pop()) {
        for (auto *next : tile->getNeighbours()) {
            if (visit(next) && is_same(next))
                push(next);
        }
    }
}


