#include "tests.h"

double f_cos(double t, double x_t) {
    return cos(t);
}

double f_id(double t, double x_t) {
    usleep(300);
    return x_t;
}

static int test_singlestep_integrator(singlestep_func s, double tol) {
    double h = 1e-5;
    int nsteps = 1e5;
    h = 1e-1;
    nsteps = 1e1;

    double x_t = 1;
    double t = 0;
    for (int i=0; i<nsteps; i++) {
        x_t = s(t, x_t, h, f_id);
        t += h;
    }

    double err = fabs(x_t - exp(t));
    DPRINTF(DBTESTS, "Error: %.2e\n", err);
    return err <= tol;
}

// TODO depends too much on hc and doesnt seem to converge to fine integr precision
static int test_parareal(singlestep_func coarse, singlestep_func fine, double tol) {

    double hc = 1e-1;
    double hf = 1e-1;

    double x_t = 1;
    double t = 0;
    double t_end = 1;

    int ncoarse = (t_end - t) / hc;
    assert(fabs(ncoarse*hc - (t_end-t)) < 1e-15 && "Interval not dividable by coarse stepsize");
    int nfine = (hc) / hf;
    assert(fabs(nfine*hf - hc) < 1e-15 && "Interval not dividable by fine stepsize");

    double* x_res = parareal(t, t_end, ncoarse, nfine, 4, x_t, coarse, fine, f_id);

    //write2file(t, hc, ncoarse+1, x_res);
    //gnuplot();

    double l2err = 0;
    double tmp;
    t = 0;
    for (int i=0; i<ncoarse; i++) {
        tmp = fabs(x_res[i] - exp(t));
        l2err += tmp*tmp;
        t+=hc;
    }

    l2err = sqrt(l2err);
    printf("l2error: %.2e, last step: %.2e (res: %f, sol: %f\n", l2err, tmp, x_res[ncoarse], exp(t_end));
    free(x_res);
    return l2err <= tol;
}

int run_tests() {
    if (! test_singlestep_integrator(fw_euler_step, 1e4)) {
        DPRINTF(true, "fw_euler_step failed\n");
        return -1;
    }
    if (! test_singlestep_integrator(rk4_step, 1e3)) {
        DPRINTF(true, "rk4_step failed\n");
        return -1;
    }
    if (! test_parareal(fw_euler_step, rk4_step, 1e3)) {
        DPRINTF(true, "Parareal failed\n");
        return -1;
    }
    return 0;
}
