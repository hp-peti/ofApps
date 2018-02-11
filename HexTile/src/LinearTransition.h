/*
 * LinearTransition.h
 *
 *  Created on: 11 Feb 2018
 *      Author: hpp
 */

#ifndef SRC_LINEARTRANSITION_H_
#define SRC_LINEARTRANSITION_H_

#include "Clock.h"

class LinearTransition
{
public:
    bool isActive() const
    {
        return is_active;
    }

    LinearTransition& stop()
    {
        is_active = false;
        return *this;
    }

    void start(const TimeStamp &now, Duration duration)
    {
        begin_time = now;
        end_time = now + duration;
        if (is_active) {
            begin_value = current_value;
        } else {
            is_active = true;
            current_value = begin_value = start_value;
        }
    }

    bool update(const TimeStamp &now) {
        if (!is_active)
            return false;
        if (now >= end_time) {
            current_value = end_value;
            is_active = false;
            return false;
        }
        const float alpha = duration_cast<FloatSeconds>(now - begin_time).count()
                          / duration_cast<FloatSeconds>(end_time - begin_time).count();

        current_value = begin_value * (1 - alpha) + end_value * alpha;
        return true;

    }

    float getValue() const {
        return current_value;
    }

    LinearTransition &setStartValue(float value) {
        start_value = value;
        if (!is_active)
            current_value = value;
        return *this;
    }
    LinearTransition &setEndValue(float value) {
        end_value = value;
        return *this;
    }

private:
    bool is_active = false;
    TimeStamp begin_time;
    TimeStamp end_time;

    float start_value = 0;
    float begin_value = 0;
    float current_value = 0;
    float end_value = 1;
};



#endif /* SRC_LINEARTRANSITION_H_ */
