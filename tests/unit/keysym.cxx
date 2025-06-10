#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtest/gtest.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static KeyCode lookup(Display *dpy, KeySym sym, unsigned mods)
{
    XkbDescPtr xkb = XkbGetMap(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
    if (!xkb)
        return 0;

    for (unsigned code = xkb->min_key_code; code <= xkb->max_key_code; ++code) {
        KeySym ks; unsigned out;
        XkbTranslateKeyCode(xkb, code, mods, &out, &ks);
        if (ks == sym) {
            XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);
            return code;
        }
    }

    XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);
    return 0;
}

static KeyCode searchKeycode(Display *dpy, KeySym sym)
{
    XkbStateRec state;
    XkbGetState(dpy, XkbUseCoreKbd, &state);
    unsigned mods = XkbBuildCoreState(XkbStateMods(&state), state.group);

    KeyCode kc = lookup(dpy, sym, mods);
    if (!kc)
        kc = lookup(dpy, sym, mods ^ ShiftMask);

    unsigned levelMask = XkbKeysymToModifiers(dpy, XK_ISO_Level3_Shift);
    if (!levelMask)
        levelMask = XkbKeysymToModifiers(dpy, XK_Mode_switch);

    if (!kc && levelMask)
        kc = lookup(dpy, sym, mods ^ levelMask);
    if (!kc && levelMask)
        kc = lookup(dpy, sym, mods ^ ShiftMask ^ levelMask);

    return kc;
}

class XvfbTest : public ::testing::Test {
protected:
    pid_t pid = 0;
    Display *dpy = nullptr;

    void SetUp() override {
        pid = fork();
        if (pid == 0) {
            execlp("Xvfb", "Xvfb", ":99", "-screen", "0", "640x480x24", nullptr);
            _exit(1);
        }
        sleep(1);
        setenv("DISPLAY", ":99", 1);
        dpy = XOpenDisplay(nullptr);
        ASSERT_NE(dpy, nullptr);
    }

    void TearDown() override {
        if (dpy)
            XCloseDisplay(dpy);
        if (pid) {
            kill(pid, SIGTERM);
            waitpid(pid, nullptr, 0);
        }
    }
};

TEST_F(XvfbTest, ShiftedKeysym)
{
    KeyCode kc = searchKeycode(dpy, XK_exclam); // '!'
    EXPECT_NE(kc, 0);
    EXPECT_EQ(kc, XKeysymToKeycode(dpy, XK_1));
}

