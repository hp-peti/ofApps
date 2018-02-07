#include "ofApp.h"

using namespace ofxCv;
using namespace cv;

void ofApp::setup() {
    ofSetVerticalSync(true);
    ofBackground(0);

    movie.setDeviceID(0);
    movie.setDesiredFrameRate(30);

    auto &w = imageWidth;
    auto &h = imageHeight;
    movie.setup(w, h);

    w = movie.getWidth();
    h = movie.getHeight();

    colorImg.allocate(w, h);
    grayImage.allocate(w, h);
    grayImage2.allocate(w, h);
    grayDiff.allocate(w, h);

    contourFinder.setMinAreaRadius(10);
    contourFinder.setMaxAreaRadius(30);
    contourFinder.setThreshold(5);
    // wait for 1/3 second before forgetting something
    contourFinder.getTracker().setPersistence(10);
    // an object can move up to 32 pixels per frame
    contourFinder.getTracker().setMaximumDistance(5);

    setupGrid();
}

void ofApp::update() {
    movie.update();
    if (movie.isFrameNew()) {

        colorImg.setFromPixels(movie.getPixels());
        grayImage = colorImg;
        grayImage.contrastStretch();
        grayImage2 = grayImage;
        grayImage.blur(15);
        grayImage2.blur(21);
#if 0
        if (bLearnBakground)
        {
            floatBg = grayImage;
            bLearnBakground = false;
        }
        else
        {
            auto eta = .0125f;
            floatBg *= 1.0 - eta;
            floatBg.addWeighted(grayImage, eta);
        }
        grayBg.setFromFloatImage(floatBg);
        grayDiff.absDiff(grayBg, grayImage);
        grayDiff.threshold(threshold);
#endif
        grayDiff = grayImage;
        grayDiff -= grayImage2;
        grayDiff.contrastStretch();
        grayDiff.blur(5);
        grayDiff.threshold(threshold);
        grayDiff.dilate_3x3();
        contourFinder.findContours(grayDiff);

        auto w = gridColumns + 1;
        auto ptIndex = [w](int x, int y) {
            return x + y * w;
        };

        auto c = gridColumns;
        auto r = gridRows;

        auto clampx = [c]( int x ) {return CLAMP(x, 0, c);};
        auto clampy = [r]( int y ) {return CLAMP(y, 0, r);};

        auto &tracker = contourFinder.getTracker();
        for (int i = 0; i < contourFinder.size(); ++i) {
            int label = tracker.getLabelFromIndex(i);
            if (tracker.getAge(label) < 2) {
                continue;
            }
            auto velocity = toOf(tracker.getVelocity(i));
            auto &rect = tracker.getCurrent(label);

            int bx = clampx(floorf(rect.x * gridColumns / imageWidth));
            int by = clampy(floorf(rect.y * gridRows / imageHeight));
            int ex = clampx(ceilf(bx + rect.width * gridColumns / imageWidth));
            int ey = clampy(ceilf(bx + rect.height * gridRows / imageHeight));

            //auto vertices = grid.getVerticesPointer();
            auto texCoords = grid.getTexCoordsPointer();

            for (int y = by; y < ey; ++y) {
                for (int x = bx; x < ex; ++x) {
                    int index = ptIndex(x, y);
                    texCoords[index] += velocity;
                }
            }
        }

        static const auto eta = 0.1f;
        static const auto neta = 1 - eta;

        auto vertices = grid.getVerticesPointer();
        auto texCoords = grid.getTexCoordsPointer();
        for (int i = 0, n = std::min(grid.getNumTexCoords(), grid.getNumVertices()); i < n; ++i) {
            texCoords[i] = texCoords[i] * neta + ofVec2f{vertices[i].x, vertices[i].y} * eta;
        }

    }
}

void ofApp::draw() {
    ofSetBackgroundAuto(showLabels);
    if (debug) {
        RectTracker& tracker = contourFinder.getTracker();

        colorImg.draw(0, 0);
        grayImage.draw(imageWidth, 0);
        grayImage2.draw(imageWidth * 2, 0);
        grayDiff.draw(0, imageHeight);

        if (showLabels) {
            ofSetColor(255);
            //movie.draw(0, 0);
            contourFinder.draw();
            for (int i = 0; i < contourFinder.size(); i++) {
                ofPoint center = toOf(contourFinder.getCenter(i));
                ofPushMatrix();
                ofTranslate(center.x, center.y);
                int label = contourFinder.getLabel(i);
                string msg = ofToString(label) + ":" + ofToString(tracker.getAge(label));
                ofDrawBitmapString(msg, 0, 0);
                ofVec2f velocity = toOf(contourFinder.getVelocity(i));
                ofScale(5, 5);
                ofDrawLine(0, 0, velocity.x, velocity.y);
                ofPopMatrix();
            }
        } else {
            for (int i = 0; i < contourFinder.size(); i++) {
                unsigned int label = contourFinder.getLabel(i);
                // only draw a line if this is not a new label
                if (tracker.existsPrevious(label)) {
                    // use the label to pick a random color
                    ofSeedRandom(label << 24);
                    ofSetColor(ofColor::fromHsb(ofRandom(255), 255, 255));
                    // get the tracked object (cv::Rect) at current and previous position
                    const cv::Rect& previous = tracker.getPrevious(label);
                    const cv::Rect& current = tracker.getCurrent(label);
                    // get the centers of the rectangles
                    ofVec2f previousPosition(previous.x + previous.width / 2, previous.y + previous.height / 2);
                    ofVec2f currentPosition(current.x + current.width / 2, current.y + current.height / 2);
                    ofDrawLine(previousPosition, currentPosition);
                }
            }
        }

        // this chunk of code visualizes the creation and destruction of labels
        const vector<unsigned int>& currentLabels = tracker.getCurrentLabels();
        const vector<unsigned int>& previousLabels = tracker.getPreviousLabels();
        const vector<unsigned int>& newLabels = tracker.getNewLabels();
        const vector<unsigned int>& deadLabels = tracker.getDeadLabels();
        ofSetColor(cyanPrint);
        for (int i = 0; i < currentLabels.size(); i++) {
            int j = currentLabels[i];
            ofDrawLine(j, 0, j, 4);
        }
        ofSetColor(magentaPrint);
        for (int i = 0; i < previousLabels.size(); i++) {
            int j = previousLabels[i];
            ofDrawLine(j, 4, j, 8);
        }
        ofSetColor(yellowPrint);
        for (int i = 0; i < newLabels.size(); i++) {
            int j = newLabels[i];
            ofDrawLine(j, 8, j, 12);
        }
        ofSetColor(ofColor::white);
        for (int i = 0; i < deadLabels.size(); i++) {
            int j = deadLabels[i];
            ofDrawLine(j, 12, j, 16);
        }

        ofSetColor(ofColor::white);
        ofPushMatrix();
        ofTranslate( { (float) imageWidth, (float) imageHeight });
        grid.drawVertices();
        ofPopMatrix();
        ofPushMatrix();
        ofTranslate( { (float) imageWidth * 2, (float) imageHeight });
        grid.drawWireframe();
        ofPopMatrix();
        ofPushMatrix();
        ofTranslate( { 0, (float) imageHeight * 2 });
        ofEnableArbTex();
        colorImg.getTexture().bind();
        grid.drawFaces();
        colorImg.getTexture().unbind();
        ofDisableArbTex();
        ofPopMatrix();
    } else {
        ofSetColor(ofColor::white);
        ofPushMatrix();
        //auto size = ofGetWindowSize();
        ofScale(3,3,1);
        //ofScale( size.x / imageWidth, size.y / imageHeight );
        ofEnableArbTex();
        movie.getTexture().bind();
        grid.drawFaces();
        movie.getTexture().unbind();
        ofDisableArbTex();
        ofPopMatrix();
    }
}

void ofApp::keyPressed(int key) {
    switch (key) {
    case ' ':
        showLabels = !showLabels;
        break;
    case '+':
        threshold = std::min(threshold + 1, 255);
        break;
    case '-':
        threshold = std::max(threshold - 1, 0);
        break;
    case 'd':
        debug = !debug;
        break;
    case OF_KEY_RETURN:
        setupGrid();
        break;
    }
}

void ofApp::setupGrid() {

    grid.clear();
    grid.setupIndicesAuto();

    for (int y = 0; y <= gridRows; ++y) {
        for (int x = 0; x <= gridColumns; ++x) {
            ofVec2f pt { (float) x * (imageWidth / gridColumns), (float) y * (imageHeight / gridRows) };
            grid.addVertex(pt);
            grid.addTexCoord(pt);
        }
    }

    auto w = gridColumns + 1;
    auto ptIndex = [w](int x, int y) {
        return x + y * w;
    };

    for (int y = 0; y < gridRows; ++y) {
        for (int x = 0; x < gridColumns; ++x) {
            if ((x + y) % 2) {
                grid.addTriangle(ptIndex(x, y), ptIndex(x + 1, y), ptIndex(x, y + 1));
                grid.addTriangle(ptIndex(x, y + 1), ptIndex(x + 1, y), ptIndex(x + 1, y + 1));
            } else {
                grid.addTriangle(ptIndex(x, y), ptIndex(x + 1, y + 1), ptIndex(x, y + 1));
                grid.addTriangle(ptIndex(x, y), ptIndex(x + 1, y), ptIndex(x + 1, y + 1));
            }
        }
    }

}
