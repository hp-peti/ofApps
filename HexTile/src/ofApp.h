#pragma once

#include "ofMain.h"

#include <chrono>
#include <complex>

#include <list>
#include <map>

#include <tuple>

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

        bool isVisible() const { return enabled or in_transition; }

        void update_alpha(const TimeStamp &now);
        void start_enabling(const TimeStamp &now);
        void start_disabling(const TimeStamp &now);

        void fill() const;
        void fill(TileImages &) const;
        void draw() const;
        void drawCubeIllusion();
        void removeOrientation() { orientation = Orientation::Blank; }

        void changeOrientationUp()
        {
            orientation = (orientation == Orientation::Blank ? Orientation::Even : (Orientation)(3 - (int)orientation));
        }
        void changeOrientationDown()
        {
            orientation = (orientation == Orientation::Blank ? Orientation::Odd : (Orientation)(3 - (int)orientation));
        }

        Tile(float x, float y, float radius);
        bool isPointInside(float x, float y) const;

        void changeToRandomColor(const TimeStamp &now)
        {
            color = (TileColor)(int)roundf(ofRandom(2));
            if (!enabled)
                start_enabling(now);
        }
        void changeToRandomOrientation()
        {
            orientation = (Orientation)(int)roundf(ofRandom(2));
        }
        void changeToRandomNonBlankOrientation()
        {
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
            color = (TileColor)(((int)color + 1) % 3);
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
            color = (TileColor)(((int)color + 2) % 3);
        }
        void invertColor()
        {
            if (!enabled) {
                enabled = true;
                return;
            }
            color = (TileColor)(2 - (int)color);
        }

        float squareDistanceFromVertex(const ofVec2f &pt, int i) const
        {
            return ofVec2f { vertices[i].x, vertices[i].y }.squareDistance(pt);
        }
        float squareDistanceFromCenter(const ofVec2f &pt) const
        {
            return center.squareDistance(pt);
        }

        float radiusSquared() const { return radius * radius; }

        TileState getStateForFloodFill()
        {
            if (!isVisible())
                return TileState(false, TileColor::White, Orientation::Blank);
            return TileState(true, color, orientation);
        }

        void connectIfNeighbour(Tile *other);
        void disconnect();

        const std::vector<Tile *>& getNeighbours() { return neighbours; }

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

    float getFocusAlpha(FloatSeconds period);
    ofColor getFocusColor(int gray, float alpha);
    ofColor getFocusColorMix(ofColor alpha, ofColor beta, FloatSeconds period);

    Tile* findTile(float x, float y);

    struct Sticky
    {
        bool visible = false;
        ofVec2f pos;
        int direction = -1;
        bool flip = false;
        int step() { return stepIndex; }
        std::vector<ofImage> images;
        bool show_arrow = false;

        void updateStep(const TimeStamp &now);
        void adjustDirection(const Tile &tile);

        std::complex<float> getDirectionVector() const;

        void draw();
        void drawArrow(float length, const float arrowhead);
        void drawNormal(float length, const float arrowhead);

    private:
        int stepIndex = 0;
        TimeStamp lastStep;
    };

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

    struct ViewCoords
    {
        ofVec2f size { 0, 0 };
        float zoom = 1.0;
        ofVec2f offset { 0, 0 };

        bool operator == (const ViewCoords &other)
        {
            return size == other.size
                && zoom == other.zoom
                && offset == other.offset;
        }

        void setZoomWithOffset(float zoom, ofVec2f center);

        ofRectangle getViewRect()
        {
            ofRectangle result(0, 0, size.x, size.y);
            result.scale(1 / zoom, 1 / zoom);
            result.x += offset.x;
            result.y += offset.y;
            return result;
        }
    };

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
