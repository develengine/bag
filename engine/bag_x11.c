#include "bag_engine_config.h"
#include "bag_engine.h"

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>

#include <GL/glx.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


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


struct bagX11_ProgramStruct
{
    Display *display;
    Window root;
    Window window;
    Atom WM_DELETE_WINDOW;
    GLXContext context;
    int adaptiveVsync;
} bagX11;

typedef struct bagX11_ProgramStruct bagX11_Program;


static int bagX11_ctxErrorOccurred = 0;

static int bagX11_ctxErrorHandler(Display *dpy, XErrorEvent *ev)
{
    bagX11_ctxErrorOccurred = 1;
    return 0;
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
    ||  (glxMajor == 1 && glxMinor < 3)
    ||  (glxMajor < 1)
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

    XSetWindowAttributes setWindowAttrs;
    Colormap colormap;
    setWindowAttrs.colormap = colormap = XCreateColormap(
            bagX11.display, 
            bagX11.root,
            visualInfo->visual,
            AllocNone
    );
    setWindowAttrs.background_pixmap = None;
    setWindowAttrs.border_pixel = 0;
    setWindowAttrs.event_mask = StructureNotifyMask;

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


    /* XInput2 */

    // TODO Skip if mouse motion is not requested
    int xiOpCode, xiEvent, xiError;
    if (!XQueryExtension(bagX11.display, "XInputExtension", &xiOpCode, &xiEvent, &xiError)) {
        fprintf(stderr, "X Input extension not available.\n");
        exit(-1);
    }

    int xiMajor = 2, xiMinor = 0;
    if (XIQueryVersion(bagX11.display, &xiMajor, &xiMinor) == BadRequest) {
        fprintf(stderr, "XI2 not available. Server supports %d.%d\n", xiMajor, xiMinor);
        exit(-1);
    }

    XIEventMask eventMask;
    unsigned char mask[XIMaskLen(XI_RawMotion)] = { 0 };

    eventMask.deviceid = XIAllMasterDevices;
    eventMask.mask_len = sizeof(mask);
    eventMask.mask = mask;
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
    ||  (!glXCreateContextAttribsARB)
    ) {
        fprintf(stderr, "'glXCreateContextARB()' wasn't found! Using old OpenGL context.\n");
    }

    glXSwapIntervalEXT = (GLXSWAPINTERVALEXTPROC)
        glXGetProcAddress((const GLubyte *)"glXSwapIntervalEXT");

    if ((!bagX11_isGLXExtensionSupported(glxExtensions, "GLX_EXT_swap_control"))
    ||  (!glXCreateContextAttribsARB)
    ) {
        fprintf(stderr, "'glXSwapIntervalEXT' couldn't be retrieved!\n");
    }


    bagX11.adaptiveVsync = bagX11_isGLXExtensionSupported(
            glxExtensions,
            "GLX_EXT_swap_control_tear"
    );

#ifdef BAGE_PRINT_DEBUG_INFO
    if (bagX11.adaptiveVsync) {
        printf("Adaptive Vsync is supported.\n");
    } else {
        printf("Adaptive Vsync isn't supported.\n");
    }
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


void bagE_pollEvents()
{
    XEvent xevent;
    xevent.type = bagE_EventNone;
    bagE_Event event;

    XEventsQueued(bagX11.display, QueuedAfterFlush);

    while (QLength(bagX11.display)) {
        XNextEvent(bagX11.display, &xevent);

        switch (xevent.type)
        {
            case ClientMessage: {
                if ((unsigned int)(xevent.xclient.data.l[0]) == bagX11.WM_DELETE_WINDOW) {
                    event.type = bagE_EventWindowClose;
                }
            } break;

            case Expose: {
                XExposeEvent *exposeEvent = (XExposeEvent*)&xevent;

                if (exposeEvent->count == 0) {
                    event.type = bagE_EventWindowResize;
                    event.data.windowResize.width = exposeEvent->width;
                    event.data.windowResize.height = exposeEvent->height;
                }
            } break;
        }
        
        if (event.type == bagE_EventNone) {
            continue;
        }

        bagE_eventHandler(&event);
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

