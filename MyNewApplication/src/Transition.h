/*
 * Transition.h
 *
 *  Created on: 26 Mar 2016
 *      Author: hpp
 */

#ifndef SRC_TRANSITION_H_
#define SRC_TRANSITION_H_

#include <chrono>
#include <functional>

template <typename T>
using Generator = std::function<T()>;

struct TransitionBase {
    using Timestamp = std::chrono::system_clock::time_point;
    using TimeDiff = std::chrono::system_clock::duration;

    using Milliseconds = std::chrono::duration<float, std::milli>;

    static Timestamp clockNow() {
        return std::chrono::system_clock::now();
    }
};

template <class Scalable>
struct Transition: TransitionBase {
    Generator<Scalable> getNextValue;
    Generator<float> getNextInterval;

    template <typename ValueFun, typename IntervalFun>
    Transition(
        ValueFun valueGenerator,
        IntervalFun intervalGenerator,
        Timestamp now = TransitionBase::clockNow()
    ) :
        getNextValue { valueGenerator },
        getNextInterval { intervalGenerator } {

        using namespace std::chrono;
        beginValue = getNextValue();
        startTime = now;
        lastTime = startTime;
        next();
    }

    Transition(const Transition &) = default;

    void update(Timestamp now) {
        if (now == lastTime)
            return;
        lastTime = now;
        TimeDiff elapsed = now - startTime;
        if (elapsed > transitionLength) {
            currentValue = endValue;
            beginValue = endValue;
            startTime = lastTime;
            next();
        } else {
            float mix = elapsed / transitionLength;
            currentValue = beginValue * (1 - mix) + endValue * mix;
        }
    }

    Scalable get() const {
        return currentValue;
    }

private:
    void next() {
        transitionLength = Milliseconds(getNextInterval());
        endValue = getNextValue();
    }

    Scalable beginValue, endValue;
    Timestamp startTime;
    Timestamp lastTime;
    Milliseconds transitionLength;
    Scalable currentValue;

};

#endif /* SRC_TRANSITION_H_ */
