#include "parareal.h"

double fw_euler_step(double t, double x_t, double h, double (*f) (double, double)) {
    return x_t + h*f(t, x_t);
}

double rk4_step(double t, double x_t, double h, double (*f) (double, double)) {
    double k1 = f(t, x_t);
    double k2 = f(t+h/2, x_t + h*k1/2);
    double k3 = f(t+h/2, x_t + h*k2/2);
    double k4 = f(t+h, x_t+h*k3);
    return x_t + h/6*(k1 + 2*k2 + 2*k3 + k4);
}

// TODO proper malloc/free symmetry by passing buffers as arguments
double* parareal(double start, double end, int ncoarse, int nfine,
        double x_0,
        double (*coarse) (double, double, double, double (*f) (double, double)),
        double (*fine) (double, double, double, double (*f) (double, double)),
        double (*f) (double, double) ) {

    double hc = (end-start)/ncoarse;
    double hf = hc/nfine;
    // +1 to account for x_0
    double *x_t_old  = (double*) malloc((ncoarse+1)*sizeof(double));
    double *x_t_new  = (double*) malloc((ncoarse+1)*sizeof(double));
    double *x_t_fine = (double*) malloc((ncoarse+1)*sizeof(double));
    x_t_old[0] = x_0;
    x_t_new[0] = x_0;
    x_t_fine[0] = x_0;

    gnuplot();

    double t = start;
    // Initial approximation
    for (int i=0; i<ncoarse; i++) {
        x_t_old[i+1] = coarse(t, x_t_old[i], hc, f);
        t += hc;
    }

    // parareal iteration
    for (int K=0; K<ncoarse; K++) {
        // parallel fine steps
        t = start;
        double x_t;
        for (int i=0; i<ncoarse-1; i++) {
            x_t = x_t_old[i];
            for (int j=0; j<nfine; j++) {
                //DEBUG(true, assert(t+hf <= start + i*hc && "Step out of bound!"));
                x_t = fine(t, x_t, hf, f);
                t += hf;
            }
            x_t_fine[i+1] = x_t;
        }
        write2file(start, hc, ncoarse, x_t_fine);

        // corrections and sequential coarse steps
        // TODO currently limited to 1 step for coarse update
        t = start;
        for (int i=0; i<ncoarse; i++) {
            x_t_new[i+1] = x_t_new[i] + x_t_fine[i] - x_t_old[i];
            // coarse step
            x_t_new[i+1] = coarse(t, x_t_new[i], hc, f);
            t += hc;
        }

        // new -> old
        memcpy(x_t_old+1, x_t_new+1, ncoarse*sizeof(double));
    }
    free(x_t_old );
    free(x_t_fine);

    return x_t_new;
}
