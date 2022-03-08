#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "tests.h"

int main(int argc, char** argv) {

#if DBMAIN_TESTS
    if (run_tests()) {
        printf("Tests failed.\n");
        return -1;
    }
#endif

    return 0;
}
