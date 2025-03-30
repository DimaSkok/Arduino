#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef void (*SwitchCallback)();
typedef enum {
    A,
    B,
    C
} SwitchNum;

class SwitchClass {
    private:
        uint8_t state[3] = { 0, 0, 0 };
        typedef struct {
            SwitchCallback pressed_cb;
            SwitchCallback release_cb;
        } SwitchCallbackGroup_t;
        SwitchCallbackGroup_t cb[3];

    public:
        SwitchClass();
        void begin() ;
        void onPressed(SwitchNum sw, SwitchCallback cb) ;
        void onRelease(SwitchNum sw, SwitchCallback cb) ;
        bool isPressed(SwitchNum sw) ;
        bool isRelease(SwitchNum sw) ;

        void loop() ;
};

extern SwitchClass Switch;
