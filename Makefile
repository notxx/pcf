OBJDIR = build
CC = gcc
LDFLAGS =
CFLAGS = -Wall -std=gnu99
SOURCES = $(wildcard *.c)
OBJS = $(patsubst %.c, $(OBJDIR)/%.o, $(SOURCES))

all: mkdir $(OBJDIR)/pcf

$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/pcf: $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS)

mkdir:
	mkdir -p $(OBJDIR)

clean:
	rm -f $(OBJDIR)/*.o

