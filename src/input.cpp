bool KeyJustPress(u32 vkCode)
{
    Button button = gCurrentInput.keys[vkCode];
    if(button.wasPress == 0 && button.isPress == 1)
    {
        return true;
    }
    return false;
}

bool KeyPress(u32 vkCode)
{
    Button button = gCurrentInput.keys[vkCode];
    return button.isPress;
}

bool KeyJustUp(u32 vkCode)
{
    Button button = gCurrentInput.keys[vkCode];
    if(button.wasPress == 1 && button.isPress == 0)
    {
        return true;
    }
    return false;
}

bool KeyUp(u32 vkCode)
{
    Button button = gCurrentInput.keys[vkCode];
    return !button.isPress;

}

bool MouseButtonJustPress(u32 button_)
{

    Button button = gCurrentInput.mouseButtons[button_];
    if(button.wasPress == 0 && button.isPress == 1)
    {
        return true;
    }
    return false;
}

bool MouseButtonPress(u32 button_)
{
    Button button = gCurrentInput.mouseButtons[button_];
    return button.isPress;

}

bool MouseButtonJustUp(u32 button_)
{

    Button button = gCurrentInput.mouseButtons[button_];
    if(button.wasPress == 1 && button.isPress == 0)
    {
        return true;
    }
    return false;
}

bool MouseButtonUp(u32 button_)
{
    Button button = gCurrentInput.mouseButtons[button_];
    return !button.isPress;

}

i32 MousePosX()
{
    return gCurrentInput.mouseX;
}

i32 MousePosY()
{
    return gCurrentInput.mouseY;
}

i32 MouseLastPosX()
{
    return gLastInput.mouseX;
}

i32 MouseLastPosY()
{
    return gLastInput.mouseY;
}
