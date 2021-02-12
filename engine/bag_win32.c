#include "bag_engine_config.h"
#include "bag_engine.h"

#include <windows.h>

#include "wglext.h"

#include <stdio.h>
#include <locale.h>


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
    /* state */
    int processingEvents;
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

    bagWIN32.window = CreateWindowExW(
            0,
            windowClass.lpszClassName,
            L"FIXME",  // FIXME convert mb to wide
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

    HDC retrievedDC = GetDC(bagWIN32.window);
    if (!retrievedDC) {
        bagWIN32_error("Failed to retrieve device context!");
        exit(-1);
    }

    int pfIndex = ChoosePixelFormat(retrievedDC, &pfd);
    if (pfIndex == 0) {
        bagWIN32_error("Failed to chooe pixel format!");
        exit(-1);
    }

    BOOL bResult = SetPixelFormat(retrievedDC, pfIndex, &pfd);
    if (!bResult) {
        bagWIN32_error("Failed to set pixel format!");
        exit(-1);
    }

    HGLRC tContext = wglCreateContext(retrievedDC);
    if (!tContext) {
        bagWIN32_error("Failed to create temporary OpenGL context!");
        exit(-1);
    }

    bResult = wglMakeCurrent(retrievedDC, tContext);
    if (!bResult) {
        bagWIN32_error("Failed to make tomporary OpenGL context current!");
        exit(-1);
    }

    int procIndex = -1;
    if ((procIndex = bagWIN32_loadWGLFunctionPointers()) != -1) {
        fprintf(stderr, "Failed to retrieve function number #d\n", procIndex);
        bagWIN32_error("Failed to load all WGL extension functions!");
        exit(-1);
    }

    // TODO: expose to the user
    int attribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
        WGL_CONTEXT_MINOR_VERSION_ARB, 4,
        WGL_CONTEXT_FLAGS_ARB, 0,
        0
    };

    bagWIN32.context = wglCreateContextAttribsARB(retrievedDC, 0, attribs);
    if (!bagWIN32.context) {
        bagWIN32_error("Failed to create OpenGL context!");
        exit(-1);
    }

    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(tContext);
    wglMakeCurrent(retrievedDC, bagWIN32.context);

    ReleaseDC(bagWIN32.window, retrievedDC);

    if (!gladLoaderLoadGL()) {
        bagWIN32_error("glad failed to load OpenGL!");
        exit(-1);
    }
    bagWIN32.openglLoaded = 1;

    bagWIN32.deviceContext = GetDC(bagWIN32.window);


    /* Running the main function */

#ifdef BAGE_WINDOWS_CONSOLE
    int programReturn = bagE_main(argc, argv);
#else
    // TODO implement parameters
    int programReturn = bagE_main(0, NULL);
#endif

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
