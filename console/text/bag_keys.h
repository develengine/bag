#ifndef BAG_KEYS_H
#define BAG_KEYS_H

/* TODO
 * [ ] convert defines to hex and remove includes
 * [ ] add support for right modifier keys on windows
 */

#ifdef _WIN32
#include <windows.h>
#define KEY_PAIR(w, x) w
#else
#include <X11/Xutil.h>
#define KEY_PAIR(w, x) x
#endif

#define KEY_BACK          KEY_PAIR(VK_BACK,       XK_BackSpace)
#define KEY_TAB           KEY_PAIR(VK_TAB,        XK_Tab)
#define KEY_RETURN        KEY_PAIR(VK_RETURN,     XK_Return)
#define KEY_CAPS_LOCK     KEY_PAIR(VK_CAPITAL,    XK_Caps_Lock)
#define KEY_ESCAPE        KEY_PAIR(VK_ESCAPE,     XK_Escape)
#define KEY_HOME          KEY_PAIR(VK_HOME,       XK_Home)
#define KEY_END           KEY_PAIR(VK_END,        XK_End)
#define KEY_INSERT        KEY_PAIR(VK_INSERT,     XK_Insert)
#define KEY_DELETE        KEY_PAIR(VK_DELETE,     XK_Delete)
#define KEY_UP            KEY_PAIR(VK_UP,         XK_Up)
#define KEY_DOWN          KEY_PAIR(VK_DOWN,       XK_Down)
#define KEY_LEFT          KEY_PAIR(VK_LEFT,       XK_Left)
#define KEY_RIGHT         KEY_PAIR(VK_RIGHT,      XK_Right)
#define KEY_PAGE_UP       KEY_PAIR(VK_PRIOR,      XK_Page_Up)
#define KEY_PAGE_DOWN     KEY_PAIR(VK_NEXT,       XK_Page_Down)
#define KEY_SUPER_LEFT    KEY_PAIR(VK_LWIN,       XK_Super_L)
#define KEY_SUPER_RIGHT   KEY_PAIR(VK_RWIN,       XK_Super_R)

/* ON WINDOWS BOTH ARE THE SAME */
#define KEY_SHIFT_LEFT    KEY_PAIR(VK_SHIFT,      XK_Shift_L)
#define KEY_SHIFT_RIGHT   KEY_PAIR(VK_SHIFT,      XK_Shift_R)
#define KEY_CONTROL_LEFT  KEY_PAIR(VK_CONTROL,    XK_Control_L)
#define KEY_CONTROL_RIGHT KEY_PAIR(VK_CONTROL,    XK_Control_R)
#define KEY_ALT_LEFT      KEY_PAIR(VK_MENU,       XK_Alt_L)
#define KEY_ALT_RIGHT     KEY_PAIR(VK_MENU,       XK_Alt_R)

#define KEY_F1            KEY_PAIR(VK_F1,         XK_F1)
#define KEY_F2            KEY_PAIR(VK_F2,         XK_F2)
#define KEY_F3            KEY_PAIR(VK_F3,         XK_F3)
#define KEY_F4            KEY_PAIR(VK_F4,         XK_F4)
#define KEY_F5            KEY_PAIR(VK_F5,         XK_F5)
#define KEY_F6            KEY_PAIR(VK_F6,         XK_F6)
#define KEY_F7            KEY_PAIR(VK_F7,         XK_F7)
#define KEY_F8            KEY_PAIR(VK_F8,         XK_F8)
#define KEY_F9            KEY_PAIR(VK_F9,         XK_F9)
#define KEY_F10           KEY_PAIR(VK_F10,        XK_F10)
#define KEY_F11           KEY_PAIR(VK_F11,        XK_F11)
#define KEY_F12           KEY_PAIR(VK_F12,        XK_F12)

#define KEY_GRAVE         KEY_PAIR(VK_OEM_3,      XK_grave)
#define KEY_MINUS         KEY_PAIR(VK_OEM_MINUS,  XK_minus)
#define KEY_EQUALS        KEY_PAIR(VK_OEM_PLUS,   XK_equal)
#define KEY_BRACKET_LEFT  KEY_PAIR(VK_OEM_4,      XK_bracketleft)
#define KEY_BRACKET_RIGHT KEY_PAIR(VK_OEM_6,      XK_bracketright)
#define KEY_BACKSLASH     KEY_PAIR(VK_OEM_5,      XK_backslash)
#define KEY_SEMICOLON     KEY_PAIR(VK_OEM_1,      XK_semicolon)
#define KEY_APOSTROPHE    KEY_PAIR(VK_OEM_7,      XK_apostrophe)
#define KEY_COMMA         KEY_PAIR(VK_OEM_COMMA,  XK_comma)
#define KEY_PERIOD        KEY_PAIR(VK_OEM_PERIOD, XK_period)
#define KEY_SLASH         KEY_PAIR(VK_OEM_2,      XK_slash)
#define KEY_SPACE         KEY_PAIR(VK_SPACE,      XK_space)

#define KEY_0             KEY_PAIR('0',           XK_0)
#define KEY_1             KEY_PAIR('1',           XK_1)
#define KEY_2             KEY_PAIR('2',           XK_2)
#define KEY_3             KEY_PAIR('3',           XK_3)
#define KEY_4             KEY_PAIR('4',           XK_4)
#define KEY_5             KEY_PAIR('5',           XK_5)
#define KEY_6             KEY_PAIR('6',           XK_6)
#define KEY_7             KEY_PAIR('7',           XK_7)
#define KEY_8             KEY_PAIR('8',           XK_8)
#define KEY_9             KEY_PAIR('9',           XK_9)

#define KEY_A             KEY_PAIR('A',           XK_a)
#define KEY_B             KEY_PAIR('B',           XK_b)
#define KEY_C             KEY_PAIR('C',           XK_c)
#define KEY_D             KEY_PAIR('D',           XK_d)
#define KEY_E             KEY_PAIR('E',           XK_e)
#define KEY_F             KEY_PAIR('F',           XK_f)
#define KEY_G             KEY_PAIR('G',           XK_g)
#define KEY_H             KEY_PAIR('H',           XK_h)
#define KEY_I             KEY_PAIR('I',           XK_i)
#define KEY_J             KEY_PAIR('J',           XK_j)
#define KEY_K             KEY_PAIR('K',           XK_k)
#define KEY_L             KEY_PAIR('L',           XK_l)
#define KEY_M             KEY_PAIR('M',           XK_m)
#define KEY_N             KEY_PAIR('N',           XK_n)
#define KEY_O             KEY_PAIR('O',           XK_o)
#define KEY_P             KEY_PAIR('P',           XK_p)
#define KEY_Q             KEY_PAIR('Q',           XK_q)
#define KEY_R             KEY_PAIR('R',           XK_r)
#define KEY_S             KEY_PAIR('S',           XK_s)
#define KEY_T             KEY_PAIR('T',           XK_t)
#define KEY_U             KEY_PAIR('U',           XK_u)
#define KEY_V             KEY_PAIR('V',           XK_v)
#define KEY_W             KEY_PAIR('W',           XK_w)
#define KEY_X             KEY_PAIR('X',           XK_x)
#define KEY_Y             KEY_PAIR('Y',           XK_y)
#define KEY_Z             KEY_PAIR('Z',           XK_z)

#endif
