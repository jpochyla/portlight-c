#ifndef INCLUDE_UTILS
#define INCLUDE_UTILS

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static void error_and_exit(const char *msg) {
    fputs(msg, stderr);
    fputs("\n", stderr);
}

static void expect(bool condition, const char *msg) {
    if (!condition) {
        error_and_exit(msg);
        exit(1);
    }
}

#endif
