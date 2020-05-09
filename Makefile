TARGET = ezhttpd

# CONFIG_DEBUG = y

SRCDIR = src
OBJDIR = obj
BINDIR = bin

SOURCES := $(wildcard $(SRCDIR)/*.c)
OBJECTS := $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

HTTP_SOURCES := $(wildcard $(SRCDIR)/http/*.c)
HTTP_OBJECTS := $(HTTP_SOURCES:$(SRCDIR)/http/%.c=$(OBJDIR)/http/%.o)

CGI_SOURCES := $(wildcard $(SRCDIR)/cgi/*.c)
CGI_OBJECTS := $(CGI_SOURCES:$(SRCDIR)/cgi/%.c=$(OBJDIR)/cgi/%.o)

DEBUG_SOURCES := $(wildcard $(SRCDIR)/debug/*.c)
DEBUG_OBJECTS := $(DEBUG_SOURCES:$(SRCDIR)/debug/%.c=$(OBJDIR)/debug/%.o)

SSL_SOURCES := $(wildcard $(SRCDIR)/ssl/*.c)
SSL_OBJECTS := $(SSL_SOURCES:$(SRCDIR)/ssl/%.c=$(OBJDIR)/ssl/%.o)

CC = gcc
CFLAGS = -std=gnu11 -Wall -I $(SRCDIR)

ifeq ($(CONFIG_DEBUG),y)
CFLAGS += -DDEBUG
endif

LD = gcc
LDFLAGS = -lssl -lcrypto

RM = rm -f

$(BINDIR)/$(TARGET): $(OBJECTS) $(HTTP_OBJECTS) $(CGI_OBJECTS) \
					 $(DEBUG_OBJECTS) $(SSL_OBJECTS)
	@echo "link" $@
	@mkdir -p $(BINDIR)
	@$(LD) $(OBJECTS) $(HTTP_OBJECTS) $(CGI_OBJECTS) \
		   $(DEBUG_OBJECTS) $(SSL_OBJECTS) $(LDFLAGS) -o $@

$(OBJECTS): $(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	@echo $(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(HTTP_OBJECTS): $(OBJDIR)/http/%.o: $(SRCDIR)/http/%.c
	@mkdir -p $(OBJDIR)/http
	@echo $(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(CGI_OBJECTS): $(OBJDIR)/cgi/%.o: $(SRCDIR)/cgi/%.c
	@mkdir -p $(OBJDIR)/cgi
	@echo $(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_OBJECTS): $(OBJDIR)/debug/%.o: $(SRCDIR)/debug/%.c
	@mkdir -p $(OBJDIR)/debug
	@echo $(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

$(SSL_OBJECTS): $(OBJDIR)/ssl/%.o: $(SRCDIR)/ssl/%.c
	@mkdir -p $(OBJDIR)/ssl
	@echo $(CC) $(CFLAGS) -c $< -o $@
	@$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	@$(RM) $(OBJECTS)
	@$(RM) $(HTTP_OBJECTS)
	@$(RM) $(CGI_OBJECTS)
	@$(RM) $(DEBUG_OBJECTS)
	@$(RM) $(SSL_OBJECTS)

.PHONY: remove
remove: clean
	@$(RM) $(BINDIR)/$(TARGET)
