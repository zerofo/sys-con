#include "switch.h"
#include <stdio.h>
#include <string>

std::string buttonToStr(u64 ButtonMask)
{
    std::string buttonStr;

    if (ButtonMask & HidNpadButton_A)
        buttonStr += "A ";
    if (ButtonMask & HidNpadButton_B)
        buttonStr += "B ";
    if (ButtonMask & HidNpadButton_X)
        buttonStr += "X ";
    if (ButtonMask & HidNpadButton_Y)
        buttonStr += "Y ";
    if (ButtonMask & HidNpadButton_StickL)
        buttonStr += "StickL ";
    if (ButtonMask & HidNpadButton_StickR)
        buttonStr += "StickR ";
    if (ButtonMask & HidNpadButton_L)
        buttonStr += "L ";
    if (ButtonMask & HidNpadButton_R)
        buttonStr += "R ";
    if (ButtonMask & HidNpadButton_ZL)
        buttonStr += "ZL ";
    if (ButtonMask & HidNpadButton_ZR)
        buttonStr += "ZR ";
    if (ButtonMask & HidNpadButton_Plus)
        buttonStr += "+ ";
    if (ButtonMask & HidNpadButton_Minus)
        buttonStr += "- ";
    if (ButtonMask & HidNpadButton_Left)
        buttonStr += "Left ";
    if (ButtonMask & HidNpadButton_Up)
        buttonStr += "Up ";
    if (ButtonMask & HidNpadButton_Right)
        buttonStr += "Right ";
    if (ButtonMask & HidNpadButton_Down)
        buttonStr += "Down ";
    /*if (ButtonMask & HidNpadButton_StickLLeft)
        buttonStr += "StickLLeft ";
    if (ButtonMask & HidNpadButton_StickLUp)
        buttonStr += "StickLUp ";
    if (ButtonMask & HidNpadButton_StickLRight)
        buttonStr += "StickLRight ";
    if (ButtonMask & HidNpadButton_StickLDown)
        buttonStr += "StickLDown ";
    if (ButtonMask & HidNpadButton_StickRLeft)
        buttonStr += "StickRLeft ";
    if (ButtonMask & HidNpadButton_StickRUp)
        buttonStr += "StickRUp ";
    if (ButtonMask & HidNpadButton_StickRRight)
        buttonStr += "StickRRight ";
    if (ButtonMask & HidNpadButton_StickRDown)
        buttonStr += "StickRDown ";
    if (ButtonMask & HidNpadButton_LeftSL)
        buttonStr += "LeftSL ";
    if (ButtonMask & HidNpadButton_LeftSR)
        buttonStr += "LeftSR ";
    if (ButtonMask & HidNpadButton_RightSL)
        buttonStr += "RightSL ";
    if (ButtonMask & HidNpadButton_RightSR)
        buttonStr += "RightSR ";
    */

    buttonStr.erase(buttonStr.find_last_not_of(' ') + 1);

    return buttonStr;
}

int main()
{
    char outputBuffer[256];

    PrintConsole *console = consoleGetDefault();

    consoleInit(console);

    padConfigureInput(8, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeAny(&pad);
    hidSetNpadHandheldActivationMode(HidNpadHandheldActivationMode_Single);

    printf("Welcome to sys-con debug application\n");
    printf("Press + to increase vibration\n");
    printf("Press - to decrease vibration\n");
    printf("Press L+R (or Home) to exit !\n");
    printf("\n");

    while (appletMainLoop())
    {
        padUpdate(&pad);

        u64 buttonDown = padGetButtons(&pad);
        HidAnalogStickState stick1 = padGetStickPos(&pad, 0);
        HidAnalogStickState stick2 = padGetStickPos(&pad, 1);

        snprintf(outputBuffer, console->consoleWidth, "Button: [%s] Stick1 [%06d, %06d] Stick2 [%06d, %06d]                                                   ",
                 buttonToStr(buttonDown).c_str(),
                 stick1.x, stick1.y,
                 stick2.x, stick2.y);

        outputBuffer[console->consoleWidth - 1] = '\r';
        outputBuffer[console->consoleWidth] = '\0';
        printf(outputBuffer);

        if (buttonDown & HidNpadButton_L && buttonDown & HidNpadButton_R)
            break;

        if (buttonDown & HidNpadButton_Plus || buttonDown & HidNpadButton_Minus)
        {
            // Not implemented
        }

        consoleUpdate(console);
    }

    consoleExit(console);
    return 0;
}