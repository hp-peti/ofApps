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

void ofApp::clear() {
    if (lines.size()) {
        undoStack.push_back(std::move(lines));
        redoStack.clear();
        lines.clear();
        backgroundOpacity = 1;
    }
}

void ofApp::undo() {
    if (undoStack.size()) {
        redoStack.push_back(std::move(lines));
        lines = std::move(undoStack.back());
        undoStack.pop_back();
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
        clear();
        break;
    case 'z':
    case 'Z':
        if (isKeyPressed.control) {
            if (isKeyPressed.shift) {
                redo();
                break;
            }
            undo();
            break;
        }
        break;
    case OF_KEY_BACKSPACE:
    case OF_KEY_LEFT:
    case 'u':
    case 'U':
        undo();
        break;
    case OF_KEY_RIGHT:
    case 'r':
    case 'R':
        redo();
        break;
    }
}

void ofApp::keyReleased(int key) {
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
    undoStack.push_back(lines);
    redoStack.clear();
}

void ofApp::mousePressed(int x, int y, int button) {

    if (button == OF_MOUSE_BUTTON_LEFT) {
        // TODO: clean up
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
        // TODO: clean up
        auto found = std::find_if(lines.rbegin(), lines.rend(), [x,y] (Line &line) {
            return line.contains( {(float)x, (float)y});
        });
        if (found != lines.rend()) {
            // TODO: clean up
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

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY) {
    if (movingLine) {
        double rate = 5;
        if (isKeyPressed.alt) {
            rate = 1;
        } else if (isKeyPressed.control) {
            rate = 15;
        }
        movingLine->line.rotate({ (float)x, (float)y }, rate * (scrollX + scrollY));
    }
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

    for (auto &lines : undoStack)
        for (auto &line : lines)
            line.resize(proportion);

    for (auto &lines : redoStack)
        for (auto &line : lines)
            line.resize(proportion);
}

void ofApp::gotMessage(ofMessage msg) {

}

void ofApp::dragEvent(ofDragInfo dragInfo) {

}

void ofApp::redo() {
    if (redoStack.size()) {
        undoStack.push_back(std::move(lines));
        lines = std::move(redoStack.back());
        redoStack.pop_back();
        backgroundOpacity = .5;
    }
}
