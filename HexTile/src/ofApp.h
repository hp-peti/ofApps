#pragma once

#include "ofMain.h"

#include "Clock.h"
#include "Tile.h"
#include "Sticky.h"
#include "ViewCoords.h"
#include "LinearTransition.h"

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
    void createMissingTiles(const ViewCoords &view);
    void removeExtraTiles(const ViewCoords &view);

    void startMoving(const TimeStamp &now, float xoffset, float yoffset);
    void startZooming(const TimeStamp &now, float newZoom);

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

    void drawToFramebuffer();

    void resizeFrameBuffer(int w, int h);

    bool redrawFramebuffer = false;
    ofFbo frameBuffer;

    ofImage concrete;

    static const int default_zoom_level();
    int zoomLevel = default_zoom_level();

    ViewCoords view, prevView, nextView;
    ofVec2f viewSize;

    LinearTransition viewTrans;

    std::list<Tile> tiles;
    TileImages tileImages;
    Sticky sticky;

    Tile* currentTile = nullptr;
    Tile* previousTile = nullptr;

    bool enableFlood = false;
    bool freezeSelection = false;
    std::vector<Tile *> selectedTiles;

    std::vector<Tile *> viewableTiles;

    bool showInfo = true;
    bool fullScreen = false;

    TimeStamp focus_start = Clock::now();

};
