#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "kernel.h"

void keyboard_init(void);
char keyboard_getc(void);
int  keyboard_has_char(void);

#endif
