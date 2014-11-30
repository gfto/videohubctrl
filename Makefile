CC = cc
STRIP = strip
CROSS := $(TARGET)
MKDEP = $(CROSS)$(CC) -MP -MM -o $*.d $<
RM = rm -f
MV = mv -f

# Setup quiet build
Q =
SAY = @true
ifndef V
Q = @
SAY = @echo
endif

CFLAGS ?= -O2 -ggdb -pipe -ffunction-sections -fdata-sections \
 -W -Wall -Wextra \
 -Wshadow -Wformat-security -Wstrict-prototypes -Wno-unused-parameter \
 -Wredundant-decls -Wold-style-definition

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')

LDFLAGS ?= -Wl,--gc-sections

ifeq ($(uname_S),Darwin)
LDFLAGS :=
endif

DEFS += -D_FILE_OFFSET_BITS=64

PREFIX ?= /usr/local

INSTALL_PRG = videohubctrl
INSTALL_PRG_DIR = $(subst //,/,$(DESTDIR)/$(PREFIX)/bin)

INSTALL_DOC = videohubctrl.1
INSTALL_DOC_DIR = $(subst //,/,$(DESTDIR)/$(PREFIX)/share/man/man1)

FUNCS_DIR = libfuncs
FUNCS_LIB = $(FUNCS_DIR)/libfuncs.a

videohubctrl_SRC = \
	cmd.c \
	net.c \
	util.c \
	display.c \
	videohubctrl.c
videohubctrl_LIBS = -lpthread
videohubctrl_OBJS = $(FUNCS_LIB) $(videohubctrl_SRC:.c=.o)

CLEAN_OBJS = videohubctrl $(videohubctrl_SRC:.c=.o) $(videohubctrl_SRC:.c=.d) test/*.out
DISTCLEAN_OBJS = version.h

.PHONY: distclean clean version install uninstall

PROGS=videohubctrl

version:
	$(shell ./version.sh >/dev/null)
	@$(MAKE) --no-print-directory videohubctrl

videohubctrl: $(videohubctrl_OBJS)
	$(SAY) "  LINK	videohubctrl"
	$(Q)$(CROSS)$(CC) $(CFLAGS) $(DEFS) $(videohubctrl_OBJS) $(videohubctrl_LIBS) $(LDFLAGS) -o videohubctrl

all: version

$(FUNCS_LIB): $(FUNCS_DIR)/libfuncs.h
	$(SAY) "  MAKE	$(FUNCS_LIB)"
	$(Q)$(MAKE) -s -C $(FUNCS_DIR)

%.o: %.c Makefile RELEASE
	@$(MKDEP)
	$(SAY) "  CC	videohubctrl	$<"
	$(Q)$(CROSS)$(CC) $(CFLAGS) $(DEFS) -c $<

-include $(videohubctrl_SRC:.c=.d)

strip:
	$(SAY) "  STRIP	$(PROGS)"
	$(Q)$(CROSS)$(STRIP) $(PROGS)

clean:
	$(SAY) "  RM	$(CLEAN_OBJS)"
	$(Q)$(RM) $(CLEAN_OBJS)

distclean: clean
	$(SAY) "  RM	$(DISTCLEAN_OBJS)"
	$(Q)$(RM) $(DISTCLEAN_OBJS)
	$(Q)$(MAKE) -s -C $(FUNCS_DIR) clean

install: all
	$(Q)install -d "$(INSTALL_PRG_DIR)"
	$(Q)install -d "$(INSTALL_DOC_DIR)"
	$(SAY) "INSTALL $(INSTALL_PRG) -> $(INSTALL_PRG_DIR)"
	$(Q)-install $(INSTALL_PRG) "$(INSTALL_PRG_DIR)"
	$(SAY) "INSTALL $(INSTALL_DOC) -> $(INSTALL_DOC_DIR)"
	$(Q)-install -m 0644 $(INSTALL_DOC) "$(INSTALL_DOC_DIR)"

uninstall:
	@-for FILE in $(INSTALL_PRG); do \
		echo "RM       $(INSTALL_PRG_DIR)/$$FILE"; \
		rm "$(INSTALL_PRG_DIR)/$$FILE"; \
	done
	@-for FILE in $(INSTALL_DOC); do \
		echo "RM       $(INSTALL_DOC_DIR)/$$FILE"; \
		rm "$(INSTALL_DOC_DIR)/$$FILE"; \
	done

help:
	@printf "\
videohubctrl build parameters\n\n\
Build targets:\n\
  all             - Build videohubctrl\n\
  install         - Install videohubctrl in PREFIX: $(PREFIX)\n\
  uninstall       - Uninstall videohubctrl from PREFIX\n\
\n\
Cleaning targets:\n\
  clean           - Remove videohubctrl generated files.\n\
  distclean       - Remove all generated files.\n\
\n\
  make V=1          Enable verbose build\n\
  make PREFIX=dir   Set install prefix\n\n"
