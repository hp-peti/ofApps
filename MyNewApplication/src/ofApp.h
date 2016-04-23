#pragma once

#include "ofMain.h"

#include <vector>

#include "Transition.h"
#include "getrandom.h"

#include "Line.h"

class ofApp: public ofBaseApp {

public:
    void setup() override;
    void update() override;
    void draw() override;

    void keyPressed(int key) override;
    void keyReleased(int key) override;
    void mouseMoved(int x, int y) override;
    void mouseDragged(int x, int y, int button) override;
    void mousePressed(int x, int y, int button) override;
    void mouseReleased(int x, int y, int button) override;
    void mouseEntered(int x, int y) override;
    void mouseExited(int x, int y) override;
    void windowResized(int w, int h) override;
    void dragEvent(ofDragInfo dragInfo) override;
    void gotMessage(ofMessage msg) override;

private:

    using LineVector = std::vector<Line>;
    using History = std::vector<LineVector>;

    Transition<ofColor> color = { getRandomColor, getRandomLongInterval };
    LineVector lines;
    History redo, undo;

    struct MovingLine {
        int button;
        ofVec2f lastPos;
        Line line;
    };

    std::unique_ptr<MovingLine> movingLine;

    ofFbo frameBuffer;

    float backgroundOpacity = 1;
    struct {
        bool shift = false;
        bool alt = false;
        bool control = false;
    } isKeyPressed;

    void drawToFrameBuffer();
    void resizeFrameBuffer(int w, int h);
    void updateBackgroundOpacity();
};

