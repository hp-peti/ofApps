#include "ofApp.h"
#include "ofGraphics.h"

#include <chrono>
#include <cassert>

//--------------------------------------------------------------
void ofApp::setup() {
    ofSetBackgroundAuto(false);
    resizeFrameBuffer(ofGetWidth(), ofGetHeight());
}

//--------------------------------------------------------------
void ofApp::update() {

    auto now = TransitionBase::clockNow();
    color.update(now);
    for (auto &line : lines)
        line.update(now);
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
    for (auto& line : lines) {
        line.draw();
    }
    frameBuffer.end();
    ofPopStyle();
}

//--------------------------------------------------------------
void ofApp::draw() {
    drawToFrameBuffer();

    ofDisableAlphaBlending();
    ofDisableDepthTest();
    frameBuffer.draw(0, 0, ofGetWidth(), ofGetHeight());
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {

    switch (key) {
    case 'f':
        ofToggleFullscreen();
        break;
    case 'q':
        ofExit(0);
        break;
    case OF_KEY_DEL:
    case 'c':
        undo.emplace_back(std::move(lines));
        redo.clear();
        lines.clear();
        backgroundOpacity = 1;
        break;
    case 'z':
    case 'Z':
        if (CONTROL_PRESSED) {
            if (SHIFT_PRESSED) {
                goto redo;
            }
            goto undo;
        }
        break;
    case OF_KEY_LEFT:
    case OF_KEY_BACKSPACE:
    case 'u':
        undo: if (undo.size()) {
            redo.emplace_back(std::move(lines));
            lines = std::move(undo.back());
            undo.pop_back();
            backgroundOpacity = .5;
        }
        break;
    case OF_KEY_RIGHT:
    case 'r':
        redo: if (redo.size()) {
            undo.emplace_back(std::move(lines));
            lines = std::move(redo.back());
            redo.pop_back();
        }
        break;
    case OF_KEY_SHIFT:
        SHIFT_PRESSED = true;
        break;
    case OF_KEY_ALT:
        ALT_PRESSED = true;
        break;
    case OF_KEY_CONTROL:
        CONTROL_PRESSED = true;
        break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
    switch (key) {
    case OF_KEY_SHIFT:
        SHIFT_PRESSED = false;
        break;
    case OF_KEY_ALT:
        ALT_PRESSED = false;
        break;
    case OF_KEY_CONTROL:
        CONTROL_PRESSED = false;
        break;
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
    if (button == OF_MOUSE_BUTTON_1) {
        if (lines.empty()) {
            return mousePressed(x, y, button);
        }
        auto &line = lines.back();
        line.add(x, y);
        if (line.lastDistance() < 8) {
            line.removeLast();
        }
    }
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    if (button == OF_MOUSE_BUTTON_1) {
        undo.push_back(lines);
        redo.clear();
        Line::Properties::Ptr properties { };
        if (not lines.empty() and SHIFT_PRESSED) {
            if (CONTROL_PRESSED) {
                return mouseDragged(x, y, button);
            }
            properties = lines.back().getProperties();
        } else {
            properties = Line::Properties::create();
        }
        Line line { (float) x, (float) y, properties };
        lines.push_back(std::move(line));
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
//    if (button == OF_MOUSE_BUTTON_1) {
//        if (lines.back().empty() or ((lines.back().size() < 2) and not (ALT_PRESSED))) {
//            lines.pop_back();
//            undo.pop_back();
//        }
//    }
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

void ofApp::resizeFrameBuffer(int w, int h) {
    frameBuffer.clear();
    frameBuffer.allocate(w, h, GL_RGBA);
    backgroundOpacity = 1;
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

    ofClear(color.get());

    ofVec2f proportion { w/(float)frameBuffer.getWidth(), h/(float)frameBuffer.getHeight()};
    resizeFrameBuffer(w, h);

    for (auto &line : lines) {
        line.resize(proportion);
    }
    for (auto &lines : undo) {
        for (auto &line : lines) {
            line.resize(proportion);
        }
    }
    for (auto &lines : redo) {
        for (auto &line : lines) {
            line.resize(proportion);
        }
    }
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

