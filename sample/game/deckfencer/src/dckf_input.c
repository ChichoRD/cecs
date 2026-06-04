#include <cecs_os.h>
#include "dckf_input.h"

#if CECS_OS_MASK & CECS_OS_MASK_WINDOWS
#include <windows.h>
#include <conio.h>

#elif CECS_OS_MASK & CECS_OS_MASK_POSIX
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#include <stdlib.h>
#include <string.h>

#else
#error "unsupported platform"

#endif


// Source - https://stackoverflow.com/a/448982
// Posted by Alnitak, modified by community. See post 'Timeline' for change history
// Retrieved 2026-05-13, License - CC BY-SA 4.0

#if CECS_OS_MASK & CECS_OS_MASK_POSIX
struct termios orig_termios;
#endif

void dckf_reset_terminal_mode(void) {
#if CECS_OS_MASK & CECS_OS_MASK_POSIX
    tcsetattr(0, TCSANOW, &orig_termios);
#endif
}
void dckf_set_conio_terminal_mode(void) {
#if CECS_OS_MASK & CECS_OS_MASK_POSIX
    struct termios new_termios;

    /* take two copies - one for now, one for later */
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));

    /* register cleanup handler, and set the new terminal mode */
    atexit(dckf_reset_terminal_mode);
    cfmakeraw(&new_termios);
    tcsetattr(0, TCSANOW, &new_termios);
#endif
}

int dckf_kbhit(void) {
#if CECS_OS_MASK & CECS_OS_MASK_POSIX
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv) > 0;
#elif CECS_OS_MASK & CECS_OS_MASK_WINDOWS
    return _kbhit();
#endif
}
int dckf_getch(void) {
#if CECS_OS_MASK & CECS_OS_MASK_POSIX
    int r;
    unsigned char c;
    if ((r = read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
#elif CECS_OS_MASK & CECS_OS_MASK_WINDOWS
    return _getch();
#endif
}
