TARGET = ezhttpd

# CONFIG_DEBUG = y

CC = gcc
CFLAGS = -std=gnu11 -Wall

ifeq ($(CONFIG_DEBUG),y)
CFLAGS += -DDEBUG
endif

LD = gcc

RM = rm -f

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(BINDIR)/$(TARGET): $(OBJECTS)
	@echo "link" $@
	@mkdir -p $(BINDIR)
	@$(LD) $(OBJECTS) -o $@

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	@echo "compile" $< "to" $@
	@mkdir -p $(OBJDIR)
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)

.PHONY: remove
remove: clean
	@$(RM) $(BINDIR)/$(TARGET)
