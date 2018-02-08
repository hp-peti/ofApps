#pragma once

#include "ofMain.h"

#include <vector>
#include <chrono>

class ofApp: public ofBaseApp
{

public:
    void setup();
    void update();
    void draw();

    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void mouseEntered(int x, int y);
    void mouseExited(int x, int y);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
private:
    enum class TileColor
    {
        Black,
        Gray,
        White,
    };

    ofImage concrete;

    struct Images
    {
        ofImage black, grey, white;
    };

    struct Tile
    {
        TileColor color = TileColor::White;
        bool enabled = false;

        void fill() const;
        void fill(Images &) const;
        void draw() const;
        Tile(float x, float y, float radius);
        bool isPointInside(float x, float y) const;

        void changeColorUp()
        {
            if (!enabled) {
                enabled = true;
                return;
            }
            color = (TileColor) (((int) color + 1) % 3);
        }
        void changeColorDown()
        {
            if (!enabled) {
                enabled = true;
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
    private:
        ofPolyline vertices;
        ofVec2f center;
        float radius;
        ofRectangle box;

    };

    ofColor getFocusColor(int);
    Tile* findTile(float x, float y);

    void findCurrentTile(float x, float y);
    void resetStartTime();

    std::vector<Tile> tiles;
    Images images;

    Tile* currentTile = nullptr;
    Tile* previousTile = nullptr;

    std::chrono::system_clock::time_point start_time = std::chrono::system_clock::now();
};
