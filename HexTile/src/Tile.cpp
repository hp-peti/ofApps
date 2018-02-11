/*
 * Tile.cpp
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#include "Tile.h"

#include "TileParams.h"

#include "ofGraphics.h"

#include <complex>
#include <array>

Tile::Tile(float x, float y, float radius) :
    center(x, y),
    radius(radius)
{
    static const auto roots = []()
    {
        std::array<std::complex<float>, 6> roots { };
        // e^(i*x) = cos(x) + i * sin(x)
        for (size_t i = 0; i < roots.size(); ++i)
            roots[i] = std::exp(std::complex<float>(0, i * M_PI / 3));

        return roots;
    }();

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

bool Tile::isPointInside(float x, float y) const
{
    return box.inside(x, y) and vertices.inside(x, y);
}

void Tile::connectIfNeighbour(Tile * other)
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

void Tile::disconnect()
{
    for (auto *n : neighbours) {
        auto &nn = n->neighbours;
        nn.erase(std::find(nn.begin(), nn.end(), this));
    }
    neighbours.clear();
}

void Tile::fill() const
{
    ofFill();
    ofBeginShape();
    for (auto& pt : vertices) {
        ofVertex(pt.x, pt.y);
    }
    ofEndShape();
    ofNoFill();
}

void Tile::fill(TileImages &images) const
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

void Tile::draw() const
{
    vertices.draw();
}

void Tile::drawCubeIllusion()
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

void Tile::update_alpha(const TimeStamp& now)
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

void Tile::start_enabling(const TimeStamp& now)
{
    if (enabled)
        return;
    enabled = true;
    in_transition = true;
    initial_alpha = alpha;
    alpha_start = now;
    alpha_stop = now + TILE_ENABLE_DURATION;
    update_alpha(now);
}

void Tile::start_disabling(const TimeStamp& now)
{
    if (not enabled)
        return;
    enabled = false;
    in_transition = true;
    initial_alpha = alpha;
    alpha_start = now;
    alpha_stop = now + TILE_DISABLE_DURATION;
    update_alpha(now);
}

