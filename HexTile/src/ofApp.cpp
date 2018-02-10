#include "ofApp.h"

#include <ofFileUtils.h>

#include <array>
#include <algorithm>

#include <cmath>
#include <set>
#include <deque>
#include <ciso646>

#ifdef _DEBUG
#include <iostream>
#include <iomanip>
using std::clog;
using std::endl;
#endif

using std::array;
using std::complex;
using std::chrono::duration_cast;
using namespace std::chrono_literals;

static constexpr float TILE_EDGE_MM = 82.f; // mm
static constexpr float TILE_SEPARATION_MM = 3.5; // mm

static constexpr float PIX_PER_MM = .5f;

static constexpr float TILE_RADIUS_PIX = (TILE_EDGE_MM + TILE_SEPARATION_MM / 2) * PIX_PER_MM;
static constexpr float LINE_WIDTH_PIX = (TILE_SEPARATION_MM) * PIX_PER_MM;

static constexpr auto ENABLE_DURATION = 250ms;
static constexpr auto DISABLE_DURATION = 750ms;
static constexpr auto STEP_DURATION = 200ms;
static constexpr auto ARROW_COLOR_PERIOD = 2s;
static constexpr auto ARROW_SHORT_LENGTH_PERIOD = 0.75s;
static constexpr auto ARROW_LONG_LENGTH_PERIOD = 1.5s;

template <typename T>
inline ofVec2f toVec2f(const complex<T> &vec)
{
    return ofVec2f(vec.real(), vec.imag());
}

template <typename T>
inline ofVec3f toVec3f(const complex<T> &vec)
{
    return ofVec3f(vec.real(), vec.imag());
}

inline ofVec3f toVec3f(const ofVec2f &vec)
{
    return ofVec3f(vec.x, vec.y);
}

//--------------------------------------------------------------
void ofApp::setup()
{
    ofSetWindowTitle("HexTile");
    ofSetBackgroundAuto(false);

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

    createTiles();

    currentTile = nullptr;
}

void ofApp::createTiles()
{
    const float radius = TILE_RADIUS_PIX;
    const float row_height = radius * std::sin(M_PI / 3);
    const float col_width = 3 * radius;
    const float col_offset[2] = { radius, 2 * radius + radius * std::cos((float)M_PI / 3) };
    const float row_offset = float(row_height / 2);

    const auto NROWS = (int)ceilf(ofGetScreenHeight() / row_height);
    const auto NCOLS = (int)ceilf(ofGetScreenWidth() / col_width);

    for (int row = -1; row <= NROWS; ++row) {
        for (int col = -1; col <= NCOLS; col++)
            tiles.emplace_back(col_width * col + col_offset[row & 1], row_height * row + row_offset, radius);
    }

    for (auto tile = tiles.begin(); tile != tiles.end(); ++tile) {
        for (auto other_tile = tiles.begin(); other_tile != tile; ++other_tile)
            tile->connectIfNeighbour(&*other_tile);
    }
}

//--------------------------------------------------------------
void ofApp::update()
{
    if (sticky.visible)
    {
        updateSticky();
        sticky.updateStep(Clock::now());
    }

    updateSelected();
}
static auto make_unit_roots()
{
    array<complex<float>, 6> roots { };

    // e^(i*x) = cos(x) + i * sin(x)
    for (size_t i = 0; i < roots.size(); ++i)
        roots[i] = exp(complex<float>(0, i * M_PI / 3));

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

void ofApp::Tile::connectIfNeighbour(Tile * other)
{
    if (other == nullptr || other == this)
        return;

    if (center.squareDistance(other->center) > (radius + other->radius) * (radius + other->radius))
        return;

    for (const auto *n : neighbours)
        if (n == other)
            return;

#ifdef _DEBUG
    clog << "connect " << this << " to " << other << endl;
#endif
    neighbours.push_back(other);
    other->neighbours.push_back(this);
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
        ofSetColor(240);
        ofRectangle winrect = ofGetWindowRect();
        ofRectangle slab { 0, 0, concrete.getWidth(), concrete.getHeight()};
        slab.scale(PIX_PER_MM, PIX_PER_MM);
        for (slab.x = 0; slab.x < winrect.width; slab.x += slab.width)
            for (slab.y = 0; slab.y < winrect.width; slab.y += slab.height)
                concrete.draw(slab);
    }
    else
    {
        ofBackgroundGradient(ofColor { 120, 120, 120 }, ofColor { 160, 160, 160 });
    }
}

void ofApp::drawShadows()
{
    ofSetLineWidth(LINE_WIDTH_PIX);
    ofPushMatrix();
    ofTranslate(LINE_WIDTH_PIX / 2, LINE_WIDTH_PIX / 2);
    for (auto& tile : tiles)
    {
        if (tile.isVisible())
        {
            ofSetColor(ofColor(0, 0, 0, 128 * tile.alpha));
            tile.draw();
        }
    }
    ofPopMatrix();
}

void ofApp::drawSticky()
{
    if (sticky.show_arrow) {
        ofSetLineWidth(2);
        ofSetColor(getFocusColorMix(ofColor(32, 32, 32, 196), ofColor(160, 160, 160, 240), ARROW_COLOR_PERIOD));
        if (sticky.visible) {
            sticky.drawArrow(TILE_RADIUS_PIX/2 + TILE_RADIUS_PIX/10 * getFocusAlpha(ARROW_SHORT_LENGTH_PERIOD), 10);
        } else {
            sticky.drawNormal(TILE_RADIUS_PIX + TILE_RADIUS_PIX/5 * getFocusAlpha(ARROW_LONG_LENGTH_PERIOD), 15);
        }
    }
    if (sticky.visible) {
        ofSetColor(255);
        sticky.draw();
    }
}

void ofApp::drawFocus()
{
    if (!enableFlood) {
        drawTileFocus(currentTile);
    } else {
        for (auto *tile : selectedTiles) {
            drawTileFocus(tile);
        }
    }
}

void ofApp::drawTileFocus(Tile * tile)
{
    if (tile == nullptr)
        return;

    if (tile->enabled or tile->in_transition) {
        ofSetColor(getFocusColor(128, tile->alpha));
        tile->fill();
    }
    if (not tile->enabled or tile->in_transition) {
        ofSetColor(getFocusColor(255, 1 - tile->alpha));
        ofSetLineWidth(1.5);
        tile->draw();
    }
}

void ofApp::selectSimilarNeighbours(Tile *from)
{
    if (from == nullptr)
        return;

    auto *found = &selectedTiles;
    found->clear();

    std::set<Tile *> visited;
    std::deque<Tile *> queue;

    const auto state = from->getStateForFloodFill();

    auto is_same = [&state](Tile *tile) {
        return tile->getStateForFloodFill() == state;
    };

    auto visit = [&visited](Tile *tile) {
        return visited.insert(tile).second == true;
    };

    auto push = [&queue, found](Tile *tile) {
        found->push_back(tile);
        queue.push_back(tile);
    };

    auto pop = [&queue]() -> Tile * {
        if (queue.empty())
            return nullptr;
        auto *p = queue.front();
        queue.pop_front();
        return p;
    };

    push(from);
    visit(from);

    while (auto *tile = pop()) {
        for (auto *next : tile->getNeighbours()) {
            if (visit(next) && is_same(next))
                push(next);
        }
    }
}

// b == a * ( 1 - alpha ) + x * alpha
// c == b * ( 1 - alpha ) + x * alpha
//   == (a * ( 1 - alpha ) + x * alpha) * (1 - alpha) + x * alpha
//   == a * (1 - (2*alpha - alpha^2) ) + x * (2*alpha - alpha^2)

template <typename T>
inline T doubleAlpha(const T &alpha) {
    return 2 * alpha - alpha * alpha;
}

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

    ofSetLineWidth(LINE_WIDTH_PIX);
    for (auto & tile : tiles) {
        if (tile.isVisible()) {
            const float lineAlpha = tile.alpha * 160 / 255;
            ofSetColor(20, 20, 20, 255 * lineAlpha);
            tile.draw();

            // as if drawn 2 times
            ofSetColor(20, 20, 20, 255 * doubleAlpha(lineAlpha));
            tile.drawCubeIllusion();
        }
    }

    drawFocus();
 
    drawSticky();
}

constexpr int KEY_CTRL_(const char ch)
{
    return ch >= 'A' && ch <= 'Z' ? ch - 'A' + 1 : ch;
}

static const auto shift = []() { return ofGetKeyPressed(OF_KEY_SHIFT); };
static const auto ctrl_or_alt = []() { return    ofGetKeyPressed(OF_KEY_CONTROL)
                                              or ofGetKeyPressed(OF_KEY_ALT)
                                              or ofGetKeyPressed(OF_KEY_LEFT_ALT)
                                              or ofGetKeyPressed(OF_KEY_RIGHT_ALT)
                                              or ofGetKeyPressed(OF_KEY_COMMAND); };


//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    auto now = Clock::now();
    
    switch (key) {
    case 'i':
    case 'I':
        for (auto &tile : tiles)
            if (tile.isVisible())
                tile.invertColor();
        break;
    case 'W':
    case 'w':
        if (not shift()) {
            for (auto &tile : tiles)
                if (tile.isVisible())
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
            if (not shift()) {
                for (auto &tile : tiles)
                    if (tile.isVisible())
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
        if (shift()) {
            for (auto &tile : tiles) {
                tile.start_disabling(now);
            }
        }
        break;
    case KEY_CTRL_('G'):
    case 'G':
    case 'g':
        if (not shift()) {
            for (auto &tile : tiles)
                if (tile.isVisible())
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
    case KEY_CTRL_('R'):
    case 'r':
    case 'R':
        if (not ctrl_or_alt()) {
            if (not shift()) {
                for (auto &tile : tiles)
                    if (tile.isVisible())
                        tile.changeColorUp(now);
            } else {
                for (auto &tile : tiles)
                    if (tile.isVisible())
                        tile.changeColorDown(now);
            }
            break;
        } else {
            if (not shift()) {
                for (auto &tile : tiles)
                    if (tile.isVisible())
                        tile.changeToRandomColor(now);
            } else {
                for (auto &tile : tiles) {
                   tile.changeToRandomColor(now);
                }
            }
            break;
        }
    case KEY_CTRL_('O'):
    case 'O':
    case 'o':
        if (not ctrl_or_alt()) {
            if (not shift()) {
                for (auto &tile : tiles)
                    if (tile.isVisible() and tile.orientation != Orientation::Blank)
                        tile.changeOrientationUp();
            } else {
                for (auto &tile : tiles)
                    if (tile.isVisible() and tile.orientation != Orientation::Blank)
                        tile.changeOrientationDown();
            }
            break;
        } else {
            if (not shift()) {
                for (auto &tile : tiles)
                    if (tile.isVisible() and tile.orientation != Orientation::Blank)
                        tile.changeToRandomNonBlankOrientation();
            } else {
                for (auto &tile : tiles) {
                    if (tile.isVisible())
                        tile.changeToRandomOrientation();
                }
            }
            break;
        }
    case KEY_CTRL_('F'):
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
    case 'A':
    case 'a':
    case KEY_CTRL_('A'):
        sticky.show_arrow = not sticky.show_arrow;
        break;
    case OF_KEY_CONTROL:
        if (not enableFlood) {
            enableFlood = true;
            freezeFlood = false;
        }
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
#if defined (_WIN32) //&& defined(_DEBUG)
    char str[256];
    sprintf(str, "key pressed: 0x%.2x (%c)", key, key);
    OutputDebugStringA(str);
#endif
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{
    switch (key) {
    case OF_KEY_CONTROL:
        enableFlood = false;
        break;
    }
}

void ofApp::updateSticky(int x, int y)
{
    sticky.pos = ofVec2f { (float) (x), (float) (y) };
    if (sticky.visible or sticky.show_arrow) {
        if (currentTile != nullptr) {
            sticky.adjustDirection(*currentTile);
        }
    }
}

void ofApp::updateSelected()
{
    if (not enableFlood) {
        selectedTiles.clear();
        if (currentTile != nullptr) {
            selectedTiles.push_back(currentTile);
        }
    } else {
        if (not freezeFlood)
            selectSimilarNeighbours(currentTile);
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
    updateSelected();
    auto now = Clock::now();
        switch (button) {
        case OF_MOUSE_BUTTON_LEFT:
            if (not shift())
                for (auto *tile : selectedTiles)
                    tile->changeColorUp(now);
            else
                for (auto *tile : selectedTiles)
                    tile->changeColorDown(now);
            freezeFlood = true;
            resetFocusStartTime();
            break;
        case OF_MOUSE_BUTTON_RIGHT:
            for (auto *tile : selectedTiles) {
                if (tile->enabled) {
                    tile->start_disabling(now);
                }
            }
            resetFocusStartTime();
            freezeFlood = true;
            break;
        case OF_MOUSE_BUTTON_MIDDLE:
            for (auto *tile : selectedTiles)
                tile->removeOrientation();
            freezeFlood = true;
            resetFocusStartTime();
            break;
        }
    updateSticky(x, y);
}

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY)
{
    findCurrentTile(x, y);
    updateSelected();

    if (not shift()) {
        for (auto * tile : selectedTiles) {
            if (tile->isVisible()) {
                if (scrollY > 0)
                    tile->changeOrientationUp();

                if (scrollY < 0)
                    tile->changeOrientationDown();
            }
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
    freezeFlood = true;
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

float ofApp::getFocusAlpha(FloatSeconds period)
{
    auto diff = Clock::now() - focus_start;
    auto elapsed_seconds = duration_cast<FloatSeconds>(diff);
    return -cosf(float(M_PI) * elapsed_seconds.count() / period.count()) / 2.f + .5f;
}

ofColor ofApp::getFocusColorMix(ofColor start, ofColor end, FloatSeconds period)
{
    const auto alpha = getFocusAlpha(period);
    return start * (1 - alpha) + end * alpha;
}

ofColor ofApp::getFocusColor(int gray, float alpha)
{
    auto a = (unsigned char) (128 * getFocusAlpha(1s));

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
        if (not enableFlood or not freezeFlood)
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

inline static void of_rotate_degrees(float degrees)
{
#if OF_VERSION_MAJOR > 0 || OF_VERSION_MAJOR == 0 && OF_VERSION_MINOR >= 10
    ofRotateDeg(degrees);
#else
    ofRotate(degrees);
#endif
}

void ofApp::Sticky::draw()
{
    static const float SQRT_3_PER_2 = std::sin(M_PI/3);

    ofPushMatrix();
    ofTranslate(pos.x, pos.y);
    if (direction >= 0) {
        of_rotate_degrees(90 + 60 * direction);
        if (flip)
            ofScale(-1, -1);
        ofScale(SQRT_3_PER_2, SQRT_3_PER_2);
    }
    const auto &image = images[stepIndex];
    const float w = image.getWidth() * PIX_PER_MM;
    const float h = image.getHeight() * PIX_PER_MM;
    image.draw( -w / 2, h * .125 - h, w, h);
    ofPopMatrix();
}

std::complex<float> ofApp::Sticky::getDirectionVector() const
{
    return direction < 0 ? complex<float>(0) :
        exp(complex<float>(0, M_PI / 2 + 2 * M_PI * direction / 6)) * (flip ? 1.f : -1.f);
}

static void drawVector(const ofVec2f &pos, complex<float> direction, float length, float arrowhead)
{
    static const float SQRT_3_PER_2 = std::sin(M_PI / 3);
    static const auto u150deg = complex<float>(0, 2 * M_PI / 3 + M_PI / 6);
    static const auto rotateP150 = exp(u150deg);
    static const auto rotateM150 = exp(-u150deg);

    auto lvector = (length - arrowhead * SQRT_3_PER_2) * direction;
    auto hvector = length *  direction;
    const auto start = ofVec2f(pos.x, pos.y);
    const auto end = start + toVec2f(lvector);

    const auto tript0 = start + toVec2f(hvector);
    const auto tript1 = tript0 + toVec2f(arrowhead  * (direction * rotateP150));
    const auto tript2 = tript0 + toVec2f(arrowhead  * (direction * rotateM150));

    ofDrawLine(start, end);
    ofPushStyle();
    ofFill();
    ofBeginShape();
    ofVec3f triangle[] = {toVec3f(tript0), toVec3f(tript1) , toVec3f(tript2)};
    for (auto &vertex : triangle)
        ofVertex(vertex);

    ofEndShape();
    ofPopStyle();
}

void ofApp::Sticky::drawArrow(float length, const float arrowhead)
{
    if (direction < 0)
        return;

    drawVector(pos, getDirectionVector(), length, arrowhead);
}

void ofApp::Sticky::drawNormal(float length, const float arrowhead)
{
    if (direction < 0)
        return;

    static constexpr complex<float> rot90 {0, 1};
    drawVector(pos, getDirectionVector() * rot90, length, arrowhead);
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

void ofApp::Sticky::updateStep(const TimeStamp& now)
{
    if ((now - lastStep) < STEP_DURATION)
        return;
    lastStep = now;
    ++stepIndex;
    if (stepIndex >= (int) images.size())
        stepIndex = 0;
}

