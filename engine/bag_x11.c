#include "bag_engine_config.h"
#include "bag_engine.h"

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XInput2.h>

#include <GL/glx.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>


#define GLX_CONTEXT_MAJOR_VERSION_ARB 0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB 0x2092

typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(
        Display*, GLXFBConfig,
        GLXContext, Bool,
        const int*
);
GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;

typedef void (*GLXSWAPINTERVALEXTPROC)(Display*, GLXDrawable, int);
GLXSWAPINTERVALEXTPROC glXSwapIntervalEXT = 0;


struct bagX11_Program
{
    Display *display;
    Window root;
    Window window;
    GLXContext context;
    int adaptiveVsync;
    int xiOpCode;
    Atom WM_DELETE_WINDOW;
    Atom _NET_WM_STATE;
    Atom _NET_WM_STATE_FULLSCREEN;
    XIM inputMethod;
    XIC inputClient;
    XPoint spot;
    XVaNestedList spotlist;
} bagX11;

typedef struct bagX11_Program bagX11_Program;


static int bagX11_ctxErrorOccurred = 0;

static int bagX11_ctxErrorHandler(Display *dpy, XErrorEvent *ev)
{
    bagX11_ctxErrorOccurred = 1;
    return 0;
}


int bagX11_ximopen(Display *dpy);

void bagX11_ximinstantiate(Display *dpy, XPointer client, XPointer call)
{
    if (bagX11_ximopen(dpy)) {
	XUnregisterIMInstantiateCallback(
                bagX11.display, NULL, NULL, NULL,
                bagX11_ximinstantiate, NULL
        );
    }
}

void bagX11_ximdestroy(XIM xim, XPointer client, XPointer call)
{
    bagX11.inputMethod = NULL;
    XRegisterIMInstantiateCallback(
            bagX11.display, NULL, NULL, NULL,
	    bagX11_ximinstantiate, NULL
    );
    XFree(bagX11.spotlist);
}

int bagX11_xicdestroy(XIC xim, XPointer client, XPointer call)
{
    bagX11.inputClient = NULL;
    return 1;
}

int bagX11_ximopen(Display *dpy)
{
    XIMCallback imdestroy = { .client_data = NULL, .callback = bagX11_ximdestroy };
    XICCallback icdestroy = { .client_data = NULL, .callback = bagX11_xicdestroy };

    bagX11.inputMethod = XOpenIM(bagX11.display, NULL, NULL, NULL);
    if (bagX11.inputMethod == NULL) {
        fprintf(stderr, "IM is null!\n");
	return 0;
    }

    if (XSetIMValues(bagX11.inputMethod, XNDestroyCallback, &imdestroy, NULL))
	fprintf(stderr, "XSetIMValues: Could not set XNDestroyCallback.\n");

    bagX11.spotlist = XVaCreateNestedList(
            0, XNSpotLocation,
            &bagX11.spot, NULL
    );

    if (bagX11.inputClient == NULL) {
	bagX11.inputClient = XCreateIC(
                bagX11.inputMethod, XNInputStyle,
		XIMPreeditNothing | XIMStatusNothing,
		XNClientWindow, bagX11.window,
		XNDestroyCallback, &icdestroy,
		NULL
        );
    }

    if (bagX11.inputClient == NULL)
	fprintf(stderr, "XCreateIC: Could not create input context.\n");

    return 1;
}


static int bagX11_isGLXExtensionSupported(const char *extList, const char *extension)
{
    const char *start;
    const char *where, *terminator;
  
    where = strchr(extension, ' ');
    if (where || *extension == '\0')
        return 0;

    for (start=extList;;) {
        where = strstr(start, extension);

        if (!where)
            break;

        terminator = where + strlen(extension);

        if (where == start || *(where - 1) == ' ')
            if (*terminator == ' ' || *terminator == '\0')
                return 1;

        start = terminator;
    }

    return 0;
}


int main(int argc, char *argv[])
{
    setlocale(LC_CTYPE, "");
    XSetLocaleModifiers("");


    /* Xlib */

    bagX11.display = XOpenDisplay(NULL);
    if (!bagX11.display) {
        fprintf(stderr, "Failed to connect to the display server!\n");
        exit(-1);
    }

    int visualAttribs[] = {
        GLX_X_RENDERABLE    , True,
        GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
        GLX_RENDER_TYPE     , GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
        GLX_RED_SIZE        , 8,
        GLX_GREEN_SIZE      , 8,
        GLX_BLUE_SIZE       , 8,
        GLX_ALPHA_SIZE      , 8,
        GLX_DEPTH_SIZE      , 24,
        GLX_STENCIL_SIZE    , 8,
        GLX_DOUBLEBUFFER    , True,
        None
    }; 

    int glxMajor, glxMinor;
    if ((!glXQueryVersion(bagX11.display, &glxMajor, &glxMinor))
     || (glxMajor == 1 && glxMinor < 3)
     || (glxMajor < 1)
    ) {
        fprintf(stderr, "Invalid GLX version!");
        exit(-1);
    }

    int fbCount;
    GLXFBConfig *fbConfigs = glXChooseFBConfig(
            bagX11.display,
            DefaultScreen(bagX11.display),
            visualAttribs, 
            &fbCount
    );
    if (!fbConfigs) {
        fprintf(stderr, "Failed to retrieve a frame buffer config!");
        exit(-1);
    }

#ifdef BAGE_PRINT_DEBUG_INFO
    printf("Found %d matching frame buffer configs.\n", fbCount);
#endif

    int bestFBConfig = -1, bestSampleCount = -1;
    for (int i = 0; i < fbCount; i++) {
        XVisualInfo *visualInfo = glXGetVisualFromFBConfig(bagX11.display, fbConfigs[i]);

        if (visualInfo) {
            int sampBuf, samples;
            glXGetFBConfigAttrib(bagX11.display, fbConfigs[i], GLX_SAMPLE_BUFFERS, &sampBuf);
            glXGetFBConfigAttrib(bagX11.display, fbConfigs[i], GLX_SAMPLES, &samples);

            if (bestFBConfig < 0 || (sampBuf && samples > bestSampleCount)) {
                bestFBConfig = i;
                bestSampleCount = samples;
            }
        }

        XFree(visualInfo);
    }

    GLXFBConfig fbConfig = fbConfigs[bestFBConfig];

    XFree(fbConfigs);

    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(bagX11.display, fbConfig);

#ifdef BAGE_PRINT_DEBUG_INFO
    printf("Chosen visual info ID = 0x%x\n", (uint32_t)(visualInfo->visualid)); 
#endif

    bagX11.root = RootWindow(bagX11.display, visualInfo->screen);

    Colormap colormap = XCreateColormap(
            bagX11.display, 
            bagX11.root,
            visualInfo->visual,
            AllocNone
    );

    XSetWindowAttributes setWindowAttrs = {
        .colormap = colormap,
        .background_pixmap = None,
        .border_pixel = 0,
        .event_mask = StructureNotifyMask
    };

    bagX11.window = XCreateWindow(
            bagX11.display,
            bagX11.root,
            0, 0,
            bagE_defaultWindowWidth,
            bagE_defaultWindowHeight,
            0,
            visualInfo->depth,
            InputOutput, 
            visualInfo->visual,
            CWBorderPixel | CWColormap | CWEventMask,
            &setWindowAttrs
    );

    if (!bagX11.window) {
        fprintf(stderr, "Failed to create window!\n");
        exit(-1);
    }

    XFree(visualInfo);

    XSelectInput(  // TODO Optimize depending on configuration
            bagX11.display,
            bagX11.window,
            KeyPressMask      | KeyReleaseMask    |
            ButtonPressMask   | ButtonReleaseMask |
            PointerMotionMask | KeymapStateMask   |
            ExposureMask
    );

    XStoreName(bagX11.display, bagX11.window, bagE_defaultTitle);

    bagX11.WM_DELETE_WINDOW = XInternAtom(bagX11.display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(bagX11.display, bagX11.window, &(bagX11.WM_DELETE_WINDOW), 1);

    XMapWindow(bagX11.display, bagX11.window);

    if (!bagX11_ximopen(bagX11.display)) {
	XRegisterIMInstantiateCallback(
                bagX11.display,
                NULL, NULL, NULL,
                bagX11_ximinstantiate,
                NULL
        );
    }

    int detectableAutoRepeat;
    XkbSetDetectableAutoRepeat(bagX11.display, True, &detectableAutoRepeat);
    if (!detectableAutoRepeat) {
        fprintf(stderr, "Detectable auto repeat is not supported!\n");
        exit(-1);
    }

    bagX11._NET_WM_STATE = XInternAtom(bagX11.display, "_NET_WM_STATE", False);
    bagX11._NET_WM_STATE_FULLSCREEN = XInternAtom(bagX11.display, "_NET_WM_STATE_FULLSCREEN", False);


    /* XInput2 */

    // TODO Skip if mouse motion is not requested
    int xiEvent, xiError;
    if (!XQueryExtension(bagX11.display, "XInputExtension", &bagX11.xiOpCode, &xiEvent, &xiError)) {
        fprintf(stderr, "X Input extension not available.\n");
        exit(-1);
    }

    int xiMajor = 2, xiMinor = 0;
    if (XIQueryVersion(bagX11.display, &xiMajor, &xiMinor) == BadRequest) {
        fprintf(stderr, "XI2 not available. Server supports %d.%d\n", xiMajor, xiMinor);
        exit(-1);
    }

    unsigned char mask[XIMaskLen(XI_RawMotion)] = { 0 };
    XIEventMask eventMask = {
        .deviceid = XIAllMasterDevices,
        .mask_len = sizeof(mask),
        .mask = mask
    };
    XISetMask(mask, XI_RawMotion);

    XISelectEvents(bagX11.display, bagX11.root, &eventMask, 1);


    /* GLX context creation */

    const char *glxExtensions = glXQueryExtensionsString(
            bagX11.display,
            DefaultScreen(bagX11.display)
    );

    glXCreateContextAttribsARB = (GLXCREATECONTEXTATTRIBSARBPROC)
        glXGetProcAddressARB((const GLubyte *)"glXCreateContextAttribsARB");

    bagX11.context = 0;
    bagX11_ctxErrorOccurred = 0;

    int (*oldHandler)(Display*, XErrorEvent*) = XSetErrorHandler(&bagX11_ctxErrorHandler);

    if ((!bagX11_isGLXExtensionSupported(glxExtensions, "GLX_ARB_create_context"))
     || (!glXCreateContextAttribsARB)
    ) {
        fprintf(stderr, "'glXCreateContextARB()' wasn't found! Using old OpenGL context.\n");
    }

    glXSwapIntervalEXT = (GLXSWAPINTERVALEXTPROC)
        glXGetProcAddress((const GLubyte *)"glXSwapIntervalEXT");

    if ((!bagX11_isGLXExtensionSupported(glxExtensions, "GLX_EXT_swap_control"))
     || (!glXCreateContextAttribsARB)
    ) {
        fprintf(stderr, "'glXSwapIntervalEXT' couldn't be retrieved!\n");
    }

    bagX11.adaptiveVsync = bagX11_isGLXExtensionSupported(
            glxExtensions,
            "GLX_EXT_swap_control_tear"
    );

#ifdef BAGE_PRINT_DEBUG_INFO
    if (bagX11.adaptiveVsync)
        printf("Adaptive Vsync is supported.\n");
    else
        printf("Adaptive Vsync isn't supported.\n");
#endif

    int contextAttribs[] = {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 4,
        GLX_CONTEXT_MINOR_VERSION_ARB, 6,
        None
    }; 

    bagX11.context = glXCreateContextAttribsARB(
            bagX11.display,
            fbConfig,
            0, True,
            contextAttribs
    );

    XSync(bagX11.display, False);

    if (bagX11_ctxErrorOccurred || !bagX11.context) {
        fprintf(stderr, "Failed to create OpenGL context!\n");
        exit(-1);
    }

    XSetErrorHandler(oldHandler);

#ifdef BAGE_PRINT_DEBUG_INFO
    printf((glXIsDirect(bagX11.display, bagX11.context) ?
                "Direct context.\n" : "Indirect context.\n"));
#endif

    glXMakeCurrent(bagX11.display, bagX11.window, bagX11.context);

    if (!gladLoaderLoadGL()) {
        fprintf(stderr, "glad failed to load OpenGL functions!\n");
        exit(-1);
    }


    /* Running the main function */

    int programReturn = bagE_main(argc, argv);

    glXMakeCurrent(bagX11.display, 0, 0);
    glXDestroyContext(bagX11.display, bagX11.context);
    XDestroyWindow(bagX11.display, bagX11.window);
    XFreeColormap(bagX11.display, colormap);
    XCloseDisplay(bagX11.display);

    return programReturn;
}


static bagE_ModMask bagX11_convertModMask(unsigned int state)
{
    bagE_ModMask modMask = 0;

    if (state & ShiftMask)   modMask |= BAGE_MOD_BIT_SHIFT;
    if (state & LockMask)    modMask |= BAGE_MOD_BIT_LOCK;
    if (state & ControlMask) modMask |= BAGE_MOD_BIT_CONTROL;
    if (state & Mod1Mask)    modMask |= BAGE_MOD_BIT_ALT;
    if (state & Mod4Mask)    modMask |= BAGE_MOD_BIT_SUPER;

    return modMask;
}


static bagE_ButtonMask bagX11_convertButtonMask(unsigned int state)
{
    bagE_ButtonMask buttonMask = 0;

    if (state & Button1Mask) buttonMask |= BAGE_BUTTON_BIT_LEFT;
    if (state & Button2Mask) buttonMask |= BAGE_BUTTON_BIT_MIDDLE;
    if (state & Button3Mask) buttonMask |= BAGE_BUTTON_BIT_RIGHT;

    return buttonMask;
}


static unsigned int bagX11_UTF8ToUTF32(unsigned char *s)
{
    if ((s[0] & 0x80) == 0)
        return s[0];

    if ((s[0] & 0xE0) == 0xC0)
        return ((s[0] & 0x1F) << 6) |
                (s[1] & 0x3F);

    if ((s[0] & 0xF0) == 0xE0)
        return ((s[0] & 0x0F) << 12) |
               ((s[1] & 0x3F) << 6)  |
                (s[2] & 0x3F);

    if ((s[0] & 0xF8) == 0xF0)
        return ((s[0] & 0x07) << 18) |
               ((s[1] & 0x3F) << 12) |
               ((s[2] & 0x3F) << 6)  |
                (s[3] & 0x3F);

    return 0;
}


static void bagX11_translateEvent(bagE_Event *events, XEvent *xevent)
{
    bagE_Event *event = events;

    switch (xevent->type)
    {
        case MappingNotify:
            XRefreshKeyboardMapping(&(xevent->xmapping));
            break;

        case ClientMessage:
            if ((unsigned int)(xevent->xclient.data.l[0]) == bagX11.WM_DELETE_WINDOW)
                event->type = bagE_EventWindowClose;
            break;

        case Expose: {
            if (xevent->xexpose.count != 0)
                break;

            event->type = bagE_EventWindowResize;
            event->data.windowResize.width = xevent->xexpose.width;
            event->data.windowResize.height = xevent->xexpose.height;
        } break;

        case KeyPress:
        case KeyRelease: {
            KeySym keySym = XkbKeycodeToKeysym(bagX11.display, xevent->xkey.keycode, 0, 0);

            if (keySym != NoSymbol) {
                event->type = xevent->type == KeyPress ? bagE_EventKeyDown : bagE_EventKeyUp;
                event->data.key.modifiers = bagX11_convertModMask(xevent->xkey.state);
                event->data.key.buttons = bagX11_convertButtonMask(xevent->xkey.state);
                event->data.key.key = keySym;
            }

            if (!XFilterEvent(xevent, None)) {
                KeySym retKeySym;
                Status retStatus;
                int retSize = Xutf8LookupString(
                        bagX11.inputClient,
                        &xevent->xkey,
                        (char *)event[1].data.textUTF8.text,
                        BAGE_TEXT_SIZE,
                        &retKeySym,
                        &retStatus
                );

                if (retSize > 0) {
                    events[1].type = bagE_EventTextUTF8;
                    events[2].type = bagE_EventTextUTF32;

                    unsigned int codePoint = bagX11_UTF8ToUTF32(event[1].data.textUTF8.text);

                    if (codePoint != 0)
                        events[2].data.textUTF32.codePoint = codePoint;
                }
            }
        } break;

        case ButtonPress:
        case ButtonRelease: {
            int buttonCode = xevent->xbutton.button;

            if (buttonCode == 4 || buttonCode == 5) {
                if (xevent->type == ButtonRelease)
                    break;

                event->type = bagE_EventMouseWheel;
                event->data.mouseWheel.scrollUp = buttonCode == 4 ? 1 :-1;
                event->data.mouseWheel.modifiers = bagX11_convertModMask(xevent->xbutton.state);
                event->data.mouseWheel.buttons = bagX11_convertButtonMask(xevent->xbutton.state);
                event->data.mouseWheel.x = xevent->xbutton.x;
                event->data.mouseWheel.y = xevent->xbutton.y;
            } else {
                if (buttonCode < 4) {
                    bagE_Button options[] = {
                        bagE_ButtonLeft,
                        bagE_ButtonMiddle,
                        bagE_ButtonRight
                    };
                    event->data.mouseButton.button = options[buttonCode - 1];
                } else
                    event->data.mouseButton.button = buttonCode;

                event->type = xevent->type == ButtonPress ? bagE_EventMouseButtonDown
                                                         : bagE_EventMouseButtonUp;
                event->data.mouseButton.modifiers = bagX11_convertModMask(xevent->xbutton.state);
                event->data.mouseButton.buttons = bagX11_convertButtonMask(xevent->xbutton.state);
                event->data.mouseButton.x = xevent->xbutton.x;
                event->data.mouseButton.y = xevent->xbutton.y;
            }
        } break;

        case MotionNotify:
            event->type = bagE_EventMousePosition;
            event->data.mouse.modifiers = bagX11_convertModMask(xevent->xmotion.state);
            event->data.mouse.buttons = bagX11_convertButtonMask(xevent->xmotion.state);
            event->data.mouse.x = xevent->xmotion.x;
            event->data.mouse.y = xevent->xmotion.y;
            break;

        case GenericEvent:
            if (xevent->xcookie.extension == bagX11.xiOpCode
             && XGetEventData(bagX11.display, &xevent->xcookie)
             && xevent->xcookie.evtype == XI_RawMotion
            ) {
                XIRawEvent* rawEvent = (XIRawEvent*)xevent->xcookie.data;

                if (rawEvent->valuators.mask_len) {
                    event->data.mouseMotion.x = 0;
                    event->data.mouseMotion.y = 0;

                    if (XIMaskIsSet(rawEvent->valuators.mask, 0))
                        event->data.mouseMotion.x = (int)rawEvent->raw_values[0];
                    if (XIMaskIsSet(rawEvent->valuators.mask, 1))
                        event->data.mouseMotion.y = (int)rawEvent->raw_values[1];

                    if (event->data.mouseMotion.x != 0 || event->data.mouseMotion.y != 0)
                        event->type = bagE_EventMouseMotion;
                }
            }
            break;
    }
}


void bagE_pollEvents()
{
    XEvent xevent;
    bagE_Event events[3];

    XEventsQueued(bagX11.display, QueuedAfterFlush);

    while (QLength(bagX11.display)) {
        XNextEvent(bagX11.display, &xevent);

        for (int i = 0; i < 3; i++)
            events[i].type = bagE_EventNone;

        bagX11_translateEvent(events, &xevent);

        for (int i = 0; i < 3; i++) {
            if (events[i].type != bagE_EventNone) {
                int exit = bagE_eventHandler(events+i);// merge

                if (exit)
                    return;
            }
        }
    }

    XFlush(bagX11.display);
}


void bagE_swapBuffers()
{
    glXSwapBuffers(bagX11.display, bagX11.window);
}


void bagE_setSwapInterval(int value)
{
    glXSwapIntervalEXT(bagX11.display, bagX11.window, value);
}


void bagE_setWindowTitle(char *value)
{
    XStoreName(bagX11.display, bagX11.window, value);
}


int bagE_isAdaptiveVsyncAvailable(void)
{
    return bagX11.adaptiveVsync;
}


void bagE_getWindowSize(int *width, int *height)
{
    XWindowAttributes attributes;
    XGetWindowAttributes(bagX11.display, bagX11.window, &attributes);
    *width = attributes.width;
    *height = attributes.height;
}


int bagE_getCursorPosition(int *x, int *y)
{
    Window r, c;
    int rx, ry;
    unsigned int m;
    return XQueryPointer(
            bagX11.display, bagX11.window,
            &r, &c,
            &rx, &ry,
            x, y,
            &m
    );
}


void bagE_setCursorPosition(int x, int y)
{
    XWarpPointer(bagX11.display, None, bagX11.window, 0, 0, 0, 0, x, y);
}


static void bagX11_sendEventToWM(
        Atom type,
        long a, long b, long c,
        long d, long e
) {
    XEvent event = { ClientMessage };
    event.xclient.window = bagX11.window;
    event.xclient.format = 32;
    event.xclient.message_type = type;
    event.xclient.data.l[0] = a;
    event.xclient.data.l[1] = b;
    event.xclient.data.l[2] = c;
    event.xclient.data.l[3] = d;
    event.xclient.data.l[4] = e;

    XSendEvent(
            bagX11.display,
            bagX11.root,
            False,
            SubstructureNotifyMask | SubstructureRedirectMask,
            &event
    );
}


void bagE_setFullscreen(int value)
{
    bagX11_sendEventToWM(
            bagX11._NET_WM_STATE,
            value,
            bagX11._NET_WM_STATE_FULLSCREEN,
            0, 1, 0
    );
}


int bagE_setHiddenCursor(int value)
{
    if (value) {
        int grabResult = XGrabPointer(
                bagX11.display, 
                bagX11.window,
                1,
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
                GrabModeAsync,
                GrabModeAsync,
                bagX11.window,
                None,
                CurrentTime
        );

        if (grabResult != GrabSuccess)
            return 0;

        Cursor invisibleCursor;
        Pixmap bitmapNoData;
        XColor black;
        static char noData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
        black.red = black.green = black.blue = 0;

        bitmapNoData = XCreateBitmapFromData(bagX11.display, bagX11.window, noData, 8, 8);
        invisibleCursor = XCreatePixmapCursor(
                bagX11.display,
                bitmapNoData,
                bitmapNoData,
                &black, &black, 0, 0
        );
        XDefineCursor(bagX11.display, bagX11.window, invisibleCursor);
        XFreeCursor(bagX11.display, invisibleCursor);
        XFreePixmap(bagX11.display, bitmapNoData);
    } else {
        XUngrabPointer(bagX11.display, CurrentTime);
        XUndefineCursor(bagX11.display, bagX11.window);
    }

    return 1;
}

