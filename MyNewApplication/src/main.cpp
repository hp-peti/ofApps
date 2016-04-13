#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main() {
    ofSetupOpenGL(1024, 768, OF_WINDOW);
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    ofSetEscapeQuitsApp(false);
    ofSetWindowTitle("Hello, world!");
    ofRunApp(new ofApp { });
}
