#include "aux.h"

double g_tic = 0;   // global time reference point
extern double g_tic;

void write2file(double start, double h, int size, double *yt, int id) {
    char fname[128];
    sprintf(fname, "./outdata/data.%d.out", id);
    FILE* fp = fopen(fname, "a");
    assert(fp && "Open failed\n");

    for(int i=0; i<size; i++) {
        fprintf(fp, "%.4f %.4f\n", start+i*h, yt[i]);
    }
    fclose(fp);
}

void gnuplot() {
    pid_t pid = fork();
    if (!pid) {
        execlp("gnuplot", "-p", "plot.gp", (char*) NULL);
        perror("exec failed");
        exit(-1);
    }
}
