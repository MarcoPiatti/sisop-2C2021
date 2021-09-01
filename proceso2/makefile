PROCESS_NAME=proceso2.out

IDIR =./include
SRCDIR =./src
ODIR=./obj
SHARED_IDIR = ../shared/include
SHARED_SRCDIR = ../shared/src
SHARED_ODIR = ../shared/obj

CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g -Wall
LIBS=-lcommons -lpthread -lreadline -lcunit -lrt

DEPS = $(wildcard $(IDIR)/*.h)
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

SHARED_DEPS = $(wildcard $(SHARED_IDIR)/*.h)
SHARED_SRC = $(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ = $(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

$(PROCESS_NAME): $(OBJ) $(SHARED_OBJ)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

$(ODIR)/%.o: $(SRCDIR)/%.c $(DEPS) $(SHARED_DEPS) | $(ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(ODIR):
	mkdir -p $@

$(SHARED_ODIR)/%.o: $(SHARED_SRCDIR)/%.c $(SHARED_DEPS) | $(SHARED_ODIR)
	$(CC) -c -o $@ $< $(CFLAGS)

$(SHARED_ODIR):
	mkdir -p $@

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(IDIR)/*~ $(PROCESS_NAME) $(SHARED_ODIR)/*.o 