TEST_NAME=test.out

IDIR =./include
SRCDIR =./src
ODIR=./obj
TDIR = ./test

CC=gcc
CFLAGS=-I$(IDIR) -g3 -Wall -fpic
LIBS=-lcommons -lpthread -lreadline -lrt

DEPS = $(wildcard $(IDIR)/*.h)
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

TEST_SRC = $(wildcard $(TDIR)/*.c)
TEST_OBJ = $(patsubst $(TDIR)/%.c, $(ODIR)/%.o, $(TEST_SRC))

$(TEST_NAME): $(OBJ) $(TEST_OBJ)
	$(CC) -o $(TEST_NAME) $^ $(CFLAGS) $(LIBS) -lcspecs

$(OBJ): $(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(TEST_OBJ): $(ODIR)/%.o: $(TDIR)/%.c $(DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $@

.PHONY: objs clean test

test: $(TEST_NAME)
	./$(TEST_NAME)

objs: $(OBJ)

clean:
	rm -f $(ODIR)/*.o $(TEST_NAME)