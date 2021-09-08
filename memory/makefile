PROCESS_NAME=memory.out
TEST_NAME=test.out

IDIR =./include
SRCDIR =./src
ODIR=./obj
TDIR = ./test

SHARED_IDIR = ../shared/include
SHARED_SRCDIR = ../shared/src
SHARED_ODIR = ../shared/obj

CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g -Wall
LIBS=-lcommons -lpthread -lreadline -lrt

DEPS = $(wildcard $(IDIR)/*.h)
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

SHARED_DEPS = $(wildcard $(SHARED_IDIR)/*.h)
SHARED_SRC = $(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ = $(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

TEST_SRC = $(wildcard $(TDIR)/*.c)
TEST_OBJ = $(patsubst $(TDIR)/%.c, $(ODIR)/%.o, $(TEST_SRC))

$(PROCESS_NAME): $(OBJ) $(SHARED_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS) $(SHARED_DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR)/%.o: $(TDIR)/%.c $(DEPS) $(SHARED_DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $@

$(SHARED_ODIR)/%.o: $(SHARED_SRCDIR)/%.c $(SHARED_DEPS) | $(SHARED_ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(SHARED_ODIR):
	mkdir -p $@

.PHONY: clean run val hel test

test: $(OBJ) $(SHARED_OBJ) $(TEST_OBJ)
	$(CC) -o $(TEST_NAME) $^ $(CFLAGS) $(LIBS) -lcspecs && \
	rm -f ./test/*.o && \
	./$(TEST_NAME)

run:
	make && \
	./$(PROCESS_NAME)

hel:
	make && \
	valgrind --tool=helgrind ./$(PROCESS_NAME)

val:
	make && \
	valgrind --tool=memcheck --leak-check=yes --show-possibly-lost=no --show-reachable=no --num-callers=20 ./$(PROCESS_NAME)

clean:
	rm -f $(ODIR)/*.o $(PROCESS_NAME) $(SHARED_ODIR)/*.o 