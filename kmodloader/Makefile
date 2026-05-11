# kmodloader build (FreeBSD).
#
# Phase 1d: ObjC daemon that asks devmatch(8) which klds are needed
# for the live device tree, then kldloads each. devmatch reads
# linker.hints (kldxref(8) output, sourced from each driver's
# PNP_INFO macros) — that's the authoritative (vendor:device → kld)
# map for the FreeBSD driver ecosystem. Boot-time matching only;
# hot-plug via /dev/devctl is Phase 2.
#
# Plan: https://pkgdemon.github.io/freebsd-kmodloader-plan.html

TOPDIR := $(abspath $(CURDIR)/..)
COMPAT := $(CURDIR)/compat
BUILD  := $(TOPDIR)/build/kmodloader

CC := clang

SYSINC := /System/Library/Headers
SYSLIB := /System/Library/Libraries

OBJCFLAGS := -fobjc-runtime=gnustep-2.2 -fobjc-arc -fblocks \
             -I$(SYSINC)/gnustep -I$(SYSINC) -I$(COMPAT) \
             -I$(CURDIR)/src \
             -O2 -Wall -Wextra -Wno-unused-parameter

LDFLAGS := -fuse-ld=lld \
           -L$(SYSLIB) \
           -ldispatch \
           -lBlocksRuntime \
           -lgnustep-base \
           -lobjc \
           -Wl,-rpath,$(SYSLIB)

SRCS_M := \
    src/main.m \
    src/KMDaemon.m

OBJS := $(SRCS_M:%.m=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all
all: $(BUILD)/kmodloader
	@echo "==> kmodloader: built in $(BUILD)"

$(BUILD)/kmodloader: $(OBJS)
	@mkdir -p $(@D)
	@echo "  LD  kmodloader"
	@$(CC) -o $@ $(OBJS) $(LDFLAGS)

$(BUILD)/%.o: %.m
	@mkdir -p $(@D)
	@echo "  OBJC $(notdir $<)"
	@$(CC) $(OBJCFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

.PHONY: clean
clean:
	rm -rf $(BUILD)

# Daemon: /usr/libexec/kmodloader (per plan §6 install paths).
PREFIX  ?= /usr/libexec
DESTDIR ?=

.PHONY: install
install: all
	@mkdir -p $(DESTDIR)$(PREFIX)
	@install -m 755 $(BUILD)/kmodloader $(DESTDIR)$(PREFIX)/kmodloader
	@echo "==> kmodloader installed: $(DESTDIR)$(PREFIX)/kmodloader"
