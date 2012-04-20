/*
 * Copyright (c) 2012 Martin Rödel aka Yomin
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <termios.h>

/*
 * ~ICANON - noncanocial mode - input immediately available
 * ~ECHO   - no echo
 * TCSANOW - apply changes immediately
 * VMIN    - minimum characters to read
 * VTIME   - timeout
 */

int getch() {
    int c = -1, fd = 0;
    struct termios new, old;
    fd = fileno(stdin);
    tcgetattr(fd, &old);
    new = old;
    new.c_lflag &= ~(ICANON|ECHO);
    tcsetattr(fd, TCSANOW, &new);
    c = getchar();
    tcsetattr(fd, TCSANOW, &old);
    return c;
}

int kbhit() {
    struct termios new, old;
    int c = -1, fd = 0;
    fd = fileno(stdin);
    tcgetattr(fd, &old);
    new = old;
    new.c_lflag &= ~ICANON;
    new.c_cc[VMIN] = 0;
    new.c_cc[VTIME] = 1;
    tcsetattr(fd, TCSANOW, &new);
    c = getchar();
    tcsetattr(fd, TCSANOW, &old);
    if(c != -1)
    {
        ungetc(c, stdin);
        return 1;
    }
    else
        return 0;
}

