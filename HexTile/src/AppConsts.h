/*
 * AppConsts.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_APPCONSTS_H_
#define SRC_APPCONSTS_H_

constexpr float TILE_EDGE_MM = 82.f; // mm
constexpr float TILE_SEPARATION_MM = 3.5; // mm

constexpr float PIX_PER_MM = .5f;
constexpr float BG_SCALE = .5f;

static constexpr auto TILE_ENABLE_DURATION = 250ms;
static constexpr auto TILE_DISABLE_DURATION = 750ms;

constexpr float TILE_RADIUS_PIX = (TILE_EDGE_MM + TILE_SEPARATION_MM / 2) * PIX_PER_MM;
constexpr float LINE_WIDTH_PIX = (TILE_SEPARATION_MM) * PIX_PER_MM;


#endif /* SRC_APPCONSTS_H_ */
