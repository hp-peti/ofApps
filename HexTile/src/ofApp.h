#pragma once

#include "ofMain.h"

#include "TileView.h"

#include "Sticky.h"

//#include <complex>
//#include <map>


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
    void findCurrentTile() { tv.findCurrentTile(ofGetMouseX(), ofGetMouseY()); }

    float getFocusAlpha(FloatSeconds period);
    ofColor getFocusColor(int gray, float alpha);
    ofColor getFocusColorMix(ofColor alpha, ofColor beta, FloatSeconds period);

    void drawBackground();
    void drawShadows();
    void updateSticky() { updateSticky(ofGetMouseX(), ofGetMouseY()); }
    void updateSticky(int x, int y);


    void drawSticky();
    void drawInfo();
    void drawFocus();
    void drawTileFocus(Tile *tile, bool shift);


    void drawToFramebuffer();

    void resizeFrameBuffer(int w, int h);

    ofFbo frameBuffer;
    bool redrawFramebuffer = false;

    ofImage concrete;
    TileImages tileImages;

    static const int default_zoom_level();
    int zoomLevel = default_zoom_level();


    TileView tv;
    Sticky sticky;


    bool showInfo = true;
    bool fullScreen = false;

    TimeStamp focus_start = Clock::now();

};
