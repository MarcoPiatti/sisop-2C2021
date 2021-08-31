PROCESS_NAME=proceso3
IDIR =./include
SHARED_IDIR = ../shared/include
CC=gcc
CFLAGS=-I$(IDIR) -I$(SHARED_IDIR) -g -Wall

ODIR=./obj
SRCDIR =./src
LIBS=-lcommons -lpthread -lreadline -lcunit -lrt

SHARED_DEPS = $(wildcard $(SHARED_IDIR)/*.h)

SHARED_SRCDIR = ../shared/src
SHARED_ODIR = ../shared/obj

SHARED_SRC = $(wildcard $(SHARED_SRCDIR)/*.c)
SHARED_OBJ = $(patsubst $(SHARED_SRCDIR)/%.c,$(SHARED_ODIR)/%.o,$(SHARED_SRC))

DEPS = $(wildcard $(IDIR)/*.h)

SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(patsubst $(SRCDIR)/%.c,$(ODIR)/%.o,$(SRC))

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