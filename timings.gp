reset

print "script             : ", ARG0
print "Ploting file       : ", ARG1
print "plot OMP too       : ", ARG2
print "number of arguments: ", ARGC

work=ARG1
workomp=ARG1."omp"

#set yrange [-0.5:8.5]
set autoscale xfixmin   # axis range automatically scaled to 
                        # include the range
set autoscale xfixmax   # of data to be plotted


set lmargin at screen 0.1
set rmargin at screen 0.82
set bmargin at screen 0.12
set tmargin at screen 0.95

set xlabel "time (s)"
set ylabel "thread id"

# Set linestyle 1 to blue (#0060ad)
set style line 1 \
    linecolor rgb '#0060ad' \
    linetype 1 linewidth 4 \
    pointtype 6 pointsize 0.5

set key outside
set format x "%2.1f"
set xtics  0,0.5,32
set ytics  0,1,32


#TODO plot pure lines instead of lines with point ends
do for [i=1:100000] {
    set multiplot layout 1,2
    plot work with linespoints linestyle 1
    replot
    if (ARGC == 3) {
        plot workomp with linespoints linestyle 1
    }
    pause mouse keypress
    if (MOUSE_CHAR eq 'q') {
        break
    }
    unset multiplot
}
