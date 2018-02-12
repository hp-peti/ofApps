/*
 * Clock.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_CLOCK_H_
#define SRC_CLOCK_H_

#include <chrono>

using Clock = std::chrono::steady_clock;
using TimeStamp = Clock::time_point;
using Duration = Clock::duration;
using FloatSeconds = std::chrono::duration<float>;

using namespace std::chrono_literals;
using std::chrono::duration_cast;

#endif /* SRC_CLOCK_H_ */
