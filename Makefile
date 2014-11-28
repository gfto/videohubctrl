CC = cc
STRIP = strip
CROSS := $(TARGET)
MKDEP = $(CROSS)$(CC) -MP -MM -o $*.d $<
RM = rm -f
MV = mv -f

ifndef V
Q = @
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

CLEAN_OBJS = videohubctrl $(videohubctrl_SRC:.c=.o) $(videohubctrl_SRC:.c=.d)
DISTCLEAN_OBJS = version.h

.PHONY: distclean clean version

PROGS=videohubctrl

version:
	$(shell ./version.sh >/dev/null)
	@$(MAKE) --no-print-directory videohubctrl

videohubctrl: $(videohubctrl_OBJS)
	$(Q)echo "  LINK	videohubctrl"
	$(Q)$(CROSS)$(CC) $(CFLAGS) $(DEFS) $(videohubctrl_OBJS) $(videohubctrl_LIBS) $(LDFLAGS) -o videohubctrl

$(FUNCS_LIB): $(FUNCS_DIR)/libfuncs.h
	$(Q)echo "  MAKE	$(FUNCS_LIB)"
	$(Q)$(MAKE) -s -C $(FUNCS_DIR)

%.o: %.c Makefile RELEASE
	@$(MKDEP)
	$(Q)echo "  CC	videohubctrl	$<"
	$(Q)$(CROSS)$(CC) $(CFLAGS) $(DEFS) -c $<

-include $(videohubctrl_SRC:.c=.d)

strip:
	$(Q)echo "  STRIP	$(PROGS)"
	$(Q)$(CROSS)$(STRIP) $(PROGS)

clean:
	$(Q)echo "  RM	$(CLEAN_OBJS)"
	$(Q)$(RM) $(CLEAN_OBJS)

distclean: clean
	$(Q)echo "  RM	$(DISTCLEAN_OBJS)"
	$(Q)$(RM) $(DISTCLEAN_OBJS)
	$(Q)$(MAKE) -s -C $(FUNCS_DIR) clean
