// header file of library "arplib.h"

#ifndef ARPLIB_H
#define ARPLIB_H

// Headers of functions

int spawn(const char *program, char **arg_list);

void writeLog(const char *format, ...);

int sign(int x);


#endif //ARPLIB_H