reset                   # force all graph-related options to default values
fname = "outdata/data.out"
set xrange [-0.1:1.1]
set autoscale yfixmin   # axis range automatically scaled to 
                        # include the range
set autoscale yfixmax   # of data to be plotted

#set tics font ",18"
#set xlabel "x" font ",18"
#set ylabel "y" font ",18"

set lmargin at screen 0.1
set rmargin at screen 0.82
set bmargin at screen 0.12
set tmargin at screen 0.95

# Set linestyle 1 to blue (#0060ad)
set style line 1 \
    linecolor rgb '#0060ad' \
    linetype 1 linewidth 4 \
    pointtype 6 pointsize 0.5

do for [i=1:100000] {
    plot fname using 1:2 linestyle 1
    replot exp(x)
    pause mouse keypress
    if (MOUSE_CHAR eq 'q') {
        break
    }
}
