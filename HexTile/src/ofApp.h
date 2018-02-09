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

    void keyPressed(int key) override;
    void keyReleased(int key) override;
    void mouseMoved(int x, int y) override;
    void mouseDragged(int x, int y, int button) override;
    void mousePressed(int x, int y, int button) override;
    void mouseScrolled(int x, int y, float scrollX, float scrollY ) override;

    void mouseReleased(int x, int y, int button) override;
    void mouseEntered(int x, int y) override;
    void mouseExited(int x, int y) override;
    void windowResized(int w, int h) override;
    void dragEvent(ofDragInfo dragInfo) override;
    void gotMessage(ofMessage msg) override;
private:
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

    ofImage concrete;

    struct TileImages
    {
        ofImage black, grey, white;
    };

    using Clock = std::chrono::steady_clock;
    using TimeStamp = Clock::time_point;
    using Duration = Clock::duration;
    using FloatSeconds = std::chrono::duration<float>;

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

        bool isVisible() const { return enabled or in_transition; }

        void update_alpha(const TimeStamp &now);
        void start_enabling(const TimeStamp &now);
        void start_disabling(const TimeStamp &now);

        void fill() const;
        void fill(TileImages &) const;
        void draw() const;
        void drawCubeIllusion();
        void removeOrientation() { orientation = Orientation::Blank; }
        void changeOrientationUp() {
            orientation = (orientation == Orientation::Blank ? Orientation::Even : (Orientation)(3 - (int)orientation));
        }
        void changeOrientationDown() {
            orientation = (orientation == Orientation::Blank ? Orientation::Odd : (Orientation)(3 - (int)orientation));
        }

        Tile(float x, float y, float radius);
        bool isPointInside(float x, float y) const;

        void changeToRandomColor(const TimeStamp &now)
        {
            if (!in_transition)
                color = (TileColor)(int)roundf(ofRandom(2));
            if (!enabled)
                start_enabling(now);
        }
        void changeToRandomOrientation()
        {
            if (!in_transition)
                orientation = (Orientation)(int)roundf(ofRandom(2));
        }
        void changeToRandomNonBlankOrientation()
        {
            if (!in_transition)
                orientation = (Orientation)(1 + (int)roundf(ofRandom(1)));
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

        float squareDistanceFromVertex(const ofVec2f &pt, int i) const {
            return ofVec2f{vertices[i].x, vertices[i].y}.squareDistance(pt);
        }
        float squareDistanceFromCenter(const ofVec2f &pt) const {
            return center.squareDistance(pt);
        }

        float radiusSquared() const { return radius * radius; }

    private:
        ofPolyline vertices;
        ofVec2f center;
        float radius;
        ofRectangle box;

    };

    ofColor getFocusColor(int gray, float alpha);
    Tile* findTile(float x, float y);

    struct Sticky
    {
        bool visible = false;
        ofVec2f pos;
        int direction = -1;
        bool flip = false;
        int step = 0;
        std::vector<ofImage> images;

        void draw();
        void adjustDirection(const Tile &tile);
    };


    void findCurrentTile(float x, float y);
    void resetFocusStartTime();
    void drawBackground();
    void drawShadows();
    void updateSticky(int x, int y);

    std::vector<Tile> tiles;
    TileImages tileImages;
    Sticky sticky;

    Tile* currentTile = nullptr;
    Tile* previousTile = nullptr;

    TimeStamp focus_start = Clock::now();
};
