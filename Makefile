     CC := clang
CCFLAGS := -O0 -Wall
DBFLAGS := -g

TARGETS := run_solver generate
  MAINS := $(addsuffix .o, $(TARGETS))
    OBJ := board_util.o netcode.o run_solver.o solve.o
   DEPS := board_util.h netcode.h solve.h
   LIBS := -lcurl -lc

all: $(TARGETS)

clean:
	rm -f $(TARGETS) $(OBJ)

# If any of the source files or headers are newer than the output files
$(OBJ): %.o : %.c $(DEPS)
	$(CC) -c -o $@ $< $(CCFLAGS) $(DBFLAGS)


$(TARGETS): % : $(filter-out $(MAINS), $(OBJ)) %.o
	$(CC) -o $@ $^ $(LIBS) $(CCFLAGS) $(DBFLAGS) $(LDFLAGS) 

