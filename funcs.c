#include "funcs.h"

inline double fw_euler_step(double t, double y_t, double h, rhs_func f) {
    DEBUG(DBRNDSLEEP_FWE, SLEEPTIME((t+0.5)*2));
    return y_t + h*f(t, y_t);
}

inline double rk4_step(double t, double y_t, double h, rhs_func f) {
    double k1 = f(t, y_t);
    double k2 = f(t+h/2, y_t + h*k1/2);
    double k3 = f(t+h/2, y_t + h*k2/2);
    double k4 = f(t+h  , y_t + h*k3);
    DEBUG(DBRNDSLEEP_RK4, SLEEPTIME((t+0.5)*8));
    return y_t + h/6*(k1 + 2*k2 + 2*k3 + k4);
}

double f_cos(double t, double x_t) {
    return cos(t);
}

double f_id(double t, double x_t) {
    usleep(F_WORK);
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

int run_tests() {
    if (! test_singlestep_integrator(fw_euler_step, 1e4)) {
        DPRINTF(true, "fw_euler_step failed\n");
        return -1;
    }
    if (! test_singlestep_integrator(rk4_step, 1e3)) {
        DPRINTF(true, "rk4_step failed\n");
        return -1;
    }
    return 0;
}
