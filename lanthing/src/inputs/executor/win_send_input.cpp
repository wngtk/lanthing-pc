#include "win_send_input.h"
#include "scancode.h"

#include <Windows.h>
#include <winuser.h>

#include <optional>

#include <g3log/g3log.hpp>

#include <ltproto/peer2peer/keyboard_event.pb.h>
#include <ltproto/peer2peer/mouse_event.pb.h>

namespace {

// ret: {win_key, use_scancode, extented, valid}
constexpr auto scancode_to_winkey(const lt::Scancode scancode)
    -> std::tuple<WORD, bool, bool, bool> {
    // FIXME: 有几个键是乱填的
    using lt::Scancode;
    if (scancode >= Scancode::SCANCODE_A && scancode <= Scancode::SCANCODE_Z) {
        return {'A' + scancode - Scancode::SCANCODE_A, true, false, true};
    }
    if (scancode >= Scancode::SCANCODE_1 && scancode < Scancode::SCANCODE_0) {
        return {'1' + scancode - Scancode::SCANCODE_1, true, false, true};
    }
    if (scancode == Scancode::SCANCODE_0) {
        return {'0', true, false, true};
    }
    if (scancode >= Scancode::SCANCODE_F1 && scancode <= Scancode::SCANCODE_F12) {
        return {VK_F1 + scancode - Scancode::SCANCODE_F1, true, false, true};
    }
    if (scancode >= Scancode::SCANCODE_KP_1 && scancode <= Scancode::SCANCODE_KP_9) {
        return {VK_NUMPAD1 + scancode - Scancode::SCANCODE_KP_1, true, false, true};
    }
    switch (scancode) {
    case lt::SCANCODE_KP_PERIOD:
        return {VK_DECIMAL, true, false, true};
    case lt::SCANCODE_RETURN:
        return {VK_RETURN, true, false, true};
    case lt::SCANCODE_ESCAPE:
        return {VK_ESCAPE, true, false, true};
    case lt::SCANCODE_BACKSPACE:
        return {VK_BACK, true, false, true};
    case lt::SCANCODE_TAB:
        return {VK_TAB, true, false, true};
    case lt::SCANCODE_SPACE:
        return {VK_SPACE, true, false, true};
    case lt::SCANCODE_MINUS:
        return {VK_OEM_MINUS, true, false, true};
    case lt::SCANCODE_EQUALS:
        return {VK_OEM_PLUS, true, false, true};
    case lt::SCANCODE_LEFTBRACKET:
        return {VK_OEM_4, true, false, true};
    case lt::SCANCODE_RIGHTBRACKET:
        return {VK_OEM_6, true, false, true};
    case lt::SCANCODE_BACKSLASH:
    case lt::SCANCODE_NONUSHASH:
        return {VK_OEM_5, true, false, true};
    case lt::SCANCODE_SEMICOLON:
        return {VK_OEM_1, true, false, true};
    case lt::SCANCODE_APOSTROPHE:
        return {VK_OEM_7, true, false, true};
    case lt::SCANCODE_GRAVE:
        return {VK_OEM_3, true, false, true};
    case lt::SCANCODE_COMMA:
        return {VK_OEM_COMMA, true, false, true};
    case lt::SCANCODE_PERIOD:
        return {VK_OEM_PERIOD, true, false, true};
    case lt::SCANCODE_SLASH:
        return {VK_OEM_2, true, false, true};
    case lt::SCANCODE_CAPSLOCK:
        return {VK_CAPITAL, true, false, true};
    case lt::SCANCODE_PRINTSCREEN:
        return {VK_SNAPSHOT, true, false, true};
    case lt::SCANCODE_SCROLLLOCK:
        return {VK_SCROLL, true, false, true};
    case lt::SCANCODE_PAUSE:
        return {VK_PAUSE, false, false, true};
    case lt::SCANCODE_INSERT:
        return {VK_INSERT, true, true, true};
    case lt::SCANCODE_HOME:
        return {VK_HOME, true, true, true};
    case lt::SCANCODE_PAGEUP:
        return {VK_PRIOR, true, true, true};
    case lt::SCANCODE_DELETE:
        return {VK_DELETE, true, true, true};
    case lt::SCANCODE_END:
        return {VK_END, true, true, true};
    case lt::SCANCODE_PAGEDOWN:
        return {VK_NEXT, true, true, true};
    case lt::SCANCODE_RIGHT:
        return {VK_RIGHT, true, true, true};
    case lt::SCANCODE_LEFT:
        return {VK_LEFT, true, true, true};
    case lt::SCANCODE_DOWN:
        return {VK_DOWN, true, true, true};
    case lt::SCANCODE_UP:
        return {VK_UP, true, true, true};
    case lt::SCANCODE_NUMLOCKCLEAR:
        return {VK_NUMLOCK, true, false, true};
    case lt::SCANCODE_KP_DIVIDE:
        return {VK_DIVIDE, true, true, true};
    case lt::SCANCODE_KP_MULTIPLY:
        return {VK_MULTIPLY, true, false, true};
    case lt::SCANCODE_KP_MINUS:
        return {VK_SUBTRACT, true, false, true};
    case lt::SCANCODE_KP_PLUS:
        return {VK_ADD, true, false, true};
    case lt::SCANCODE_KP_ENTER:
        return {VK_RETURN, true, true, true};
    case lt::SCANCODE_KP_0:
        return {VK_NUMPAD0, true, false, true};
    case lt::SCANCODE_KP_DECIMAL:
        return {VK_DECIMAL, true, false, true};
    case lt::SCANCODE_LCTRL:
        return {VK_LCONTROL, true, false, true};
    case lt::SCANCODE_LSHIFT:
        return {VK_LSHIFT, true, false, true};
    case lt::SCANCODE_LALT:
        return {VK_LMENU, true, false, true};
    case lt::SCANCODE_LGUI:
        return {VK_LWIN, true, true, true};
    case lt::SCANCODE_RCTRL:
        return {VK_RCONTROL, true, true, true};
    case lt::SCANCODE_RSHIFT:
        return {VK_RSHIFT, true, false, true};
    case lt::SCANCODE_RALT:
        return {VK_RMENU, true, true, true};
    case lt::SCANCODE_RGUI:
        return {VK_RWIN, true, true, true};
    case lt::SCANCODE_APPLICATION:
        return {VK_APPS, true, true, true};
    default:
        break;
    }
    return {0, false, false, false};
}

} // namespace

namespace lt {

void Win32SendInput::onKeyboardEvent(const std::shared_ptr<google::protobuf::MessageLite>& msg) {
    auto keyboard = std::static_pointer_cast<ltproto::peer2peer::KeyboardEvent>(msg);
    Scancode sc = static_cast<Scancode>(keyboard->key());
    auto [key, use_scancode, extented, valid] = scancode_to_winkey(sc);
    if (!valid) {
        return;
    }
    INPUT inputs[1] = {};
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = key;
    if (use_scancode) {
        inputs[0].ki.wScan =
            static_cast<WORD>(MapVirtualKeyW(static_cast<UINT>(key), MAPVK_VK_TO_VSC_EX));
        inputs[0].ki.dwFlags = KEYEVENTF_SCANCODE;
    }
    inputs[0].ki.dwFlags |= keyboard->down() ? 0 : KEYEVENTF_KEYUP;
    inputs[0].ki.dwFlags |= extented ? KEYEVENTF_EXTENDEDKEY : 0;
    // FIXME: implement it;
    inputs[0].ki.time = 0;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

bool Win32SendInput::initKeyMouse() {
    return true;
}

void Win32SendInput::onMouseEvent(const std::shared_ptr<google::protobuf::MessageLite>& msg) {
    auto mouse = std::static_pointer_cast<ltproto::peer2peer::MouseEvent>(msg);
    INPUT inputs[1] = {};
    inputs[0].type = INPUT_MOUSE;
    if (mouse->has_key_falg()) {
        switch (mouse->key_falg()) {
        case ltproto::peer2peer::MouseEvent_KeyFlag_LeftDown:
            inputs[0].mi.dwFlags |= MOUSEEVENTF_LEFTDOWN;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_LeftUp:
            inputs[0].mi.dwFlags |= MOUSEEVENTF_LEFTUP;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_RightDown:
            inputs[0].mi.dwFlags |= MOUSEEVENTF_RIGHTDOWN;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_RightUp:
            inputs[0].mi.dwFlags |= MOUSEEVENTF_RIGHTUP;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_MidDown:
            inputs[0].mi.dwFlags |= MOUSEEVENTF_MIDDLEDOWN;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_MidUp:
            inputs[0].mi.dwFlags |= MOUSEEVENTF_MIDDLEUP;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_X1Down:
            inputs[0].mi.mouseData = XBUTTON1;
            inputs[0].mi.dwFlags |= MOUSEEVENTF_XDOWN;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_X1Up:
            inputs[0].mi.mouseData = XBUTTON1;
            inputs[0].mi.dwFlags |= MOUSEEVENTF_XUP;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_X2Down:
            inputs[0].mi.mouseData = XBUTTON2;
            inputs[0].mi.dwFlags |= MOUSEEVENTF_XDOWN;
            break;
        case ltproto::peer2peer::MouseEvent_KeyFlag_X2Up:
            inputs[0].mi.mouseData = XBUTTON2;
            inputs[0].mi.dwFlags |= MOUSEEVENTF_XUP;
            break;
        default:
            // LOG(WARNING) << "";
            break;
        }
    }

    if (isAbsoluteMouse()) {
        if (mouse->has_x() || mouse->has_y()) {
            inputs[0].mi.dx = static_cast<LONG>((65535.0f * mouse->x()));
            inputs[0].mi.dy = static_cast<LONG>((65535.0f * mouse->y()));
            inputs[0].mi.dwFlags |= MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
        }
    }
    else {
        if (mouse->has_delta_x() || mouse->has_delta_y()) {
            inputs[0].mi.dx = mouse->delta_x();
            inputs[0].mi.dy = mouse->delta_y();
            inputs[0].mi.dwFlags |= MOUSEEVENTF_MOVE;
        }
    }
    if (mouse->has_delta_z()) {
        inputs[0].mi.mouseData = mouse->delta_z();
        inputs[0].mi.dwFlags = MOUSEEVENTF_WHEEL;
    }

    // FIXME: implement it;
    inputs[0].mi.time = 0;
    SendInput(ARRAYSIZE(inputs), inputs, sizeof(INPUT));
}

} // namespace lt