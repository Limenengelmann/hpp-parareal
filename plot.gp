reset                   # force all graph-related options to default values
fname = "outdata/data.out"
set autoscale xfixmin   # axis range automatically scaled to 
                        # include the range
set autoscale xfixmax   # of data to be plotted

set tics font ",18"
set xlabel "x" font ",18"
set ylabel "y" font ",18"

set lmargin at screen 0.1
set rmargin at screen 0.82
set bmargin at screen 0.12
set tmargin at screen 0.95

do for [i=1:100000] {
    plot fname using 1:2
    replot exp(x)
    pause mouse keypress
    if (MOUSE_CHAR eq 'q') {
        break
    }
}
