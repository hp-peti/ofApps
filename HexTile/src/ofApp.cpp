#include "ofApp.h"

#include <ofFileUtils.h>

#include <array>
#include <algorithm>

#include <sstream>
#include <iomanip>

#include <set>
#include <deque>

#include <cmath>
#include <ciso646>

#ifdef _DEBUG
#include <iostream>
using std::clog;
using std::endl;
#endif

using std::array;
using std::complex;
using std::chrono::duration_cast;
using namespace std::chrono_literals;

#define SQRT_3    1.7320508075688772935274463415059

static constexpr float TILE_EDGE_MM = 82.f; // mm
static constexpr float TILE_SEPARATION_MM = 3.5; // mm

static constexpr float PIX_PER_MM = .5f;
static constexpr float BG_SCALE = .5f;

static constexpr float TILE_RADIUS_PIX = (TILE_EDGE_MM + TILE_SEPARATION_MM / 2) * PIX_PER_MM;
static constexpr float LINE_WIDTH_PIX = (TILE_SEPARATION_MM) * PIX_PER_MM;

static constexpr auto ENABLE_DURATION = 250ms;
static constexpr auto DISABLE_DURATION = 750ms;
static constexpr auto STEP_DURATION = 200ms;
static constexpr auto ARROW_COLOR_PERIOD = 2s;
static constexpr auto ARROW_SHORT_LENGTH_PERIOD = 0.75s;
static constexpr auto ARROW_LONG_LENGTH_PERIOD = 1.5s;

namespace ZoomLevels {

struct rational
{
    int num;
    unsigned den;

    bool operator == (const rational &other) const
    {
        return num * (int)other.den == other.num * (int)den;
    }

    bool operator < (const rational &other) const
    {
        return num * (int)other.den < other.num * (int)den;
    }

    bool operator != (const rational &other) const
    {
        return !(*this == other);
    }

    bool operator > (const rational &other) const
    {
        return other < *this;
    }

    operator float() const { return (float)num / (float)den; }

};

const std::vector<rational> generate(unsigned N)
{
    std::vector<rational> v;

    for (int num = 1; num <= (int)N; ++num) {
        for (unsigned den = 1; den <= N; ++den) {
            rational r { num, den };
            auto p = std::lower_bound(v.begin(), v.end(), r);
            if (p == v.end() || *p != r) {
                v.insert(p, r);
            }
        }
    }
#ifdef _DEBUG
    clog << "generated ratios: ";
    for (auto &r : v) {
        clog << " " << r.num << "/" << r.den;
    }
    clog << endl;
#endif
    return v;
}

} // namespace ZoomLevels

static constexpr float X_STEP = TILE_RADIUS_PIX;
static constexpr float Y_STEP = TILE_RADIUS_PIX;

const auto zoom_levels = ZoomLevels::generate(6);

const int ofApp::default_zoom_level()
{
    return std::find(zoom_levels.begin(), zoom_levels.end(), ZoomLevels::rational { 1,1 }) - zoom_levels.begin();
}

static ofVec2f getViewportSize(bool fullscreen)
{
    return ofVec2f(ofGetWindowWidth(), ofGetWindowHeight());
}

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

namespace TileParams {

static constexpr float sin_60_deg = SQRT_3 / 2;
static constexpr float cos_60_deg = 0.5;

static constexpr float radius = TILE_RADIUS_PIX;
static constexpr float row_height = radius *  sin_60_deg;
static constexpr float col_width = 3 * radius;
static constexpr float col_offset[2] = { radius, 2 * radius + radius * cos_60_deg};
static constexpr float row_offset = float(row_height / 2);

inline ofVec2f center(int row, int col)
{
    return ofVec2f(col_width * col + col_offset[row & 1],
                   row_height * row + row_offset);
}

struct IntRange
{
    int begin;
    int end;
};

float rowf(float y)
{
    return ((y - row_offset ) / row_height);
}

float colf(float x)
{
    return ((x - col_offset[0]) / col_width);
}

IntRange row_range(float begin_y, float end_y)
{
    return IntRange { (int)std::floor(rowf(begin_y) - .5), (int)std::ceil(rowf(end_y) + .5) };
}
IntRange col_range(float begin_x, float end_x)
{
    return IntRange { (int) std::floor(colf(begin_x) - .5), (int) std::ceil(colf(end_x) + .5) };
}

struct TileRange
{
    IntRange rows;
    IntRange cols;
};

TileRange tile_range(const ofVec2f &size, float zoom = 1, const ofVec2f &offset = ofVec2f{0,0})
{
    return TileRange {
        row_range(offset.y, size.y / zoom + offset.y),
        col_range(offset.x, size.x / zoom + offset.x)
    };
}

} // namespace TCP

void ofApp::createTiles()
{
    view.zoom = zoom_levels[zoomLevel];
    view.offset = ofVec2f { 0,0 };
    view.size = getViewportSize(fullScreen);

    auto range = TileParams::tile_range(view.size, view.zoom, view.offset);

    for (int row = range.rows.begin; row <= range.rows.end; ++row) {
        for (int col = range.cols.begin; col <= range.cols.end; col++) {
            auto center = TileParams::center(row, col);
            tiles.emplace_back(center.x, center.y, TileParams::radius);
        }
    }

    for (auto tile = tiles.begin(); tile != tiles.end(); ++tile) {
        for (auto other_tile = tiles.begin(); other_tile != tile; ++other_tile)
            tile->connectIfNeighbour(&*other_tile);
    }
}

void ofApp::createMissingTiles()
{
    if (view == prevView)
        return;

    prevView = view;

    auto hasTile = [this](const ofVec2f center) {
        return std::find_if(tiles.begin(), tiles.end(), [&center](const Tile &tile) {
            return tile.squareDistanceFromCenter(center) < tile.radiusSquared();
        }) != tiles.end();
    };

    auto range = TileParams::tile_range(view.size, view.zoom, view.offset);

    for (int row = range.rows.begin; row <= range.rows.end; ++row) {
        for (int col = range.cols.begin; col <= range.cols.end; col++) {
            auto center = TileParams::center(row, col);
            if (hasTile(center))
                continue;
            tiles.emplace_back(center.x, center.y, TileParams::radius);
            auto last = std::prev(tiles.end());
            for (auto tile = tiles.begin(); tile != last; ++tile)
                last->connectIfNeighbour(&*tile);
        }
    }
    removeExtraTiles();
}

void ofApp::removeExtraTiles()
{
    auto windowRect = view.getViewRect();


    auto tile = tiles.begin();
    while (tile != tiles.end()) {
        if (not tile->isVisible()) {
            if (not tile->isInRect(windowRect)) {
                tile->disconnect();
                tile = tiles.erase(tile);
                continue;
            }
        }
        ++tile;
    }



}

//--------------------------------------------------------------
void ofApp::update()
{
    createMissingTiles();

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
    // clog << "connect " << this << " to " << other << endl;
#endif
    neighbours.push_back(other);
    other->neighbours.push_back(this);
}

void ofApp::Tile::disconnect()
{
    for (auto *n : neighbours) {
        auto &nn = n->neighbours;
        nn.erase(std::find(nn.begin(), nn.end(), this));
    }
    neighbours.clear();
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
        ofRectangle winrect(0, 0, view.size.x, view.size.y);
        ofRectangle slab { 0, 0, concrete.getWidth(), concrete.getHeight()};
        slab.scale(BG_SCALE * view.zoom, BG_SCALE * view.zoom);
        float offsetx = std::fmod(-view.offset.x * view.zoom, slab.width);
        if (offsetx > 0)
            offsetx -= slab.width;

        float offsety = std::fmod(-view.offset.y * view.zoom, slab.width);
        if (offsety > 0)
            offsety -= slab.height;

        for (slab.x = offsetx; slab.x < winrect.width; slab.x += slab.width)
            for (slab.y = offsety; slab.y < winrect.width; slab.y += slab.height)
                concrete.draw(slab);
    }
    else
    {
        ofBackgroundGradient(ofColor { 120, 120, 120 }, ofColor { 160, 160, 160 });
    }
}

void ofApp::drawShadows()
{
    ofSetLineWidth(LINE_WIDTH_PIX * view.zoom);
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
        ofSetLineWidth(2 * view.zoom);
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

static void drawBottomText(const std::string &text, ofVec2f pos)
{
    pos.y -= std::count_if(text.begin(), text.end(), [](char c) { return c == '\n'; }) * 13.5f;

    ofSetColor(0, 200);
    ofDrawBitmapString(text, pos.x + 1, pos.y + 1);
    ofSetColor(255);
    ofDrawBitmapString(text, pos.x, pos.y);
}

void ofApp::drawInfo()
{
    if (not showInfo)
        return;

    auto viewrect_mm = view.getViewRect();
    viewrect_mm.scale(1 / PIX_PER_MM, 1 / PIX_PER_MM);

    std::ostringstream info;
    info
        << "Scale      : " << "1 px = " << view.zoom / PIX_PER_MM << " mm\n"
        << "View       : " << (int)viewrect_mm.width << "x" << (int)viewrect_mm.height
                           << " @ " << (int)viewrect_mm.x << "," << (int)viewrect_mm.y << " mm\n"
        << "Tiles      : " << tiles.size() << "\n"
        << "Frame rate : " << std::setprecision(2) << ofGetFrameRate() << " fps";
        ;
    const ofVec2f pos(2, ofGetViewportHeight() - 2);
    drawBottomText(info.str(), pos);
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
        ofSetLineWidth(1.5 * view.zoom);
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

    ofPushMatrix();
    ofScale(view.zoom, view.zoom);
    ofTranslate(-view.offset.x, -view.offset.y);

    drawShadows();

    for (auto & tile : tiles) {
        if (tile.isVisible()) {
            tile.fill(tileImages);
        }
    }

    ofSetLineWidth(LINE_WIDTH_PIX * view.zoom);
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

    ofPopMatrix();

    drawInfo();
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
        for (auto *tile : selectedTiles) {
            if (tile->isVisible()) {
                tile->invertColor();
                freezeSelection = true;
            }
        }
        break;
    case 'h':
    case 'H':
        showInfo = not showInfo;
        break;
    case 'W':
    case 'w':
        for (auto *tile : selectedTiles) {
            tile->color = TileColor::White;
            if (not tile->isVisible())
                tile->orientation = Orientation::Blank;
            tile->start_enabling(now);
        }
        freezeSelection = true;
        break;
    case 'B':
    case 'b':
        for (auto *tile : selectedTiles) {
            tile->color = TileColor::Black;
            if (not tile->isVisible())
                tile->orientation = Orientation::Blank;
            tile->start_enabling(now);
        }
        freezeSelection = true;
        break;
    case 'G':
    case 'g':
        for (auto *tile : selectedTiles) {
            tile->color = TileColor::Gray;
            if (not tile->isVisible())
                tile->orientation = Orientation::Blank;
            tile->start_enabling(now);
        }
        freezeSelection = true;
        break;
    case 'c':
    case 'C':
        for (auto *tile : selectedTiles)
            if (tile->isVisible())
                tile->orientation = Orientation::Blank;
        freezeSelection = true;
        break;
    case 'D':
    case 'd':
    case OF_KEY_DEL:
        if (not shift()) {
            for (auto *tile : selectedTiles) {
                tile->start_disabling(now);
            }
        } else {
            for (auto &tile : tiles) {
                tile.start_disabling(now);
            }
        }
        freezeSelection = true;
        break;
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
                for (auto *tile : selectedTiles)
                    tile->changeToRandomColor(now);
            } else {
                for (auto &tile : selectedTiles) {
                   tile->changeToRandomOrientation();
                }
            }
            freezeSelection = true;
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
                for (auto *tile : selectedTiles)
                    tile->changeToRandomOrientation();
            } else {
                for (auto *tile : selectedTiles)
                    tile->changeToRandomNonBlankOrientation();
            }
            freezeSelection = true;
            break;
        }
    case KEY_CTRL_('F'):
    case 'f':
    case 'F':
        fullScreen = !fullScreen;
        ofSetFullscreen(fullScreen);
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
    case OF_KEY_ALT:
    case OF_KEY_COMMAND:
        if (not enableFlood) {
            enableFlood = true;
            freezeSelection = false;
        }
        break;
    case OF_KEY_ESC:
        break;
    case OF_KEY_LEFT:
        view.offset.x -= X_STEP;
        break;
    case OF_KEY_RIGHT:
        view.offset.x += X_STEP;
        break;
    case OF_KEY_UP:
        view.offset.y -= Y_STEP;
        break;
    case OF_KEY_DOWN:
        view.offset.y += Y_STEP;
        break;
    case '+':
        if (zoomLevel + 1 < (int)zoom_levels.size())
            view.setZoomWithOffset(zoom_levels[++zoomLevel], ofVec2f(ofGetMouseX(), ofGetMouseY()));
        break;
    case '-':
        if (zoomLevel > 1)
            view.setZoomWithOffset(zoom_levels[--zoomLevel], ofVec2f(ofGetMouseX(), ofGetMouseY()));
        break;
    case '*':
        view.setZoomWithOffset(zoom_levels[zoomLevel = default_zoom_level()], ofVec2f(ofGetMouseX(), ofGetMouseY()));
        break;
    case ']':
        if (sticky.direction >= 0)
            ++sticky.direction %= 6;
        break;
    case '[':
        if (sticky.direction >= 0)
        (sticky.direction+= 5) %= 6;
        break;
    case 'Q':
    case 'q':
        if (ofGetKeyPressed(OF_KEY_ALT))
            ofExit(0);
        break;
    }

#if defined(_DEBUG)
    clog << std::hex << "key pressed: 0x" <<  key << "('" << (char)key << "')" << endl;
#endif
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key)
{
    switch (key) {
    case OF_KEY_CONTROL:
    case OF_KEY_ALT:
    case OF_KEY_COMMAND:
        if (!ctrl_or_alt()) {
            enableFlood = false;
        }
        break;
    }
}

void ofApp::updateSticky(int x, int y)
{
    sticky.pos = ofVec2f { (float) (x), (float) (y) } / view.zoom + view.offset;
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
        if (not freezeSelection)
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
            freezeSelection = true;
            resetFocusStartTime();
            break;
        case OF_MOUSE_BUTTON_RIGHT:
            for (auto *tile : selectedTiles) {
                if (tile->enabled) {
                    tile->start_disabling(now);
                }
            }
            resetFocusStartTime();
            freezeSelection = true;
            break;
        case OF_MOUSE_BUTTON_MIDDLE:
            for (auto *tile : selectedTiles)
                tile->removeOrientation();
            freezeSelection = true;
            resetFocusStartTime();
            break;
        }
    updateSticky(x, y);
}

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY)
{
    findCurrentTile(x, y);
    updateSelected();

    for (auto * tile : selectedTiles) {
        if (tile->isVisible()) {
            if (scrollY > 0)
                tile->changeOrientationUp();

            if (scrollY < 0)
                tile->changeOrientationDown();
        }
    }
    freezeSelection = true;
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
#ifdef _DEBUG
    clog << "window resized: w = " << w << "; h = " << h << endl;
#endif
    view.size = getViewportSize(fullScreen);
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
    x /= view.zoom;
    y /= view.zoom;
    x += view.offset.x;
    y += view.offset.y;

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
        if (not enableFlood or not freezeSelection)
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
    static constexpr float sin_60_deg = SQRT_3 / 2;

    ofPushMatrix();
    ofTranslate(pos.x, pos.y);
    if (direction >= 0) {
        of_rotate_degrees(90 + 60 * direction);
        if (flip)
            ofScale(-1, -1);
        ofScale(sin_60_deg, sin_60_deg);
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
    static constexpr float sin_60_deg = SQRT_3 / 2;
    static constexpr auto u150deg = complex<float>(0, 2 * M_PI / 3 + M_PI / 6);

    static const auto rotateP150 = exp(u150deg);
    static const auto rotateM150 = exp(-u150deg);

    auto lvector = (length - arrowhead * sin_60_deg) * direction;
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

void ofApp::ViewCoords::setZoomWithOffset(float newZoom, ofVec2f center)
{
    offset += center / zoom;
    offset -= center / newZoom;
    zoom = newZoom;
}
