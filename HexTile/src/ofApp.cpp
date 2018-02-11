#include "ofApp.h"
#include "TileParams.h"
#include "ZoomLevels.h"

#include <ofFileUtils.h>

#include <array>
#include <algorithm>

#include <sstream>
#include <iomanip>

#include <set>
#include <deque>

#include <cmath>
#include <ciso646>

using std::complex;

static constexpr auto ARROW_COLOR_PERIOD = 2s;
static constexpr auto ARROW_SHORT_LENGTH_PERIOD = 0.75s;
static constexpr auto ARROW_LONG_LENGTH_PERIOD = 1.5s;

constexpr auto VIEW_TRANS_DURATION = 125ms;

static constexpr float X_STEP = TILE_RADIUS_PIX / 2;
static constexpr float Y_STEP = SQRT_3 * TILE_RADIUS_PIX / 4;

static
const auto zoom_levels = ZoomLevels::generate(6);

const int ofApp::default_zoom_level()
{
    static const int index = std::find(
        zoom_levels.begin(), zoom_levels.end(),
        ZoomLevels::Ratio { 1,1 }) - zoom_levels.begin();
    return index;
}

static ofVec2f getViewSize()
{
    return ofVec2f(ofGetWindowWidth(), ofGetWindowHeight());
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

    view.zoom = zoom_levels[zoomLevel = default_zoom_level()];
    view.offset = ofVec2f { 0,0 };
    viewSize = getViewSize();
    prevView = view;
    nextView = view;

    createTiles();

    currentTile = nullptr;
}


void ofApp::createTiles()
{
    auto range = TileParams::tile_range(viewSize, view.zoom, view.offset);

    for (int row = range.rows.begin; row <= range.rows.end; ++row) {
        for (int col = range.cols.begin; col <= range.cols.end; col++) {
            auto center = TileParams::center(row, col);
            tiles.emplace_back(center.x, center.y, TileParams::radius);
        }
    }

    viewableTiles.reserve(tiles.size());

    for (auto tile = tiles.begin(); tile != tiles.end(); ++tile) {
        for (auto other_tile = tiles.begin(); other_tile != tile; ++other_tile)
            tile->connectIfNeighbour(&*other_tile);
        viewableTiles.push_back(&*tile);
    }

}

void ofApp::createMissingTiles(const ViewCoords &view)
{
    viewableTiles.clear();

    auto findTile = [this](const ofVec2f &center) {
        auto found = std::find_if(tiles.begin(), tiles.end(), [&center](const Tile &tile) {
            return tile.squareDistanceFromCenter(center) < tile.radiusSquared();
        });
        return found != tiles.end() ? &*found : nullptr;
    };

    auto range = TileParams::tile_range(viewSize, view.zoom, view.offset);

    for (int row = range.rows.begin; row <= range.rows.end; ++row) {
        for (int col = range.cols.begin; col <= range.cols.end; col++) {
            auto center = TileParams::center(row, col);
            auto existingTile = findTile(center);
            if (existingTile != nullptr) {
                viewableTiles.push_back(existingTile);
                continue;
            }
            tiles.emplace_back(center.x, center.y, TileParams::radius);
            auto last = std::prev(tiles.end());
            viewableTiles.push_back(&*last);
            for (auto tile = tiles.begin(); tile != last; ++tile)
                tile->connectIfNeighbour(&*last);
        }
    }
}

void ofApp::removeExtraTiles(const ViewCoords &view)
{
    auto windowRect = view.getViewRect(viewSize);

    std::vector<Tile *> removedTiles;

    auto tile = tiles.begin();
    while (tile != tiles.end()) {
        if (not tile->isVisible()) {
            if (not tile->isInRect(windowRect)) {
                tile->disconnect();
                removedTiles.push_back(&*tile);
                tile = tiles.erase(tile);
                continue;
            }
        }
        ++tile;
    }

    if (not removedTiles.empty()) {
        std::sort(removedTiles.begin(), removedTiles.end());
        if (currentTile != nullptr) {
            if (std::binary_search(removedTiles.begin(), removedTiles.end(), currentTile)) {
                if (previousTile == currentTile)
                    previousTile = nullptr;
                currentTile = nullptr;
            }
        }
        if (previousTile != nullptr and std::binary_search(removedTiles.begin(), removedTiles.end(), previousTile)) {
            currentTile = nullptr;
        }
        viewableTiles.erase(std::remove_if(viewableTiles.begin(), viewableTiles.end(), [&removedTiles](Tile *tile) { 
            return std::binary_search(removedTiles.begin(), removedTiles.end(), tile); }), viewableTiles.end());

        viewableTiles.shrink_to_fit();
    }
}

//--------------------------------------------------------------
void ofApp::update()
{
    const auto now = Clock::now();

    if (viewTrans.isActive()) {
        if (viewTrans.update(now)) {
            auto blend = sin(M_PI * viewTrans.getValue() / 2);
            view = ViewCoords::blend(prevView, nextView, blend);
        } else {
            prevView = view = nextView;
            removeExtraTiles(view);
            findCurrentTile();
        }
    }

    if (sticky.visible)
    {
        updateSticky();
        sticky.updateStep(now);
    }

    updateSelected();
}

void ofApp::drawBackground()
{
    if (concrete.isAllocated())
    {
        ofSetColor(240);
        ofRectangle winrect(0, 0, viewSize.x, viewSize.y);
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
    for (auto* tile : viewableTiles)
    {
        if (tile->isVisible())
        {
            ofSetColor(ofColor(0, 0, 0, 128 * tile->alpha));
            tile->draw();
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

    auto viewrect_mm = view.getViewRect(viewSize);
    viewrect_mm.scale(1 / PIX_PER_MM, 1 / PIX_PER_MM);

    std::ostringstream info;
    info
        << "Scale      : " << "1 px = " << 1 / (PIX_PER_MM * view.zoom) << " mm\n"
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

    for (auto * tile : viewableTiles)
       tile->update_alpha(now);

    ofPushMatrix();
    ofScale(view.zoom, view.zoom);
    ofTranslate(-view.offset.x, -view.offset.y);

    drawShadows();

    for (auto * tile : viewableTiles) {
        if (tile->isVisible()) {
            tile->fill(tileImages);
        }
    }

    ofSetLineWidth(LINE_WIDTH_PIX * view.zoom);
    for (auto * tile : viewableTiles) {
        if (tile->isVisible()) {
            const float lineAlpha = tile->alpha * 160 / 255;
            ofSetColor(20, 20, 20, 255 * lineAlpha);
            tile->draw();

            // as if drawn 2 times
            ofSetColor(20, 20, 20, 255 * doubleAlpha(lineAlpha));
            tile->drawCubeIllusion();
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

static const float step_multiplier() { return shift() ? 9 : 1; };

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
    case OF_KEY_HOME:
        startMoving(now, -view.offset.x, -view.offset.y);
        break;
    case OF_KEY_LEFT:
        startMoving(now, -X_STEP * step_multiplier(), 0);
        break;
    case OF_KEY_RIGHT:
        startMoving(now, +X_STEP * step_multiplier(), 0);
        break;
    case OF_KEY_UP:
        startMoving(now, 0, -Y_STEP * step_multiplier());
        break;
    case OF_KEY_DOWN:
        startMoving(now, 0, +Y_STEP * step_multiplier());
        break;
    case '+':
        if (zoomLevel + 1 < (int)zoom_levels.size())
            startZooming(now, zoom_levels[++zoomLevel]);
        break;
    case '-':
        if (zoomLevel > 1)
            startZooming(now, zoom_levels[--zoomLevel]);
        break;
    case '*':
        if (zoomLevel != default_zoom_level())
            startZooming(now, zoom_levels[zoomLevel = default_zoom_level()]);
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
    case OF_KEY_LEFT_ALT:
    case OF_KEY_RIGHT_ALT:
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
    auto prevTile = currentTile;
    findCurrentTile(x, y);

    if (not enableFlood) {
        switch (button) {
        case OF_MOUSE_BUTTON_LEFT:
            if (   prevTile != nullptr
               and prevTile != currentTile
               and currentTile != nullptr
               and prevTile->enabled
               ) {
                currentTile->color = prevTile->color;
                if (not currentTile->enabled) {
                    currentTile->orientation = prevTile->orientation;
                    currentTile->start_enabling(Clock::now());
                }
            }
            break;
        case OF_MOUSE_BUTTON_RIGHT:
            if (   currentTile != nullptr 
                and currentTile != prevTile
                and currentTile->enabled
            ) {
                currentTile->start_disabling(Clock::now());
            }
            break;
        }
    }

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
    viewSize = getViewSize();
    createMissingTiles(view);
    if (!viewTrans.isActive())
        removeExtraTiles(view);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{

}

Tile* ofApp::findTile(float x, float y)
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
        if (enableFlood and freezeSelection) {
            if (std::find(selectedTiles.begin(), selectedTiles.end(), currentTile) == selectedTiles.end()) {
                freezeSelection = false;
            }
        }
        if (not enableFlood or not freezeSelection)
            resetFocusStartTime();
    }
    previousTile = currentTile;

}

void ofApp::startMoving(const TimeStamp& now, float xoffset, float yoffset)
{
    if (!viewTrans.isActive())
        nextView = view;

    prevView = view;
    nextView.offset.x += xoffset;
    nextView.offset.y += yoffset;

    createMissingTiles(nextView);
    viewTrans.stop().start(now, VIEW_TRANS_DURATION);
}

void ofApp::startZooming(const TimeStamp &now, float newZoom)
{
    if (!viewTrans.isActive())
        nextView = view;

    nextView = view;
    prevView = view;
    nextView.setZoomWithOffset(newZoom, ofVec2f(ofGetMouseX(), ofGetMouseY()));
    createMissingTiles(nextView);
    viewTrans.stop().start(now, VIEW_TRANS_DURATION);
}
