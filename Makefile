CC = gcc
LD = gcc
OPT = -O3 -ffast-math -pthread
#PFLAGS = -pg	# separated because it messes with valgrind
#DBFLAGS = -g -fopt-info-optimized-missed-vec -fopt-info-optimized-missed-loop
CFLAGS = -Wall -g $(OPT) $(DBFLAGS) $(PFLAGS)
ASMFLAGS = -fverbose-asm
#INCLUDES = -I/opt/X11/include
#LDFLAGS = $(PFLAGS) -L/opt/X11/lib -lX11 -lm -lpthread
LDFLAGS = $(PFLAGS) -lm -lpthread

RM = rm -f
OBJS = main.o parareal.o tests.o plot.o
NAME = main

main: $(OBJS)
	$(LD) $(OPT) $(OBJS) $(LDFLAGS) -o $@

main.o: main.c tests.h parareal.h plot.h
	$(CC) $(CFLAGS) -c main.c

tests.o: tests.h parareal.h tests.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c tests.c

plot.o: plot.c
	$(CC) $(CFLAGS) -c plot.c

parareal.o: parareal.h tests.h parareal.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c parareal.c

debug%:
	# only change flag if valid call
	if [ ! -z $* ]; then \
		echo "debug = $*"; \
		sed -i "/#define DEBUGGING/ s/[0-9][0-9]*/$*/" tests.h; \
	fi

# produce assembler output
%.s: %.c
	$(CC) $(CFLAGS) -S $(ASMFLAGS) $< -o "$*.s"

clean:
	$(RM) $(NAME) $(OBJS)
