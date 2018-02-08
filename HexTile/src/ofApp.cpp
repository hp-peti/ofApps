#include "ofApp.h"

#include <ofFileUtils.h>

#include <complex>
#include <array>
#include <algorithm>

#include <cmath>

#include <ciso646>

//--------------------------------------------------------------
void ofApp::setup()
{

    auto imagefile = [](auto file) {
        static const auto images = ofFilePath::join(ofFilePath::getCurrentExeDir(), "images");
        return ofFilePath::join(images,file);
    };

    concrete.load(imagefile("concrete.jpg"));

    images.black.load(imagefile("black.png"));
    images.grey.load(imagefile("grey.png"));
    images.white.load(imagefile("white.png"));

    const float radius = 50.0;
    const float row_height = radius * std::sin(M_PI / 3);
    const float col_width = 3 * radius;
    const float col_offset[2] = { radius, 2 * radius + radius * std::cos((float) M_PI / 3) };
    const float row_offset = float(row_height / 2);

    const auto NROWS = (int) ceilf(ofGetScreenHeight() / row_height);
    const auto NCOLS = (int) ceilf(ofGetScreenWidth() / col_width);

    for (int row = -1; row <= NROWS; ++row) {
        for (int col = -1; col <= NCOLS; col++)
            tiles.emplace_back(col_width * col + col_offset[row & 1], row_height * row + row_offset, radius);
    }

    currentTile = nullptr;
}

//--------------------------------------------------------------
void ofApp::update()
{

}
static auto make_unit_roots()
{
    std::array<std::complex<float>, 6> roots { };

    // e^(i*x) = cos(x) + i * sin(x)
    for (size_t i = 0; i < roots.size(); ++i)
        roots[i] = std::exp(std::complex<float>(0, i * M_PI / 3));

    return roots;
}

ofApp::Tile::Tile(float x, float y, float radius) :
    center(x, y),
    radius(radius)
{

    static const auto roots = make_unit_roots();
    for (int i = 0; i < 6; ++i) {
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

bool ofApp::Tile::isPointInside(float x, float y) const
{
    return box.inside(x, y) and vertices.inside(x, y);
}

void ofApp::Tile::fill() const
{
    ofFill();
    ofBeginShape();
    for (auto& pt : vertices) {
        ofVertex(pt.x, pt.y);
    }
    ofEndShape();
    ofNoFill();
}

void ofApp::Tile::fill(Images &images) const
{
    ofImage *img = nullptr;
    switch (color) {
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
    if (img != nullptr and img->isAllocated()) {
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
        fill();
    }
}

void ofApp::Tile::draw() const
{
    vertices.draw();
}

//--------------------------------------------------------------
void ofApp::draw()
{
    if (concrete.isAllocated()) {
        ofSetColor(200);
        ofRectangle winrect = ofGetWindowRect();
        const auto xinc = concrete.getWidth();
        const auto yinc = concrete.getHeight();
        for (float x = 0; x < winrect.width; x += xinc)
            for (float y = 0; y < winrect.width; y += yinc)
                concrete.draw(x, y, xinc, yinc);
    } else {
        ofClear(ofColor { 128, 128, 128 });
    }

    ofEnableSmoothing();
    ofEnableAntiAliasing();
    ofEnableAlphaBlending();

    ofSetColor(ofColor(0,0,0,128));
    ofSetLineWidth(5);
    ofPushMatrix();
    ofTranslate(1.f,1.f);
    for (auto & tile : tiles)
        if (tile.enabled)
            tile.draw();
    ofPopMatrix();
    for (auto & tile : tiles)
        if (tile.enabled)
            tile.fill(images);

    ofSetColor(20,20,20,160);
    ofSetLineWidth(2.5);
    for (auto & tile : tiles)
        if (tile.enabled)
            tile.draw();

    if (currentTile != nullptr) {
        if (currentTile->enabled) {
            ofSetColor(getFocusColor(128));
            currentTile->fill();
        } else {
            ofSetColor(getFocusColor(255));
            ofSetLineWidth(1.5);
            currentTile->draw();
        }
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    switch (key) {
    case 'i':
    case 'I':
        for (auto &tile : tiles)
            if (tile.enabled)
                tile.invertColor();
        break;
    case 'W':
    case 'w':
        if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
            for (auto &tile : tiles)
                if (tile.enabled)
                    tile.color = TileColor::White;
        } else {
            for (auto &tile : tiles) {
                tile.color = TileColor::White;
                tile.enabled = true;
            }
        }
        break;
    case 'B':
    case 'b':
        if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
            for (auto &tile : tiles)
                if (tile.enabled)
                    tile.color = TileColor::Black;
        } else {
            for (auto &tile : tiles) {
                tile.color = TileColor::Black;
                tile.enabled = true;
            }
        }
        break;
    case 'c':
    case 'C':
        if (ofGetKeyPressed(OF_KEY_SHIFT)) {
            for (auto &tile : tiles) {
                tile.color = TileColor::White;
                tile.enabled = false;
            }
        }
        break;
    case 'G':
    case 'g':
        if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
            for (auto &tile : tiles)
                if (tile.enabled)
                    tile.color = TileColor::Gray;
        } else {
            for (auto &tile : tiles) {
                tile.color = TileColor::Gray;
                tile.enabled = true;
            }
        }
        break;
    case 'r':
    case 'R':
        if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
            for (auto &tile : tiles)
                if (tile.enabled)
                    tile.changeColorUp();
        } else {
            for (auto &tile : tiles)
                if (tile.enabled)
                    tile.changeColorDown();
        }
        break;
    case 'f':
    case 'F':
        ofToggleFullscreen();
        break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
    findCurrentTile(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
    findCurrentTile(x, y);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
    findCurrentTile(x, y);

    if (currentTile != nullptr) {
        switch (button) {
        case OF_MOUSE_BUTTON_LEFT:
            if (not ofGetKeyPressed(OF_KEY_SHIFT))
                currentTile->changeColorUp();
            else
                currentTile->changeColorDown();
            resetStartTime();
            break;
        case OF_MOUSE_BUTTON_RIGHT:
            if (currentTile->enabled) {
                currentTile->enabled = false;
                resetStartTime();
            }
            break;
        case OF_MOUSE_BUTTON_MIDDLE:
            currentTile->invertColor();
            resetStartTime();
            break;
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{
    currentTile = nullptr;
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
    currentTile = nullptr;
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{

}

ofApp::Tile* ofApp::findTile(float x, float y)
{
    if (currentTile != nullptr)
        if (currentTile->isPointInside(x, y))
            return currentTile;

    auto found = std::find_if(tiles.begin(), tiles.end(), [x,y](auto &tile) {
        return tile.isPointInside(x,y);
    });

    if (found == tiles.end())
        return nullptr;

    return &*found;
}

ofColor ofApp::getFocusColor(int gray)
{
    auto diff = std::chrono::system_clock::now() - start_time;
    auto elapsed_seconds = std::chrono::duration_cast<std::chrono::duration<float>>(diff).count();

    auto a = (unsigned char) (128 * (-cos(M_PI * elapsed_seconds) / 2 + .5));

    return ofColor(gray, gray, gray, a);
}

void ofApp::resetStartTime()
{
    start_time = std::chrono::system_clock::now();
}

void ofApp::findCurrentTile(float x, float y)
{
    currentTile = findTile(x, y);
    if (currentTile != nullptr and currentTile != previousTile)
    {
        resetStartTime();
    }
    previousTile = currentTile;

}
