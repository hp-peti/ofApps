#pragma once

#include "ofMain.h"
#include "ofMath.h"
#include "ofMesh.h"

#include "ofxCv.h"
#include "ofxCvColorImage.h"
#include "ofxCvFloatImage.h"
#include "ofxCvGrayscaleImage.h"

#define _USE_LIVE_VIDEO

class ofApp: public ofBaseApp {
public:
    void setup();
    void update();
    void draw();
    void keyPressed(int key);

    void setupGrid();

    int imageWidth = 320;
    int imageHeight = 240;
private:
    ofVideoGrabber movie;

    ofxCvColorImage colorImg;

    ofxCvGrayscaleImage grayImage;
    ofxCvGrayscaleImage grayImage2;
    ofxCvGrayscaleImage grayDiff;

    ofxCv::ContourFinder contourFinder;

    ofMesh grid;

    bool showLabels = false;
    int threshold = 50;

    bool debug = false;

    std::vector<ofVec3f> points;


    const int gridColumns = 320 / 5;
    const int gridRows = 240 / 5;
};
