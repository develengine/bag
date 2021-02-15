#include "bag_engine_config.h"
#include "bag_engine.h"

#include <windows.h>

#include "wglext.h"

#include <stdio.h>
#include <locale.h>
#include <wchar.h>

// Uses dedicated gpu by default but creates a dll file.
// __declspec(dllexport) unsigned long NvOptimusEnablement = 1;
// __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;

/*
 * [X] void bagE_pollEvents();
 * [X] void bagE_swapBuffers();
 * 
 * [X] int bagE_getCursorPosition(int *x, int *y);
 * [X] void bagE_getWindowSize(int *width, int *height);
 * 
 * [ ] int bagE_isAdaptiveVsyncAvailable(void);
 * 
 * [X] int bagE_setHiddenCursor(int value);
 * [X] void bagE_setFullscreen(int value);
 * [X] void bagE_setWindowTitle(char *value);
 * [ ] void bagE_setSwapInterval(int value);
 * [X] void bagE_setCursorPosition(int x, int y);
 *
 * [X] bagE_EventWindowClose,
 * [X] bagE_EventWindowResize,
 *
 * [ ] bagE_EventMouseMotion,
 * [ ] bagE_EventMouseButtonUp,
 * [ ] bagE_EventMouseButtonDown,
 * [ ] bagE_EventMouseWheel,
 * [ ] bagE_EventMousePosition,
 *
 * [X] bagE_EventKeyUp,
 * [X] bagE_EventKeyDown,
 * [ ] bagE_EventTextUTF8,
 * [ ] bagE_EventTextUTF32,
 */

/* FIXME 
 * can't change keyboard language when window selected
 * cursor doesn't update to arrow when hower over client area
 */


static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = 0;
static PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = 0;

#define BAGWIN32_CHECK_WGL_PROC_ADDR(identifier) {\
    void *pfn = (void*)identifier;\
    if ((pfn == 0)\
    ||  (pfn == (void*)0x1) || (pfn == (void*)0x2) || (pfn == (void*)0x3)\
    ||  (pfn == (void*)-1)\
    ) return procedureIndex;\
    ++procedureIndex;\
}


struct bagWIN32_Program
{
    HINSTANCE instance;
    HWND window;
    HGLRC context;
    HDC deviceContext;
    int openglLoaded;
    int newContext;
    /* state */
    int processingEvents;
    int cursorHidden;
    WINDOWPLACEMENT previousWP;
} bagWIN32;

typedef struct bagWIN32_Program bagWIN32_Program;


static void bagWIN32_error(const char *message)
{
    MessageBoxA(NULL, message, "BAG Engine", MB_ICONERROR);
}


static int bagWIN32_loadWGLFunctionPointers()
{
    int procedureIndex = 0;

    wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)
        wglGetProcAddress("wglCreateContextAttribsARB");
    BAGWIN32_CHECK_WGL_PROC_ADDR(wglCreateContextAttribsARB);

    wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)
        wglGetProcAddress("wglSwapIntervalEXT");
    BAGWIN32_CHECK_WGL_PROC_ADDR(wglSwapIntervalEXT);

    return -1;
}


LRESULT CALLBACK bagWIN32_windowProc(
        HWND windowHandle,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
);


#ifdef BAGE_WINDOWS_CONSOLE
int main(int argc, char *argv[]) {
    bagWIN32.instance = GetModuleHandle(NULL);
#else
int WinMain(
        HINSTANCE hinstance,
        HINSTANCE previousInstance,
        LPSTR commandLine,
        int nShowCmd
) {
    bagWIN32.instance = hinstance;
#endif
    setlocale(LC_CTYPE, ""); // NOTE: Might need to be LC_ALL
    bagWIN32.openglLoaded = 0;


    /* WIN32 */

    WNDCLASSW windowClass = {
        .style = CS_OWNDC,
        .lpfnWndProc = &bagWIN32_windowProc,
        .hInstance = bagWIN32.instance,
        .lpszClassName = L"bag engine"
    };

    ATOM registerOutput = RegisterClassW(&windowClass);
    if (!registerOutput) {
        bagWIN32_error("Failed to register window class!");
        exit(-1);
    }

    wchar_t windowTitle[256];
    int errorRet = mbstowcs_s(
            NULL,
            windowTitle,
            256,
            bagE_defaultTitle,
            256
    );
    if (errorRet) 
        fprintf(stderr, "Problems converting default window title");


    bagWIN32.window = CreateWindowExW(
            0,
            windowClass.lpszClassName,
            windowTitle,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            bagE_defaultWindowWidth,
            bagE_defaultWindowHeight,
            0,
            0,
            bagWIN32.instance,
            0
    );

    if (!bagWIN32.window) {
        bagWIN32_error("Failed to create window!");
        exit(-1);
    }

    bagWIN32.cursorHidden = 0;
    bagWIN32.previousWP.length = sizeof(bagWIN32.previousWP);

    
    /* Raw input */

    RAWINPUTDEVICE rawInputDevice = {
        .usUsagePage = 0x01,  // HID_USAGE_PAGE_GENERIC
        .usUsage = 0x02,  // HID_USAGE_GENERIC_MOUSE
        .dwFlags = 0,
        .hwndTarget = bagWIN32.window
    };

    if (!RegisterRawInputDevices(&rawInputDevice, 1, sizeof(rawInputDevice))) {
        bagWIN32_error("Failed to register raw input device!");
        exit(-1);
    }


    /* WGL context creation */

    // TODO: expose to the user
    PIXELFORMATDESCRIPTOR pfd = {
        .nSize = sizeof(PIXELFORMATDESCRIPTOR),
        .nVersion = 1,
        .dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW,
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 32,
        .cDepthBits = 24,
        .cStencilBits = 8,
        .iLayerType = PFD_MAIN_PLANE
    };

    bagWIN32.deviceContext = GetDC(bagWIN32.window);
    if (!bagWIN32.deviceContext) {
        bagWIN32_error("Failed to retrieve device context!");
        exit(-1);
    }

    int pfIndex = ChoosePixelFormat(bagWIN32.deviceContext, &pfd);
    if (pfIndex == 0) {
        bagWIN32_error("Failed to chooe pixel format!");
        exit(-1);
    }

    BOOL bResult = SetPixelFormat(bagWIN32.deviceContext, pfIndex, &pfd);
    if (!bResult) {
        bagWIN32_error("Failed to set pixel format!");
        exit(-1);
    }

    HGLRC tContext = wglCreateContext(bagWIN32.deviceContext);
    if (!tContext) {
        bagWIN32_error("Failed to create temporary OpenGL context!");
        exit(-1);
    }

    bResult = wglMakeCurrent(bagWIN32.deviceContext, tContext);
    if (!bResult) {
        bagWIN32_error("Failed to make tomporary OpenGL context current!");
        exit(-1);
    }

    int procIndex = -1;
    if ((procIndex = bagWIN32_loadWGLFunctionPointers()) != -1) {
        fprintf(stderr, "Failed to retrieve function number %d\n", procIndex);
        // bagWIN32_error("Failed to load all WGL extension functions!");
        // exit(-1);
        bagWIN32.context = tContext;
        bagWIN32.newContext = 0;
    } else {
        // TODO: expose to the user
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 4,
            WGL_CONTEXT_FLAGS_ARB, 0,
            0
        };
    
        bagWIN32.context = wglCreateContextAttribsARB(bagWIN32.deviceContext, 0, attribs);
        if (!bagWIN32.context) {
            bagWIN32_error("Failed to create OpenGL context!");
            exit(-1);
        }
    
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(tContext);
        wglMakeCurrent(bagWIN32.deviceContext, bagWIN32.context);
        bagWIN32.newContext = 1;
    }

    if (!gladLoaderLoadGL()) {
        bagWIN32_error("glad failed to load OpenGL!");
        exit(-1);
    }
    bagWIN32.openglLoaded = 1;


    /* Running the main function */

    int programReturn = bagE_main(__argc, __argv);

    ReleaseDC(bagWIN32.window, bagWIN32.deviceContext);
    wglDeleteContext(bagWIN32.context);

    return programReturn;
}


LRESULT CALLBACK bagWIN32_windowProc(
        HWND windowHandle,
        UINT message,
        WPARAM wParam,
        LPARAM lParam
) {
    if (!bagWIN32.openglLoaded)
        return DefWindowProcW(windowHandle, message, wParam, lParam);

    LRESULT result = 0;
    bagE_Event events[3];
    bagE_Event *event = events;

    for (int i = 0; i < 3; i++)
        events[i].type = bagE_EventNone;

    switch (message)
    {
        case WM_CLOSE:
            event->type = bagE_EventWindowClose;
            break;

        case WM_WINDOWPOSCHANGED: {
            unsigned int winFlags = ((WINDOWPOS*)lParam)->flags;

            if ((winFlags & SWP_NOSIZE) && (winFlags & SWP_NOMOVE))
                break;

            RECT winRect;
            GetClientRect(windowHandle, &winRect);

            if (!(winFlags & SWP_NOSIZE)) {
                event->type = bagE_EventWindowResize;
                event->data.windowResize.width = winRect.right - winRect.left;
                event->data.windowResize.height = winRect.bottom - winRect.top;
            }

            if (bagWIN32.cursorHidden)
                ClipCursor(&winRect);
        } break;

        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            event->type = message == WM_KEYDOWN || message == WM_SYSKEYDOWN
                        ? bagE_EventKeyDown : bagE_EventKeyUp;
            event->data.key.key = wParam;
            event->data.key.repeat = lParam & 0xFF;
        } break;

        case WM_MOUSEMOVE:
            event->type = bagE_EventMousePosition;
            event->data.mouse.x = (signed short)(lParam & 0xFFFF);
            event->data.mouse.y = (signed short)(lParam >> 16);
            break;

        default:
            result = DefWindowProcW(windowHandle, message, wParam, lParam);
    }

    for (int i = 0; i < 3; i++) {
        if (events[i].type != bagE_EventNone) {
            if (bagE_eventHandler(events+i))
                bagWIN32.processingEvents = 0;
        }
    }

    return result;
}


void bagE_pollEvents()
{
    MSG message;

    bagWIN32.processingEvents = 1;

    while  (bagWIN32.processingEvents
         && PeekMessage(&message, bagWIN32.window, 0, 0, PM_REMOVE)
    ) {
        if (message.message == WM_QUIT) {
            bagE_Event event = { .type = bagE_EventWindowClose };

            if (bagE_eventHandler(&event))
                return;

            continue;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }
}


void bagE_swapBuffers()
{
    SwapBuffers(bagWIN32.deviceContext);
}


int bagE_getCursorPosition(int *x, int *y)
{
    POINT point;
    BOOL ret = GetCursorPos(&point);

    if (!ret)
        return 0;

    ret = ScreenToClient(bagWIN32.window, &point);

    if (!ret)
        return 0;

    *x = point.x;
    *y = point.y;

    return 1;
}


void bagE_getWindowSize(int *width, int *height)
{
    RECT rect;
    GetClientRect(bagWIN32.window, &rect);
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
}


void bagE_setWindowTitle(char *value)
{
    SetWindowTextA(bagWIN32.window, value);
}


void bagE_setFullscreen(int value)
{
    DWORD style = GetWindowLong(bagWIN32.window, GWL_STYLE);
    int isOverlapped = style & WS_OVERLAPPEDWINDOW;

    if (isOverlapped && value) {
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };

        if (GetWindowPlacement(bagWIN32.window, &bagWIN32.previousWP) &&
                GetMonitorInfo(
                        MonitorFromWindow(bagWIN32.window, MONITOR_DEFAULTTOPRIMARY),
                        &monitorInfo
                )
        ) {
            SetWindowLong(bagWIN32.window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(
                    bagWIN32.window,
                    HWND_TOP,
                    monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.top,
                    monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                    monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                    SWP_NOOWNERZORDER | SWP_FRAMECHANGED
            );
        }
    } else if (!(isOverlapped || value)) {
        SetWindowLong(bagWIN32.window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(bagWIN32.window, &bagWIN32.previousWP);
        SetWindowPos(
                bagWIN32.window,
                NULL, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED
        );
    }
}


int bagE_setHiddenCursor(int value)
{
    if (!bagWIN32.cursorHidden && value) {
        RECT windowRect;
        GetClientRect(bagWIN32.window, &windowRect);
        ClientToScreen(bagWIN32.window, (POINT*)&windowRect.left);
        ClientToScreen(bagWIN32.window, (POINT*)&windowRect.right);

        bagWIN32.cursorHidden = 1;
        ClipCursor(&windowRect);
        SetCursor(NULL);
    } else if (bagWIN32.cursorHidden && !value) {
        bagWIN32.cursorHidden = 0;
        ClipCursor(NULL);
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }

    return 1;
}


void bagE_setCursorPosition(int x, int y)
{
    POINT point = { x, y };
    ClientToScreen(bagWIN32.window, &point);
    SetCursorPos(point.x, point.y);
}
