#include <node_api.h>
#include <napi.h>
#include <windows.h>
#include <hidusage.h>
#include <sstream>

namespace
{
    HWND windowHwnd;
    HWND rawInputWindowHandle = nullptr;
    LPCSTR rawInputWindowClassName = "HikRawInputWindow";

    bool isDesktopActive()
    {
        POINT mouse_position;
        GetCursorPos(&mouse_position);
        HWND foreground = WindowFromPoint(mouse_position);
        // std::stringstream ss;
        // ss << foreground;
        // auto str = ss.str();
        // OutputDebugStringA(str.data());
        // OutputDebugStringA("\r\n");
        HWND tempHwnd{0};
        // HWND foreground = GetForegroundWindow();
        HWND desktop = GetDesktopWindow();
        HWND shell = GetShellWindow();
        HWND desktopWorkerW = FindWindowEx(nullptr, nullptr, "WorkerW", nullptr);
        while (desktopWorkerW != nullptr)
        {
            HWND shellDllDefView = FindWindowEx(desktopWorkerW, nullptr, "SHELLDLL_DefView", nullptr);
            if (shellDllDefView != nullptr)
            {
                tempHwnd = FindWindowEx(shellDllDefView, nullptr, "SysListView32", nullptr);
                break;
            }
            desktopWorkerW = FindWindowEx(nullptr, desktopWorkerW, "WorkerW", nullptr);
        }

        HWND wallpaperWorkerW = FindWindowEx(nullptr, desktopWorkerW, "WorkerW", nullptr);
        bool isForeground = (foreground == desktop ||
                             foreground == shell ||
                             foreground == tempHwnd ||
                             foreground == desktopWorkerW ||
                             foreground == wallpaperWorkerW);
        return isForeground;
    }

    LRESULT CALLBACK handleWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        if (isDesktopActive() && uMsg == WM_INPUT)
        {
            UINT dwSize = sizeof(RAWINPUT);
            static BYTE lpb[sizeof(RAWINPUT)];
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
            auto *raw = (RAWINPUT *)lpb;
            switch (raw->header.dwType)
            {
            case RIM_TYPEMOUSE:
            {
                RAWMOUSE rawMouse = raw->data.mouse;
                if ((rawMouse.usButtonFlags & RI_MOUSE_WHEEL) == RI_MOUSE_WHEEL || (rawMouse.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL)
                {
                    bool isHorizontalScroll = (rawMouse.usButtonFlags & RI_MOUSE_HWHEEL) == RI_MOUSE_HWHEEL;
                    auto wheelDelta = (float)(short)rawMouse.usButtonData;
                    if (isHorizontalScroll)
                    {
                        PostMessageA(windowHwnd, WM_HSCROLL, wheelDelta > 0 ? SB_LINELEFT : SB_LINERIGHT, 0);
                    }
                    else
                    {
                        PostMessageA(windowHwnd, WM_VSCROLL, wheelDelta > 0 ? SB_LINEUP : SB_LINEDOWN, 0);
                    }
                    break;
                }
                POINT point;
                GetCursorPos(&point);
                ScreenToClient(windowHwnd, &point);
                auto lParam = MAKELPARAM(point.x, point.y);
                switch (rawMouse.ulButtons)
                {
                case RI_MOUSE_LEFT_BUTTON_DOWN:
                {
                    PostMessageA(windowHwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
                    break;
                }
                case RI_MOUSE_LEFT_BUTTON_UP:
                {
                    PostMessageA(windowHwnd, WM_LBUTTONUP, MK_LBUTTON, lParam);
                    break;
                }
                case RI_MOUSE_RIGHT_BUTTON_DOWN:
                {
                    PostMessageA(windowHwnd, WM_RBUTTONDOWN, MK_RBUTTON, lParam);
                    break;
                }
                case RI_MOUSE_RIGHT_BUTTON_UP:
                {
                    PostMessageA(windowHwnd, WM_RBUTTONUP, MK_RBUTTON, lParam);
                    break;
                }
                default:
                {
                    PostMessageA(windowHwnd, WM_MOUSEMOVE, 0x0020, lParam);
                    break;
                }
                }
                break;
            }
            case RIM_TYPEKEYBOARD:
            {
                auto message = raw->data.keyboard.Message;
                auto vKey = raw->data.keyboard.VKey;
                auto makeCode = raw->data.keyboard.MakeCode;
                auto flags = raw->data.keyboard.Flags;
                std::uint32_t lParam = 1u;
                lParam |= static_cast<std::uint32_t>(makeCode) << 16;
                lParam |= 1u << 24;
                lParam |= 0u << 29;
                if (!(flags & RI_KEY_BREAK)) {
                    lParam |= 1u << 30;
                    lParam |= 1u << 31;
                }
                PostMessageA(windowHwnd, message, vKey, lParam);
                break;
            }
            }
        }
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    void startForwardingRawInput(Napi::Env env)
    {
        if (rawInputWindowHandle != nullptr)
        {
            return;
        }
        HINSTANCE hInstance = GetModuleHandleA(nullptr);
        WNDCLASS wc = {};
        wc.lpfnWndProc = handleWindowMessage;
        wc.hInstance = hInstance;
        wc.lpszClassName = rawInputWindowClassName;
        if (!RegisterClass(&wc))
        {
            Napi::TypeError::New(env, "Could not register raw input window class").ThrowAsJavaScriptException();
            return;
        }
        rawInputWindowHandle = CreateWindowEx(0, wc.lpszClassName, nullptr, 0, 0, 0, 0, 0, HWND_MESSAGE, nullptr, hInstance, nullptr);
        if (!rawInputWindowHandle)
        {
            Napi::TypeError::New(env, "Could not create raw input window").ThrowAsJavaScriptException();
        }
        RAWINPUTDEVICE rids[2];
        rids[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
        rids[0].usUsage = HID_USAGE_GENERIC_MOUSE;
        rids[0].dwFlags = RIDEV_INPUTSINK;
        rids[0].hwndTarget = rawInputWindowHandle;

        rids[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
        rids[1].usUsage = HID_USAGE_GENERIC_KEYBOARD;
        rids[1].dwFlags = RIDEV_EXINPUTSINK;
        rids[1].hwndTarget = rawInputWindowHandle;

        if (RegisterRawInputDevices(rids, 2, sizeof(rids[0])) == FALSE)
        {
            Napi::TypeError::New(env, "Could not register raw input devices").ThrowAsJavaScriptException();
        }
    }
    void stopForwardingRawInput()
    {
        if (rawInputWindowHandle == nullptr)
        {
            return;
        }
        HINSTANCE hInstance = GetModuleHandleA(nullptr);
        DestroyWindow(rawInputWindowHandle);
        UnregisterClass(rawInputWindowClassName, hInstance);
        rawInputWindowHandle = nullptr;
    }
}
void embed(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    auto buffer = info[0].As<Napi::Buffer<void *>>();
    windowHwnd = static_cast<HWND>(*buffer.Data());
    auto shellHandle = GetShellWindow();
    SendMessage(shellHandle, 0x052C, 0x0000000D, 0);
    SendMessage(shellHandle, 0x052C, 0x0000000D, 1);
    static HWND workerW = nullptr;
    EnumWindows([](HWND topHandle, LPARAM topParamHandle)
                {
        HWND shellDllDefView = FindWindowEx(topHandle, nullptr, "SHELLDLL_DefView", nullptr);
        if (shellDllDefView != nullptr) {
            workerW = FindWindowEx(nullptr, topHandle, "WorkerW", nullptr);
        }
        return TRUE; },
                NULL);
    if (workerW == nullptr)
    {
        Napi::TypeError::New(env, "couldn't locate WorkerW").ThrowAsJavaScriptException();
        return;
    }
    SetParent(windowHwnd, workerW);
    SetWindowLong(windowHwnd, GWL_EXSTYLE, GetWindowLong(windowHwnd, GWL_EXSTYLE) | WS_EX_LAYERED);
    SetLayeredWindowAttributes(windowHwnd, 0, 255, LWA_ALPHA);
    startForwardingRawInput(env);
}
void unEmbed(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    auto buffer = info[0].As<Napi::Buffer<void *>>();
    HWND windowHandle = static_cast<HWND>(*reinterpret_cast<void **>(buffer.Data()));
    SetParent(windowHandle, nullptr);
    SetWindowLong(windowHandle, GWL_EXSTYLE, GetWindowLong(windowHandle, GWL_EXSTYLE) & ~WS_EX_LAYERED);
    stopForwardingRawInput();
    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, nullptr, SPIF_UPDATEINIFILE);
    windowHwnd = nullptr;
}
void refresh(const Napi::CallbackInfo &info)
{
    Napi::Env env = info.Env();
    Napi::HandleScope scope(env);
    auto buffer = info[0].As<Napi::Buffer<void *>>();
    HWND windowHandle = static_cast<HWND>(*reinterpret_cast<void **>(buffer.Data()));
    HWND desktop = GetDesktopWindow();
    SetForegroundWindow(desktop);
    HWND shell = GetShellWindow();
    SetForegroundWindow(shell);
    HWND hwndDesktop = FindWindowA("Progman", "Program Manager");
    SetForegroundWindow(hwndDesktop);
}