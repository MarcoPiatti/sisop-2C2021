PROCESS_NAME=libmatelib.so
TEST_NAME=test.out

IDIR =./include
SRCDIR =./src
ODIR=./obj
TDIR = ./test

SHARED_IDIR = ../shared/include
SHARED_SRCDIR = ../shared/src
SHARED_ODIR = ../shared/obj

CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g3 -Wall -fpic
LIBS=-lcommons -lpthread -lreadline

DEPS = $(wildcard $(IDIR)/*.h)
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

SHARED_DEPS = $(wildcard $(SHARED_IDIR)/*.h)
SHARED_SRC = $(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ = $(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

TEST_SRC = $(wildcard $(TDIR)/*.c)
TEST_OBJ = $(patsubst $(TDIR)/%.c, $(ODIR)/%.o, $(TEST_SRC))

$(PROCESS_NAME): $(OBJ) $(SHARED_OBJ)
	$(CC) -shared -o $@ $^ $(CFLAGS) $(LIBS)

$(TEST_NAME): $(OBJ) $(TEST_OBJ) $(SHARED_OBJ)
	$(CC) -o $(TEST_NAME) $^ $(CFLAGS) $(LIBS) -lcspecs

$(OBJ): $(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS) $(SHARED_DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TEST_OBJ): $(ODIR)/%.o: $(TDIR)/%.c $(DEPS) $(SHARED_DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $@

$(SHARED_OBJ):
	cd ../shared && make objs

.PHONY: clean test

test: $(TEST_NAME)
	./$(TEST_NAME)

clean:
	rm -f $(ODIR)/*.o $(PROCESS_NAME) $(TEST_NAME)