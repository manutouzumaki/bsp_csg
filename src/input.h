#ifndef _INPUT_H_
#define _INPUT_H_

struct Button
{
    bool wasPress;
    bool isPress;
};

struct InputState
{
    Button keys[349];
    Button mouseButtons[3];
    i32 mouseX;
    i32 mouseY;

    // TODO: xinput for joysticks

};

#endif
