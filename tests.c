#include "tests.h"

static double f_cos(double t, double x_t) {
    return cos(t);
}

static double f_id(double t, double x_t) {
    return x_t;
}

static int test_singlestep_integrator(double (*s) (double, double, double, double (*f) (double, double)), double tol) {
    double h = 1e-5;
    int nsteps = 1e5;

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
static int test_parareal(
        double (*coarse) (double, double, double, double (*f) (double, double)),
        double (*fine) (double, double, double, double (*f) (double, double)),
        double tol) {

    double hc = 1e-1;
    double hf = 1e-5;

    double x_t = 1;
    double t = 0;
    double t_end = 1;

    int ncoarse = (t_end - t) / hc;
    assert(fabs(ncoarse*hc - (t_end-t)) < 1e-15 && "Interval not dividable by coarse stepsize");
    int nfine = (hc) / hf;
    assert(fabs(nfine*hf - hc) < 1e-15 && "Interval not dividable by fine stepsize");

    double* x_res = parareal(t, t_end, ncoarse, nfine, x_t, coarse, fine, f_id);
    gnuplot(t, hc, ncoarse, x_res);

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
    if (! test_singlestep_integrator(fw_euler_step, 1e-4)) {
        DPRINTF(DBTESTS, "fw_euler_step failed\n");
        return -1;
    }
    if (! test_singlestep_integrator(rk4_step, 1e-3)) {
        DPRINTF(DBTESTS, "rk4_step failed\n");
        return -1;
    }
    if (! test_parareal(fw_euler_step, rk4_step, 1e-3)) {
        DPRINTF(DBTESTS, "Parareal failed\n");
        return -1;
    }
    return 0;
}
