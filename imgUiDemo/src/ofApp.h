#ifndef IMG_UI_DEMO_OF_APP_H_
#define IMG_UI_DEMO_OF_APP_H_

#include "ofMain.h"
#include "ofxImGui.h"
#include "ThemeTest.h"

#include <vector>
#include <string>

#include <memory>

class ofImguiApp : public ofBaseApp {
public:
    ofImguiApp();
    ~ofImguiApp();

    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    void mouseScrolled(float x, float y);

    struct Private;
private:
    ofxImGui::Gui gui;

    std::unique_ptr<Private> p;

};

#endif // IMG_UI_DEMO_OF_APP_H_
