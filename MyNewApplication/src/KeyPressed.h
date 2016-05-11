#pragma once

#ifndef KEY_PRESSED_H_
#define KEY_PRESSED_H_

#include <ofEvents.h>

struct KeyPressed {
    operator bool() const {
        return ofGetKeyPressed(key);
    }
    explicit KeyPressed(int key): key(key) {}
    KeyPressed() = delete;
    KeyPressed(const KeyPressed &) = delete;
    KeyPressed(KeyPressed &&) = delete;
    KeyPressed &operator=(const KeyPressed &) = delete;
    KeyPressed &operator=(KeyPressed &&) = delete;
private:
    int key;
};

#endif // KEY_PRESSED_H_
