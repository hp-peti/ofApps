#include "ofApp.h"

#include <ciso646>

struct ofImguiApp::Private
{
    //backgroundColor is stored as an ImVec4 type but can handle ofColor
    ImVec4 backgroundColor = ofColor(114, 144, 154);
    bool show_test_window = true;
    bool show_another_window = false;

    float floatValue = 0.0f;
    ofImage imageButtonSource;
    GLuint imageButtonID;

    ofPixels pixelsButtonSource;
    GLuint pixelsButtonID;

    ofTexture textureSource;
    GLuint textureSourceID;

    std::vector<std::string> fileNames;
    std::vector<ofFile> files;

    int currentListBoxIndex = 0;
    int currentFileIndex = 0;

    bool doSetTheme = false;
    bool doThemeColorsWindow = false;
};

//--------------------------------------------------------------
void ofImguiApp::setup()
{
    ofSetLogLevel(OF_LOG_VERBOSE);
    ofHideCursor();

    //required call
    gui.setup();

    auto &imgui = ImGui::GetIO();

    imgui.MouseDrawCursor = true;
    imgui.IniFilename = nullptr;

    //load your own ofImage
    p->imageButtonSource.load("of.png");
    p->imageButtonID = gui.loadImage(p->imageButtonSource);

    //or have the loading done for you if you don't need the ofImage reference
    //imageButtonID = gui.loadImage("of.png");

    //can also use ofPixels in same manner
    ofLoadImage(p->pixelsButtonSource, "of_upside_down.png");
    p->pixelsButtonID = gui.loadPixels(p->pixelsButtonSource);

    //and alt method
    //p->pixelsButtonID = gui.loadPixels("of_upside_down.png");

    //pass in your own texture reference if you want to keep it
    p->textureSourceID = gui.loadTexture(p->textureSource, "of_upside_down.png");

    //or just pass a path
    //p->textureSourceID = gui.loadTexture("of_upside_down.png");

    ofLogVerbose() << "textureSourceID: " << p->textureSourceID;

    const ofDirectory dataDirectory(ofToDataPath("", true));

    p->files = dataDirectory.getFiles();

    for (const auto & file : p->files)
        p->fileNames.push_back(file.getFileName());
}

//--------------------------------------------------------------
void ofImguiApp::update()
{

    if (p->doSetTheme) {
        p->doSetTheme = false;
        gui.setTheme(new ThemeTest());

    }
}

//--------------------------------------------------------------
void ofImguiApp::draw()
{

    //backgroundColor is stored as an ImVec4 type but is converted to ofColor automatically

    ofSetBackgroundColor(p->backgroundColor);

    //required to call this at beginning
    gui.begin();

    //In between gui.begin() and gui.end() you can use ImGui as you would anywhere else

    // 1. Show a simple window
    {
        ImGui::Text("Hello, world!");
        ImGui::SliderFloat("Float", &p->floatValue, 0.0f, 1.0f);

        //this will change the app background color
        ImGui::ColorEdit3("Background Color", (float*) &p->backgroundColor);
        if (ImGui::Button("Demo Window")) {
            p->show_test_window = not p->show_test_window;
        }

        if (ImGui::Button("Another Window")) {
            //bitwise XOR
            p->show_another_window ^= true;

        }
        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                    ImGui::GetIO().Framerate);
    }
    // 2. Show another window, this time using an explicit ImGui::Begin and ImGui::End
    if (p->show_another_window) {
        //note: ofVec2f and ImVec2f are interchangeable
        ImGui::SetNextWindowSize(ofVec2f(200, 100), ImGuiSetCond_FirstUseEver);
        ImGui::Begin("Another Window", &p->show_another_window);
        ImGui::Text("Hello");
        ImGui::End();
    }

    // 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowDemoWindow()
    if (p->show_test_window) {
        ImGui::SetNextWindowPos(ofVec2f(650, 20), ImGuiSetCond_FirstUseEver);
        ImGui::ShowDemoWindow(&p->show_test_window);
    }

    if (not p->fileNames.empty()) {

        //ofxImGui::VectorListBox allows for the use of a vector<string> as a data source
        if (ofxImGui::VectorListBox("VectorListBox", &p->currentListBoxIndex, p->fileNames)) {
            ofLog() << " VectorListBox FILE PATH: " << p->files[p->currentListBoxIndex].getAbsolutePath();
        }

        //ofxImGui::VectorCombo allows for the use of a vector<string> as a data source
        if (ofxImGui::VectorCombo("VectorCombo", &p->currentFileIndex, p->fileNames)) {
            ofLog() << "VectorCombo FILE PATH: " << p->files[p->currentFileIndex].getAbsolutePath();
        }
    }

    //GetImTextureID is a static function define in Helpers.h that accepts ofTexture, ofImage, or GLuint
    if (ImGui::ImageButton(GetImTextureID(p->imageButtonID), ImVec2(200, 200))) {
        ofLog() << "PRESSED";
    }

    //or do it manually
    ImGui::Image((ImTextureID) (uintptr_t) p->textureSourceID, ImVec2(200, 200));

    ImGui::Image(GetImTextureID(p->pixelsButtonID), ImVec2(200, 200));

    if (p->doThemeColorsWindow) {
        gui.openThemeColorWindow();

    }

    //required to call this at end
    gui.end();

}

//--------------------------------------------------------------
void ofImguiApp::keyPressed(int key)
{

    ofLogVerbose(__FUNCTION__) << key;
    switch (key) {
    case 't':
        {
            p->doThemeColorsWindow = not p->doThemeColorsWindow;
            break;
        }
    case 'c':
        {
            p->doSetTheme = not p->doSetTheme;
            break;
        }
    case 's':
        {
            break;
        }
    case 'f':
        {
            ofToggleFullscreen();
        }
    }

}

//--------------------------------------------------------------
void ofImguiApp::keyReleased(int key)
{
    ofLogVerbose(__FUNCTION__) << key;

}

ofImguiApp::ofImguiApp() :
    p { std::make_unique<Private>() }
{
}

ofImguiApp::~ofImguiApp() = default;

void ofImguiApp::mouseScrolled(float x, float y)
{
    ofLogVerbose(__FUNCTION__) << "x: " << x << " y: " << y;
}
//--------------------------------------------------------------
void ofImguiApp::mouseMoved(int x, int y)
{

}

//--------------------------------------------------------------
void ofImguiApp::mouseDragged(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofImguiApp::mousePressed(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofImguiApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofImguiApp::windowResized(int w, int h)
{

}

//--------------------------------------------------------------
void ofImguiApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void ofImguiApp::dragEvent(ofDragInfo dragInfo)
{

}
