#include "ofApp.h"

#include <ofFileUtils.h>

#include <complex>
#include <array>
#include <algorithm>

#include <cmath>

#include <ciso646>

using std::chrono::duration_cast;
using namespace std::chrono_literals;

static constexpr auto ENABLE_DURATION = 500ms;
static constexpr auto DISABLE_DURATION = 500ms;

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetWindowTitle("HexTile");
    auto imagefile = [](auto file) {
        static const auto images = ofFilePath::join(ofFilePath::getCurrentExeDir(), "images");
        return ofFilePath::join(images,file);
    };

    concrete.load(imagefile("concrete.jpg"));

    tileImages.black.load(imagefile("black.png"));
    tileImages.grey.load(imagefile("grey.png"));
    tileImages.white.load(imagefile("white.png"));

    sticky.images.resize(4);
    for (auto i : {0, 1, 2})
        sticky.images[i].load(imagefile("sticky" + std::to_string(i) + ".png"));
    sticky.images[3] = sticky.images[1];

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

void ofApp::Tile::fill(TileImages &images) const
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
        ofSetColor(255, 255, 255, 255 * alpha);
        img->draw(box);
    } else {
        switch (color) {
        case TileColor::White:
            ofSetColor(255, 255, 255, 255 * alpha);
            break;
        case TileColor::Black:
            ofSetColor(2, 2, 2, 255 * alpha);
            break;
        case TileColor::Gray:
            ofSetColor(96, 96, 96, 255 * alpha);
            break;
        }
        fill();
    }
}

void ofApp::Tile::draw() const
{
    vertices.draw();
}

void ofApp::Tile::drawCubeIllusion()
{
    const ofPoint c(center.x, center.y);

    switch (orientation)
    {
    case Orientation::Blank:
        break;
    case Orientation::Odd:
        for (auto i: {1,3,5})
            ofDrawLine(c, vertices[i]);
        break;
    case Orientation::Even:
        for (auto i: {0,2,4})
            ofDrawLine(c, vertices[i]);
        break;
    }
}

void ofApp::drawBackground()
{
    if (concrete.isAllocated())
    {
        ofSetColor(200);
        ofRectangle winrect = ofGetWindowRect();
        const auto xinc = concrete.getWidth();
        const auto yinc = concrete.getHeight();
        for (float x = 0; x < winrect.width; x += xinc)
            for (float y = 0; y < winrect.width; y += yinc)
                concrete.draw(x, y, xinc, yinc);
    }
    else
    {
        ofClear(ofColor { 128, 128, 128 });
    }
}

void ofApp::drawShadows()
{
    ofSetLineWidth(5);
    ofPushMatrix();
    ofTranslate(1.f, 1.f);
    for (auto& tile : tiles)
    {
        if (tile.enabled || tile.in_transition)
        {
            ofSetColor(ofColor(0, 0, 0, 128 * tile.alpha));
            tile.draw();
        }
    }
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::draw()
{
    auto now = Clock::now();

    drawBackground();

    ofEnableSmoothing();
    ofEnableAntiAliasing();
    ofEnableAlphaBlending();

    for (auto & tile : tiles)
       tile.update_alpha(now);

    drawShadows();

    for (auto & tile : tiles) {
        if (tile.isVisible()) {
            tile.fill(tileImages);
        }
    }

    ofSetLineWidth(2.5);
    for (auto & tile : tiles)
        if (tile.isVisible()) {
            ofSetColor(20,20,20,160 * tile.alpha);
            tile.draw();
            tile.drawCubeIllusion();
        }

    if (currentTile != nullptr) {
        if (currentTile->enabled or currentTile->in_transition) {
            ofSetColor(getFocusColor(128, currentTile->alpha));
            currentTile->fill();
        }
        if (not currentTile->enabled or currentTile->in_transition) {
            ofSetColor(getFocusColor(255, 1 - currentTile->alpha));
            ofSetLineWidth(1.5);
            currentTile->draw();
        }
    }

    if (sticky.visible) {
        ofSetColor(255);
        sticky.draw();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    auto now = Clock::now();

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
                if (!tile.isVisible())
                    tile.orientation = Orientation::Blank;
                tile.start_enabling(now);
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
                if (!tile.isVisible())
                    tile.orientation = Orientation::Blank;
                tile.start_enabling(now);
            }
        }
        break;
    case 'c':
    case 'C':
        if (ofGetKeyPressed(OF_KEY_SHIFT)) {
            for (auto &tile : tiles) {
                tile.start_disabling(now);
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
                if (!tile.isVisible())
                    tile.orientation = Orientation::Blank;
                tile.start_enabling(now);
            }
        }
        break;
    case 'r':
    case 'R':
        if (not ofGetKeyPressed(OF_KEY_CONTROL)) {
            if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
                for (auto &tile : tiles)
                    if (tile.enabled)
                        tile.changeColorUp(now);
            } else {
                for (auto &tile : tiles)
                    if (tile.enabled)
                        tile.changeColorDown(now);
            }
            break;
        } else {
            if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
                for (auto &tile : tiles)
                    if (tile.enabled)
                        tile.changeToRandomColor(now);
            } else {
                for (auto &tile : tiles) {
                   tile.changeToRandomColor(now);
                }
            }
            break;
        }
    case 'O':
    case 'o':
        if (not ofGetKeyPressed(OF_KEY_CONTROL)) {
            if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
                for (auto &tile : tiles)
                    if (tile.enabled and tile.orientation != Orientation::Blank)
                        tile.changeOrientationUp();
            } else {
                for (auto &tile : tiles)
                    if (tile.enabled and tile.orientation != Orientation::Blank)
                        tile.changeOrientationDown();
            }
            break;
        } else {
            if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
                for (auto &tile : tiles)
                    if (tile.enabled and tile.orientation != Orientation::Blank)
                        tile.changeToRandomNonBlankOrientation();
            } else {
                for (auto &tile : tiles) {
                    if (tile.enabled)
                        tile.changeToRandomOrientation();
                }
            }
            break;
        }
    case 'f':
    case 'F':
        ofToggleFullscreen();
        break;
    case 'S':
    case 's':
        sticky.visible = not sticky.visible;
        if (sticky.visible)
            ofHideCursor();
        else
            ofShowCursor();
        break;
    case OF_KEY_RIGHT:
        if (sticky.direction >= 0)
            ++sticky.direction %= 6;
        break;
    case OF_KEY_LEFT:
        if (sticky.direction >= 0)
        (sticky.direction+= 5) %= 6;
        break;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{

}

void ofApp::updateSticky(int x, int y)
{
    sticky.pos = ofVec2f { (float) (x), (float) (y) };
    if (sticky.visible) {
        if (currentTile != nullptr) {
            sticky.adjustDirection(*currentTile);
        }
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
    findCurrentTile(x, y);
    updateSticky(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
    findCurrentTile(x, y);
    updateSticky(x, y);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
    findCurrentTile(x, y);

    if (currentTile != nullptr) {
        auto now = Clock::now();
        switch (button) {
        case OF_MOUSE_BUTTON_LEFT:
            if (not ofGetKeyPressed(OF_KEY_SHIFT))
                currentTile->changeColorUp(now);
            else
                currentTile->changeColorDown(now);
            resetFocusStartTime();
            break;
        case OF_MOUSE_BUTTON_RIGHT:
            if (currentTile->enabled) {
                currentTile->start_disabling(now);
                resetFocusStartTime();
            }
            break;
        case OF_MOUSE_BUTTON_MIDDLE:
            currentTile->removeOrientation();
            resetFocusStartTime();
            break;
        }
    }
    updateSticky(x, y);
}

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY)
{
    if (not ofGetKeyPressed(OF_KEY_SHIFT)) {
        findCurrentTile(x, y);

        if (currentTile != nullptr and currentTile->isVisible()) {
            if (scrollY > 0)
                currentTile->changeOrientationUp();

            if (scrollY < 0)
                currentTile->changeOrientationDown();
        }
    } else {

        if (scrollY > 0)
            for (auto &tile : tiles)
                if (tile.isVisible())
                    tile.changeOrientationUp();

        if (scrollY < 0)
            for (auto &tile : tiles)
                if (tile.isVisible())
                    tile.changeOrientationDown();
    }
    updateSticky(x, y);
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

ofColor ofApp::getFocusColor(int gray, float alpha)
{
    auto diff = Clock::now() - focus_start;
    auto elapsed_seconds = duration_cast<FloatSeconds>(diff);

    auto a = (unsigned char) (128 * (-cos(M_PI * elapsed_seconds.count()) / 2 + .5));

    return ofColor(gray, gray, gray, a * alpha);
}

void ofApp::resetFocusStartTime()
{
    focus_start = Clock::now();
}

void ofApp::findCurrentTile(float x, float y)
{
    currentTile = findTile(x, y);
    if (currentTile != nullptr and currentTile != previousTile)
    {
        resetFocusStartTime();
    }
    previousTile = currentTile;

}

void ofApp::Tile::update_alpha(const TimeStamp& now)
{
    if (not in_transition)
        return;

    const float final_alpha = enabled ? 1 : 0;

    if ((now - alpha_stop).count() > 0) {
        in_transition = false;
        alpha = final_alpha;
        return;
    }

    auto from_start = duration_cast<FloatSeconds>(now - alpha_start);
    auto total = duration_cast<FloatSeconds>(alpha_stop - alpha_start);

    float progress = from_start.count() / total.count();
    alpha = initial_alpha * (1 - progress) + final_alpha * progress;
}

void ofApp::Tile::start_enabling(const TimeStamp& now)
{
    if (enabled)
        return;
    enabled = true;
    in_transition = true;
    initial_alpha = alpha;
    alpha_start = now;
    alpha_stop = now + ENABLE_DURATION;
    update_alpha(now);
}

void ofApp::Tile::start_disabling(const TimeStamp& now)
{
    if (not enabled)
        return;
    enabled = false;
    in_transition = true;
    initial_alpha = alpha;
    alpha_start = now;
    alpha_stop = now + DISABLE_DURATION;
    update_alpha(now);
}

void ofApp::Sticky::draw()
{
    static const float small = std::sin(M_PI/3);

    ofPushMatrix();
    ofTranslate(pos.x, pos.y);
    if (direction >= 0) {
        ofRotate(90 + 60 * direction);
        if (flip)
            ofScale(1, -1);
        ofScale(small, small);
    }
    const auto &image = images[(++step %= images.size()*10)/10];
    const float w = image.getWidth();
    const float h = image.getHeight();
    image.draw( -w / 2, h * .125 - h, w, h);

    ofPopMatrix();
}

void ofApp::Sticky::adjustDirection(const Tile& tile)
{
    auto adjust_by_closest_vertex_index = [this, &tile](std::initializer_list<int> indices) {
        auto dist2min = tile.radiusSquared() * 4;
        int imin = -1;
        for (auto i : indices) {
            auto dist2 = tile.squareDistanceFromVertex(pos, i);
            if (dist2 <= dist2min) {
                dist2min = dist2;
                imin = i;
            }
        }
        if (imin < 0) {
            direction = -1;
            return;
        }
        direction = imin;
        flip = dist2min <= tile.squareDistanceFromCenter(pos);
    };

    if (tile.isVisible()) {
        if (tile.orientation == Orientation::Even) {
            adjust_by_closest_vertex_index({1,3,5});

        } else if(tile.orientation == Orientation::Odd) {
            adjust_by_closest_vertex_index({0,2,4});
        } else {
            direction = -1;
        }
    } else {
        direction = -1;
    }

}
