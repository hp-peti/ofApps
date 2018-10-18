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


static const auto shift = []() { return ofGetKeyPressed(OF_KEY_SHIFT); };
static const auto ctrl_or_alt = []() { return    ofGetKeyPressed(OF_KEY_CONTROL)
                                              or ofGetKeyPressed(OF_KEY_ALT)
                                              or ofGetKeyPressed(OF_KEY_LEFT_ALT)
                                              or ofGetKeyPressed(OF_KEY_RIGHT_ALT)
                                              or ofGetKeyPressed(OF_KEY_COMMAND); };

static const float step_multiplier() { return shift() ? 9 : 1; };


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

    tv.initView(ViewCoords{zoom_levels[zoomLevel = default_zoom_level()],
                           ofVec2f{0,0}},
                getViewSize());

    tv.createTiles();
    resizeFrameBuffer(ofGetWidth(), ofGetHeight());

    tv.currentTile = nullptr;
    tv.resetFocusStartTime = [this]{focus_start = Clock::now();};
}




//--------------------------------------------------------------
void ofApp::update()
{
    const auto now = Clock::now();

    if (tv.viewTrans.isActive()) {
        if (tv.viewTrans.update(now)) {
            auto blend = sin(M_PI * tv.viewTrans.getValue() / 2);
            tv.view = ViewCoords::blend(tv.prevView, tv.nextView, blend);
            findCurrentTile();
        } else {
            tv.prevView = tv.view = tv.nextView;
            tv.removeExtraTiles(tv.view);
            findCurrentTile();
        }
        redrawFramebuffer = true;
    }

    if (sticky.visible)
    {
        updateSticky();
        sticky.updateStep(now);
    }

    tv.updateSelected();
}

void ofApp::drawBackground()
{
    if (concrete.isAllocated())
    {
        const auto &viewSize = tv.viewSize;
        const auto &view = tv.view;
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
    ofSetLineWidth(LINE_WIDTH_PIX * tv.view.zoom);
    ofPushMatrix();
    ofTranslate(LINE_WIDTH_PIX / 2, LINE_WIDTH_PIX / 2);
    for (auto* tile : tv.viewableTiles)
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
        ofSetLineWidth(2 * tv.view.zoom);
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

    const auto &view = tv.view;
    const auto &tiles = tv.tiles;
    const auto &viewSize = tv.viewSize;

    auto viewrect_mm = view.getViewRect(viewSize);
    viewrect_mm.x /= PIX_PER_MM;
    viewrect_mm.y /= PIX_PER_MM;
    viewrect_mm.width /= PIX_PER_MM;
    viewrect_mm.height /= PIX_PER_MM;

    std::ostringstream info;
    info
        << "Scale      : " << "1px = " << 1 / (PIX_PER_MM * view.zoom) << "mm\n"
        << "View       : " << (int)viewrect_mm.width << "mm x " << (int)viewrect_mm.height << "mm"
                           << " @ " << (int)viewrect_mm.x << "mm, " << (int)viewrect_mm.y << "mm\n"
        << "Tiles      : " << tiles.size() << "\n"
        << "Frame rate : " << std::fixed << std::setprecision(2) << ofGetFrameRate() << " fps";
        ;
    const ofVec2f pos(2, ofGetViewportHeight() - 2);
    drawBottomText(info.str(), pos);
}

void ofApp::drawFocus()
{
    auto shift = ::shift();

    if (!tv.enableFlood) {
        drawTileFocus(tv.currentTile, shift);
    } else {
        for (auto *tile : tv.selectedTiles) {
            drawTileFocus(tile, shift);
        }
    }
}

void ofApp::drawTileFocus(Tile * tile, bool shift)
{
    if (tile == nullptr)
        return;

    const auto getFocusGray = [shift](const TileColor color) -> unsigned char {
        switch (color) {
        case TileColor::Gray:
            return ! shift ? 240 : 64;
        case TileColor::Black:
            return ! shift ? 128 : 240;
        case TileColor::White:
            return ! shift ? 64 : 128;
        default:
            return 128;
        }
    };

    if (tile->enabled or tile->in_transition) {
        ofSetColor(getFocusColor(getFocusGray(tile->color), tile->alpha));
        tile->fill();
    }
    if (not tile->enabled or tile->in_transition) {
        ofSetColor(getFocusColor(!shift ? 255 : 0 , 1 - tile->alpha));
        ofSetLineWidth(1.5 * tv.view.zoom);
        tile->draw();
    }
}

void ofApp::resizeFrameBuffer(int w, int h)
{
    frameBuffer.clear();
    frameBuffer.allocate(w, h, GL_RGBA);
    redrawFramebuffer = true;
}

// b == a * ( 1 - alpha ) + x * alpha
// c == b * ( 1 - alpha ) + x * alpha
//   == (a * ( 1 - alpha ) + x * alpha) * (1 - alpha) + x * alpha
//   == a * (1 - (2*alpha - alpha^2) ) + x * (2*alpha - alpha^2)

template <typename T>
inline T doubleAlpha(const T &alpha) {
    return 2 * alpha - alpha * alpha;
}

void ofApp::drawToFramebuffer()
{
    ofPushStyle();
    frameBuffer.begin();

    drawBackground();

    ofEnableSmoothing();
    ofEnableAntiAliasing();
    ofEnableAlphaBlending();

    const auto &view = tv.view;

    ofPushMatrix();
    view.applyToCurrentMatrix();

    drawShadows();

    const auto &viewableTiles = tv.viewableTiles;

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

    ofPopMatrix();
    frameBuffer.end();
    ofPopStyle();
}

void ofApp::draw()
{
    auto now = Clock::now();
    bool rfb = false;
    for (auto * tile : tv.viewableTiles)
        rfb |= tile->update_alpha(now);
    redrawFramebuffer |= rfb;

    if (redrawFramebuffer) {
        drawToFramebuffer();
        redrawFramebuffer = false;
    }

    ofPushStyle();
    ofDisableAlphaBlending();
    ofDisableDepthTest();
    ofSetColor(255);
    frameBuffer.draw(0, 0, ofGetWidth(), ofGetHeight());
    ofPopStyle();

    ofPushMatrix();
    tv.view.applyToCurrentMatrix();
    drawFocus();
    drawSticky();
    ofPopMatrix();

    drawInfo();

}

constexpr int KEY_CTRL_(const char ch)
{
    return ch >= 'A' && ch <= 'Z' ? ch - 'A' + 1 : ch;
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key)
{
    auto &selectedTiles = tv.selectedTiles;
    auto &freezeSelection = tv.freezeSelection;
    auto &enableFlood = tv.enableFlood;
    auto &tiles = tv.tiles;
    const auto &view = tv.view;

    auto now = Clock::now();
    
    switch (key) {
    case 'i':
    case 'I':
        for (auto *tile : selectedTiles) {
            if (tile->isVisible()) {
                tile->invertColor();
                freezeSelection = true;
                redrawFramebuffer = true;
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
        redrawFramebuffer = true;
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
        redrawFramebuffer = true;
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
        redrawFramebuffer = true;
        break;
    case 'c':
    case 'C':
        for (auto *tile : selectedTiles)
            if (tile->isVisible())
                tile->orientation = Orientation::Blank;
        freezeSelection = true;
        redrawFramebuffer = true;
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
        redrawFramebuffer = true;
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
            redrawFramebuffer = true;
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
            redrawFramebuffer = true;
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
            redrawFramebuffer = true;
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
        tv.startMoving(now, VIEW_TRANS_DURATION, -view.offset.x, -view.offset.y);
        break;
    case OF_KEY_LEFT:
        tv.startMoving(now, VIEW_TRANS_DURATION, -TileParams::X_STEP * step_multiplier(), 0);
        break;
    case OF_KEY_RIGHT:
        tv.startMoving(now, VIEW_TRANS_DURATION, +TileParams::X_STEP * step_multiplier(), 0);
        break;
    case OF_KEY_UP:
        tv.startMoving(now, VIEW_TRANS_DURATION, 0, -TileParams::Y_STEP * step_multiplier());
        break;
    case OF_KEY_DOWN:
        tv.startMoving(now, VIEW_TRANS_DURATION, 0, +TileParams::Y_STEP * step_multiplier());
        break;
    case '+':
        if (zoomLevel + 1 < (int)zoom_levels.size())
            tv.startZooming(now, VIEW_TRANS_DURATION, zoom_levels[++zoomLevel]);
        break;
    case '-':
        if (zoomLevel > 1)
            tv.startZooming(now, VIEW_TRANS_DURATION, zoom_levels[--zoomLevel]);
        break;
    case '*':
        if (zoomLevel != default_zoom_level())
            tv.startZooming(now, VIEW_TRANS_DURATION, zoom_levels[zoomLevel = default_zoom_level()]);
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
            tv.enableFlood = false;
        }
        break;
    }
}

void ofApp::updateSticky(int x, int y)
{
    sticky.pos = ofVec2f { (float) (x), (float) (y) } / tv.view.zoom + tv.view.offset;
    if (sticky.visible or sticky.show_arrow) {
        if (tv.currentTile != nullptr) {
            sticky.adjustDirection(*tv.currentTile);
        }
    }
}


//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y)
{
    tv.findCurrentTile(x, y);
    updateSticky(x, y);
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button)
{
    auto &currentTile = tv.currentTile;
    const auto &selectedTiles = tv.selectedTiles;
    const auto &enableFlood = tv.enableFlood;
    auto &freezeSelection = tv.freezeSelection;

    auto prevTile = currentTile;
    tv.findCurrentTile(x, y);

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
                redrawFramebuffer = true;
            }
            break;
        case OF_MOUSE_BUTTON_RIGHT:
            if (   currentTile != nullptr 
                and currentTile != prevTile
                and currentTile->enabled
            ) {
                currentTile->start_disabling(Clock::now());
                redrawFramebuffer = true;
            }
            break;
        }
    } else {
        switch (button) {
        case OF_MOUSE_BUTTON_RIGHT:
            if (currentTile != nullptr
                and currentTile->enabled
                ) {
                const auto now = Clock::now();
                for (auto *tile : selectedTiles) {
                    tile->start_disabling(now);
                }
                freezeSelection = false;
                redrawFramebuffer = true;
            }
            break;
        }
    }
    updateSticky(x, y);
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button)
{
    tv.findCurrentTile(x, y);
    tv.updateSelected();

    auto &selectedTiles = tv.selectedTiles;
    auto &freezeSelection = tv.freezeSelection;

    auto now = Clock::now();
    switch (button) {
        case OF_MOUSE_BUTTON_LEFT:
            if (not shift())
                for (auto *tile : selectedTiles)
                    tile->changeColorUp(now);
            else
                for (auto *tile : selectedTiles)
                    tile->changeColorDown(now);
            redrawFramebuffer = true;
            freezeSelection = true;
            tv.resetFocusStartTime();
            break;
        case OF_MOUSE_BUTTON_RIGHT:
            for (auto *tile : selectedTiles) {
                if (tile->enabled) {
                    tile->start_disabling(now);
                }
            }
            tv.resetFocusStartTime();
            freezeSelection = true;
            break;
        case OF_MOUSE_BUTTON_MIDDLE:
            for (auto *tile : selectedTiles)
                redrawFramebuffer |= tile->removeOrientation();
            freezeSelection = true;
            tv.resetFocusStartTime();
            break;
        }
    updateSticky(x, y);
}

void ofApp::mouseScrolled(int x, int y, float scrollX, float scrollY)
{
    tv.findCurrentTile(x, y);
    tv.updateSelected();

    for (auto * tile : tv.selectedTiles) {
        if (tile->isVisible()) {
            if (scrollY > 0)
                tile->changeOrientationUp();

            if (scrollY < 0)
                tile->changeOrientationDown();

            redrawFramebuffer = true;
        }
    }
    tv.freezeSelection = true;
    updateSticky(x, y);
}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button)
{

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y)
{
    tv.findCurrentTile(x,y);
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y)
{
    tv.currentTile = nullptr;
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h)
{
    resizeFrameBuffer(w, h);
#ifdef _DEBUG
    clog << "window resized: w = " << w << "; h = " << h << endl;
#endif
    tv.resizeView(getViewSize());
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg)
{

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo)
{

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



