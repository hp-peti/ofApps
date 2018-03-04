#ifndef KEY_PRESSED_H_
#define KEY_PRESSED_H_
#pragma once

#include <ofEvents.h>

struct KeyPressed {
    operator bool() const {
        return ofGetKeyPressed(key);
    }
    explicit KeyPressed(int key): key(key) {}
private:
    const int key;
};

#endif // KEY_PRESSED_H_
