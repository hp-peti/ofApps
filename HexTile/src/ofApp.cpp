#include "ofApp.h"

#include <ofFileUtils.h>

#include <complex>
#include <array>
#include <algorithm>

#include <cmath>

//--------------------------------------------------------------
void ofApp::setup() {

    auto imagefile = [](auto file) {
        static const auto images = ofFilePath::join(ofFilePath::getCurrentExeDir(), "images");
        return ofFilePath::join(images,file);
    };


    images.black.load(imagefile("black.png"));
    images.grey.load(imagefile("grey.png"));
    images.white.load(imagefile("white.png"));

    const float radius = 50.0;
    const float row_height = radius * std::sin(M_PI / 3);
    const float col_width = 3 * radius;
    const float col_offset[2] = {radius, 2 * radius + radius * std::cos((float)M_PI/3) };
    const float row_offset = float(row_height / 2);

    const auto NROWS = (int) ceilf(ofGetScreenHeight() / row_height);
    const auto NCOLS = (int) ceilf(ofGetScreenWidth() / col_width);

    for (int row = -1; row <= NROWS; ++row)
    {
        for (int col = -1; col <= NCOLS; col++)
            tiles.emplace_back(
                col_width * col + col_offset[row & 1],
                row_height * row + row_offset,
                radius );

    }


}

//--------------------------------------------------------------
void ofApp::update() {

}
static auto make_unit_roots()
{
    std::array<std::complex<float>, 6> roots;

    // e^(i*x) = cos(x) + i * sin(x)
    for (size_t i = 0 ; i < roots.size(); ++ i)
        roots[i] = exp(std::complex<float>(0, i * M_PI / 3));

    return roots;
}


ofApp::Tile::Tile(float x, float y, float radius)
    : center(x,y), radius(radius)
{

    static const auto roots = make_unit_roots();
    for (int i = 0 ; i < 6; ++ i)
    {
        const float vx = x + radius * roots[i].real();
        const float vy = y + radius * roots[i].imag();
        vertices.addVertex(vx, vy, 0);
    }
    vertices.close();
    box.x = vertices[3].x;
    box.width = vertices[0].x - vertices[3].x;
    box.y = vertices[5].y;
    box.height = vertices[1].y - vertices[5].y;
}

bool ofApp::Tile::isPointInside(float x, float y) const {
    return vertices.inside(x, y);
}

void ofApp::Tile::fill(Images &images) const {
    ofImage *img = nullptr;
    switch (color)
    {
    case TileColor::Black:
        img = &images.black;
        break;
    case TileColor::Gray:
        img = &images.grey;
        break;
    case TileColor::White:
        img = &images.white;
        break;
    }
    if (img != nullptr && img->isAllocated()) {
        ofSetColor(255);
        img->draw(box);
    } else {
        switch (color) {
        case TileColor::White:
            ofSetColor(255);
            break;
        case TileColor::Black:
            ofSetColor(2);
            break;
        case TileColor::Gray:
            ofSetColor(96);
            break;
        }
        ofFill();
        ofBeginShape();
        for (auto &pt : vertices)
        {
            ofVertex(pt.x, pt.y);
        }
        ofEndShape();
        ofNoFill();
    }
}

void ofApp::Tile::draw() const
{
    ofSetColor(20);
    ofSetLineWidth(2.0);
    vertices.draw();
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofClear(ofColor { 128, 128, 128 });
    ofEnableSmoothing();
    ofEnableAntiAliasing();

    for (auto & tile : tiles)
        tile.fill(images);
    for (auto & tile : tiles)
        tile.draw();
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    switch (key)
    {
    case 'i':
    case 'I':
        for (auto &tile : tiles)
            tile.invertColor();
        break;
    case 'r':
        for (auto &tile : tiles)
            tile.changeColorUp();
        break;
    case 'R':
        for (auto &tile : tiles)
            tile.changeColorDown();
        break;
    case 'f':
    case 'F':
        ofToggleFullscreen();
        break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
    auto found = std::find_if(tiles.begin(), tiles.end(), [x,y](auto &tile) {
        return tile.isPointInside(x,y);
    });
    if (found != tiles.end())
    {
        switch (button)
        {
        case OF_MOUSE_BUTTON_LEFT:
            found->changeColorUp();break;
        case OF_MOUSE_BUTTON_RIGHT:
            found->changeColorDown(); break;
        case OF_MOUSE_BUTTON_MIDDLE:
            found->invertColor(); break;
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {

}

