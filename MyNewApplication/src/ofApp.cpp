#include "ofApp.h"
#include "ofGraphics.h"

#include <chrono>
#include <cassert>
#include <ciso646>

void ofApp::setup() {
    ofSetBackgroundAuto(false);
    resizeFrameBuffer(ofGetWidth(), ofGetHeight());
}

void ofApp::update() {
    auto now = TransitionBase::clockNow();
    color.update(now);
    for (auto &line : lines)
        line.update(now);
    if (movingLine)
        movingLine->line.update(now);
}

void ofApp::updateBackgroundOpacity() {
    static const auto baseBgOpacity = .1;
    static const auto opacityDecayPerFrame = .5;
    backgroundOpacity = baseBgOpacity + (backgroundOpacity - baseBgOpacity) * (1.0 - opacityDecayPerFrame);
}

void ofApp::drawToFrameBuffer() {
    ofPushStyle();
    frameBuffer.begin(true);
    ofEnableAlphaBlending();
    ofEnableAntiAliasing();
    ofSetColor(color.get(), backgroundOpacity * 0xFF);
    updateBackgroundOpacity();
    ofFill();
    ofDrawRectangle(0, 0, ofGetWidth(), ofGetHeight());
    ofEnableSmoothing();
    for (auto& line : lines)
        line.draw();
    if (movingLine) {
        movingLine->line.draw();
    }
    frameBuffer.end();
    ofPopStyle();
}

void ofApp::draw() {
    drawToFrameBuffer();

    ofDisableAlphaBlending();
    ofDisableDepthTest();
    frameBuffer.draw(0, 0, ofGetWidth(), ofGetHeight());
}

void ofApp::clearLines() {
    if (lines.size()) {
        undo.push_back(std::move(lines));
        redo.clear();
        lines.clear();
        backgroundOpacity = 1;
    }
}

void ofApp::undoLines() {
    if (undo.size()) {
        redo.push_back(std::move(lines));
        lines = std::move(undo.back());
        undo.pop_back();
        backgroundOpacity = .5;
    }
}

void ofApp::keyPressed(int key) {

    switch (key) {
    case 'f':
    case 'F':
        ofToggleFullscreen();
        break;
    case 'q':
    case 'Q':
        ofExit(0);
        break;
    case OF_KEY_DEL:
    case 'c':
        clearLines();
        break;
    case 'z':
    case 'Z':
        if (isKeyPressed.control) {
            if (isKeyPressed.shift) {
                redoLines();
                break;
            }
            undoLines();
            break;
        }
        break;
    case OF_KEY_BACKSPACE:
    case OF_KEY_LEFT:
    case 'u':
    case 'U':
        undoLines();
        break;
    case OF_KEY_RIGHT:
    case 'r':
    case 'R':
        redoLines();
        break;
    case OF_KEY_SHIFT:
        isKeyPressed.shift = true;
        break;
    case OF_KEY_ALT:
        isKeyPressed.alt = true;
        break;
    case OF_KEY_CONTROL:
        isKeyPressed.control = true;
        break;
    }
}

void ofApp::keyReleased(int key) {
    switch (key) {
    case OF_KEY_SHIFT:
        isKeyPressed.shift = false;
        break;
    case OF_KEY_ALT:
        isKeyPressed.alt = false;
        break;
    case OF_KEY_CONTROL:
        isKeyPressed.control = false;
        break;
    }
}

void ofApp::mouseMoved(int x, int y) {

}

void ofApp::mouseDragged(int x, int y, int button) {
    if (movingLine != nullptr and button == movingLine->button) {
        ofVec2f curPos { (float) x, (float) y };
        movingLine->line.move(curPos - movingLine->lastPos);
        movingLine->lastPos = curPos;
        backgroundOpacity = .5;
        return;
    }
    if (button == OF_MOUSE_BUTTON_LEFT) {
        if (lines.empty())
            return mousePressed(x, y, button);

        auto &line = lines.back();
        line.add(x, y);

        if (line.lastDistance() < 8)
            line.removeLast();
    }
    return;
}

void ofApp::saveUndo() {
    undo.push_back(lines);
    redo.clear();
}

void ofApp::mousePressed(int x, int y, int button) {

    if (button == OF_MOUSE_BUTTON_LEFT) {
        Line::Properties::Ptr properties { };
        if (not lines.empty() and isKeyPressed.shift) {
            if (isKeyPressed.control) {
                saveUndo();
                return mouseDragged(x, y, button);
            }
            properties = lines.back().getProperties();
        } else {
            properties = Line::Properties::create();
        }
        saveUndo();
        lines.emplace_back((float) x, (float) y, properties);
    } else if (button == OF_MOUSE_BUTTON_RIGHT or button == OF_MOUSE_BUTTON_MIDDLE) {
        auto found = std::find_if(lines.rbegin(), lines.rend(), [x,y] (Line &line) {
            return line.contains( {(float)x, (float)y});
        });
        if (found != lines.rend()) {
            saveUndo();
            bool isCopying { button == OF_MOUSE_BUTTON_RIGHT and isKeyPressed.shift };
            bool cloneNewProperties { isCopying and not isKeyPressed.control };
            bool isDeleting { button == OF_MOUSE_BUTTON_MIDDLE or (button == OF_MOUSE_BUTTON_RIGHT and isKeyPressed.control and not isKeyPressed.shift) };
            if (isDeleting) {
                backgroundOpacity = .5;
            } else {
                movingLine.reset(new MovingLine { button, { (float) x, (float) y }, *found });
                if (cloneNewProperties) {
                    movingLine->line.cloneNewProperties();
                }
            }
            if (not isCopying)
                lines.erase(found.base() - 1);
        }
    }
}

void ofApp::mouseReleased(int x, int y, int button) {
    if (movingLine) {
        lines.push_back(std::move(movingLine->line));
        movingLine.reset();
    }
}

void ofApp::mouseEntered(int x, int y) {

}

void ofApp::mouseExited(int x, int y) {

}

void ofApp::resizeFrameBuffer(int w, int h) {
    frameBuffer.clear();
    frameBuffer.allocate(w, h, GL_RGBA);
    backgroundOpacity = 1;
}

void ofApp::windowResized(int w, int h) {

    ofClear(color.get());

    ofVec2f proportion { w / (float) frameBuffer.getWidth(), h / (float) frameBuffer.getHeight() };
    resizeFrameBuffer(w, h);

    for (auto &line : lines)
        line.resize(proportion);

    if (movingLine)
        movingLine->line.resize(proportion);

    for (auto &lines : undo)
        for (auto &line : lines)
            line.resize(proportion);

    for (auto &lines : redo)
        for (auto &line : lines)
            line.resize(proportion);
}

void ofApp::gotMessage(ofMessage msg) {

}

void ofApp::dragEvent(ofDragInfo dragInfo) {

}

void ofApp::redoLines() {
    if (redo.size()) {
        undo.push_back(std::move(lines));
        lines = std::move(redo.back());
        redo.pop_back();
        backgroundOpacity = .5;
    }
}
