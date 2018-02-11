#pragma once

#include "ofMain.h"

#include "Clock.h"
#include "Tile.h"
#include "Sticky.h"
#include "ViewCoords.h"

#include <complex>

#include <list>
#include <map>


class ofApp: public ofBaseApp
{

public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key) override;
    void keyReleased(int key) override;
    void mouseMoved(int x, int y) override;
    void mouseDragged(int x, int y, int button) override;
    void mousePressed(int x, int y, int button) override;
    void mouseScrolled(int x, int y, float scrollX, float scrollY) override;

    void mouseReleased(int x, int y, int button) override;
    void mouseEntered(int x, int y) override;
    void mouseExited(int x, int y) override;
    void windowResized(int w, int h) override;
    void dragEvent(ofDragInfo dragInfo) override;
    void gotMessage(ofMessage msg) override;

private:
    void createTiles();
    void createMissingTiles();
    void removeExtraTiles();



    float getFocusAlpha(FloatSeconds period);
    ofColor getFocusColor(int gray, float alpha);
    ofColor getFocusColorMix(ofColor alpha, ofColor beta, FloatSeconds period);

    Tile* findTile(float x, float y);

    void findCurrentTile() { findCurrentTile(ofGetMouseX(), ofGetMouseY()); }
    void findCurrentTile(float x, float y);
    void resetFocusStartTime();
    void drawBackground();
    void drawShadows();
    void updateSticky() { updateSticky(ofGetMouseX(), ofGetMouseY()); }
    void updateSticky(int x, int y);
    void updateSelected();
    void drawSticky();
    void drawInfo();
    void drawFocus();
    void drawTileFocus(Tile *tile);

    void selectSimilarNeighbours(Tile *from);


    ofImage concrete;

    static const int default_zoom_level();
    int zoomLevel = default_zoom_level();

    ViewCoords view, prevView;

    std::list<Tile> tiles;
    TileImages tileImages;
    Sticky sticky;

    Tile* currentTile = nullptr;
    Tile* previousTile = nullptr;

    bool enableFlood = false;
    bool freezeSelection = false;
    std::vector<Tile *> selectedTiles;

    bool showInfo = true;
    bool fullScreen = false;

    TimeStamp focus_start = Clock::now();

};
