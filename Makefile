CC = gcc
LD = gcc
OPT = -O3 -ffast-math -pthread -fopenmp
#PFLAGS = -pg	# separated because it messes with valgrind
#DBFLAGS = -g -fopt-info-optimized-missed-vec -fopt-info-optimized-missed-loop
CFLAGS = -Wall -g $(OPT) $(DBFLAGS) $(PFLAGS)
ASMFLAGS = -fverbose-asm
#INCLUDES = -I/opt/X11/include
#LDFLAGS = $(PFLAGS) -L/opt/X11/lib -lX11 -lm -lpthread
LDFLAGS = $(PFLAGS) -lm -lpthread

RM = rm -f
OBJS = main.o parareal.o funcs.o aux.o
NAME = main

main: $(OBJS) outdata
	$(LD) $(OPT) $(OBJS) $(LDFLAGS) -o $@

main.o: main.c funcs.h parareal.h aux.h
	$(CC) $(CFLAGS) -c main.c

funcs.o: funcs.h parareal.h aux.h funcs.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c funcs.c

aux.o: aux.c
	$(CC) $(CFLAGS) -c aux.c

parareal.o: parareal.h funcs.h aux.h parareal.c
	$(CC) $(CFLAGS) $(LDFLAGS) -c parareal.c

outdata:
	mkdir outdata

debug%:
	# only change flag if valid call
	if [ ! -z $* ]; then \
		echo "debug = $*"; \
		sed -i "/#define DEBUGGING/ s/[0-9][0-9]*/$*/" aux.h; \
	fi

# produce assembler output
%.s: %.c
	$(CC) $(CFLAGS) -S $(ASMFLAGS) $< -o "$*.s"

clean:
	$(RM) $(NAME) $(OBJS)
