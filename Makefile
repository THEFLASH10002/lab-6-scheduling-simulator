TASK1_SRC	:= schedsim.c util.c
EXE		:= schedsim

all: $(EXE)

$(EXE): $(TASK1_SRC) process.h util.h
	gcc -Wall -Wextra -std=c11 -g $(TASK1_SRC) -o $(EXE)

clean:
	rm -f $(EXE)
