#include "ofMain.h"
#include "ofApp.h"

//========================================================================
int main() {

	auto window = new ofAppGlutWindow { };
	window->setGlutDisplayString("rgba double samples>=4");
	ofSetupOpenGL(1024, 768, OF_WINDOW);			// <-------- setup the GL context
	ofSetVerticalSync(true);

	// this kicks off the running of my app
	// can be OF_WINDOW or OF_FULLSCREEN
	// pass in width and height too:
	ofRunApp(new ofApp { });

}
