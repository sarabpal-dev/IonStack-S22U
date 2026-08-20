API ?= 35
PROJECT ?= S908WVLS8FYG7
PROJECT_SUFFIX := $(if $(USE_BUILDROOT),-qemu,)
OUTDIR ?= build/$(PROJECT)$(PROJECT_SUFFIX)/bin

TARGET_DIR := src/targets/$(PROJECT)
TARGET_HEADER := $(TARGET_DIR)/target.h

ifeq ($(wildcard $(TARGET_HEADER)),)
$(error unknown PROJECT=$(PROJECT), missing $(TARGET_HEADER))
endif

# pick_src: use target-specific override if it exists, otherwise use src/
define pick_src
$(if $(wildcard $(TARGET_DIR)/$(1)),$(TARGET_DIR)/$(1),src/$(1))
endef

PRELOAD := $(OUTDIR)/cve-2026-43499
EXPLOIT_EXEC := $(OUTDIR)/cve-2026-43499-exec
ROOT_HELPER := $(OUTDIR)/cve-2026-43499-root
EXP32_OUT := $(OUTDIR)/cve-exp32
TEST_SKB_RECLAIM := $(OUTDIR)/test-skb-reclaim
TEST_PSELECT_ROUTE := $(OUTDIR)/test-pselect-route

CORE_SRCS := \
  $(call pick_src,main.c) \
  $(call pick_src,util.c) \
  $(call pick_src,slide.c) \
  $(call pick_src,fops.c) \
  $(call pick_src,pipe.c) \
  src/api.c \
  src/exp32_blob.S \
  src/root.c

PRELOAD_SRCS := $(CORE_SRCS) src/preload.c

# Buildroot cross-compilation toolchain
BUILDROOT_DIR := buildroot-tc
BUILDROOT_TOOLCHAIN := $(BUILDROOT_DIR)/bin
BUILDROOT_CC := $(BUILDROOT_TOOLCHAIN)/aarch64-linux-gcc
BUILDROOT_SYSROOT := $(BUILDROOT_DIR)/aarch64-buildroot-linux-gnu/sysroot

DEFAULT_NDK_ROOT := $(or $(wildcard $(HOME)/Android/Sdk/ndk/27.2.12479018),$(wildcard $(HOME)/Desktop/android-ndk-r27d),$(HOME)/android-ndk-cache/android-ndk-r29)
NDK_ROOT ?= $(or $(ANDROID_NDK_HOME),$(ANDROID_NDK_ROOT),$(DEFAULT_NDK_ROOT))
NDK_HOST_TAG ?= $(if $(filter Windows_NT,$(OS)),windows-x86_64,linux-x86_64)
NDK_EXE := $(if $(filter windows-x86_64,$(NDK_HOST_TAG)),.cmd,)
NDK_TOOLCHAIN ?= $(if $(NDK_ROOT),$(NDK_ROOT)/toolchains/llvm/prebuilt/$(NDK_HOST_TAG))
NDK_CC := $(NDK_TOOLCHAIN)/bin/aarch64-linux-android$(API)-clang$(NDK_EXE)
HOST_CLANG ?= clang
SYSROOT ?= $(if $(NDK_TOOLCHAIN),$(NDK_TOOLCHAIN)/sysroot)
RESOURCE_DIR ?= $(if $(NDK_TOOLCHAIN),$(NDK_TOOLCHAIN)/lib/clang/21)

HOST_TARGET_FLAGS := \
  --target=aarch64-linux-android$(API) \
  --sysroot=$(SYSROOT) \
  -resource-dir $(RESOURCE_DIR) \
  --rtlib=compiler-rt \
  --unwindlib=none

HOST_COMMON_LDFLAGS := \
  -fuse-ld=lld \
  -Wl,-rpath-link,$(SYSROOT)/usr/lib/aarch64-linux-android/$(API) \
  -L$(SYSROOT)/usr/lib/aarch64-linux-android/$(API) \
  -L$(SYSROOT)/usr/lib/aarch64-linux-android

HOST_PIE_LDFLAGS := \
  $(HOST_COMMON_LDFLAGS) \
  -Wl,-dynamic-linker,/system/bin/linker64

ifneq ($(origin CC),default)
  TARGET_CC := $(CC)
  TARGET_FLAGS :=
  TARGET_COMMON_LDFLAGS :=
  TARGET_PIE_LDFLAGS :=
else ifneq ($(and $(or $(filter-out S908WVLS8FYG7,$(PROJECT)),$(USE_BUILDROOT)),$(wildcard $(BUILDROOT_CC))),)
  BUILDROOT_CC_WORKS := $(shell $(BUILDROOT_CC) --version >/dev/null 2>&1 && echo yes)
  ifeq ($(BUILDROOT_CC_WORKS),yes)
    TARGET_CC := $(BUILDROOT_CC)
    TARGET_FLAGS := --sysroot=$(BUILDROOT_SYSROOT)
    TARGET_COMMON_LDFLAGS := -L$(BUILDROOT_SYSROOT)/usr/lib -L$(BUILDROOT_SYSROOT)/lib
    TARGET_PIE_LDFLAGS := $(TARGET_COMMON_LDFLAGS) -Wl,-dynamic-linker,/lib/ld-linux-aarch64.so.1
  else
    TARGET_CC := $(HOST_CLANG)
    TARGET_FLAGS := $(HOST_TARGET_FLAGS)
    TARGET_COMMON_LDFLAGS := $(HOST_COMMON_LDFLAGS)
    TARGET_PIE_LDFLAGS := $(HOST_PIE_LDFLAGS)
  endif
else ifneq ($(wildcard $(NDK_CC)),)
  NDK_CC_WORKS := $(shell $(NDK_CC) --version >/dev/null 2>&1 && echo yes)
  ifeq ($(NDK_CC_WORKS),yes)
    TARGET_CC := $(NDK_CC)
    TARGET_FLAGS :=
    TARGET_COMMON_LDFLAGS :=
    TARGET_PIE_LDFLAGS :=
  else
    TARGET_CC := $(HOST_CLANG)
    TARGET_FLAGS := $(HOST_TARGET_FLAGS)
    TARGET_COMMON_LDFLAGS := $(HOST_COMMON_LDFLAGS)
    TARGET_PIE_LDFLAGS := $(HOST_PIE_LDFLAGS)
  endif
else
  TARGET_CC := $(HOST_CLANG)
  TARGET_FLAGS := $(HOST_TARGET_FLAGS)
  TARGET_COMMON_LDFLAGS := $(HOST_COMMON_LDFLAGS)
  TARGET_PIE_LDFLAGS := $(HOST_PIE_LDFLAGS)
endif

# ── Embedded 32-bit (armeabi-v7a) exploit stage ─────────────────────
# exp32 MUST stay 32-bit: the stack-stamp only lines up via the compat
# syscall path (see src/exp32/).  Built once, embedded via exp32_blob.S.
EMBEDDIR ?= build/embed
EMBED_EXP32 := $(EMBEDDIR)/cve_exp32_arm32
EXP32_SRCS := src/exp32/main.c src/exp32/stack.c src/exp32/tls_align.S

API32 ?= 28
NDK_CC32 := $(NDK_TOOLCHAIN)/bin/armv7a-linux-androideabi$(API32)-clang$(NDK_EXE)
HOST_CC32 ?= arm-linux-gnueabi-gcc

ifeq ($(shell command -v $(HOST_CC32) >/dev/null 2>&1 && echo yes),yes)
  EXP32_CC := $(HOST_CC32)
  EXP32_CFLAGS := -O2 -g0 -Wall -Isrc -static -pthread
  EXP32_LDFLAGS := -static -pthread
else ifneq ($(wildcard $(NDK_CC32)),)
  EXP32_CC := $(NDK_CC32)
  EXP32_CFLAGS := -O2 -g0 -Wall -Isrc -fPIE -pthread \
    -Wno-unused-parameter -Wno-unused-function
  EXP32_LDFLAGS := -static -pie -pthread
else
  $(error no 32-bit ARM toolchain: install arm-linux-gnueabi-gcc or an NDK)
endif

# ── Build flags ────────────────────────────────────────────────────
COMMON_CFLAGS := -O2 -g0 -Wall -Wextra -Isrc
PIE_CFLAGS := -fPIE -pie $(COMMON_CFLAGS)
SO_CFLAGS := -fPIC $(COMMON_CFLAGS)
WARN_CFLAGS := -Wno-unused-parameter -Wno-sign-compare -Wno-unused-function
TARGET_CFLAGS := -DTARGET_CONFIG_H=\"targets/$(PROJECT)/target.h\" -I$(TARGET_DIR)

.DEFAULT_GOAL := preload

.PHONY: all preload exploit-exec root-helper test-skb-reclaim test-pselect-route info clean list-projects

all: preload exploit-exec root-helper $(EXP32_OUT)

exploit-exec: $(EXPLOIT_EXEC)

test-skb-reclaim: $(TEST_SKB_RECLAIM)

test-pselect-route: $(TEST_PSELECT_ROUTE)

preload: $(PRELOAD) $(EXP32_OUT)

root-helper: $(ROOT_HELPER)

$(OUTDIR):
	rm -rf $(OUTDIR)
	mkdir -p $@

$(EMBEDDIR):
	mkdir -p $@

$(EMBED_EXP32): $(EXP32_SRCS) src/kernelsnitch/utils.h | $(EMBEDDIR)
	$(EXP32_CC) $(EXP32_CFLAGS) $(EXP32_SRCS) $(EXP32_LDFLAGS) -o $@
	sha256sum $@

$(EXP32_OUT): $(EMBED_EXP32) | $(OUTDIR)
	cp $< $@
	sha256sum $@

$(ROOT_HELPER): src/su_daemon.c $(TARGET_HEADER) | $(OUTDIR)
	$(TARGET_CC) $(TARGET_FLAGS) $(PIE_CFLAGS) $(TARGET_CFLAGS) \
	  $< $(TARGET_PIE_LDFLAGS) -o $@
	sha256sum $@

$(PRELOAD): $(PRELOAD_SRCS) $(EMBED_EXP32) $(TARGET_HEADER) src/offset.h src/common.h src/kernelsnitch/*.h | $(OUTDIR)
	$(TARGET_CC) $(TARGET_FLAGS) $(SO_CFLAGS) $(WARN_CFLAGS) $(TARGET_CFLAGS) \
	  $(PRELOAD_SRCS) $(TARGET_COMMON_LDFLAGS) \
	  -shared -o $@ -pthread -ldl
	ln -sf $(notdir $@) $(OUTDIR)/cve.so
	sha256sum $@

$(EXPLOIT_EXEC): $(CORE_SRCS) src/exploit_main.c $(EMBED_EXP32) $(TARGET_HEADER) src/offset.h src/common.h src/kernelsnitch/*.h | $(OUTDIR)
	$(TARGET_CC) $(TARGET_FLAGS) $(PIE_CFLAGS) $(WARN_CFLAGS) $(TARGET_CFLAGS) \
	  $(CORE_SRCS) src/exploit_main.c $(TARGET_PIE_LDFLAGS) -o $@ -pthread
	ln -sf $(notdir $@) $(OUTDIR)/cve-exec
	sha256sum $@

$(TEST_SKB_RECLAIM): test-programs/test_skb_reclaim.c src/util.c $(TARGET_HEADER) src/offset.h src/kernelsnitch/*.h | $(OUTDIR)
	$(TARGET_CC) $(TARGET_FLAGS) $(PIE_CFLAGS) $(WARN_CFLAGS) $(TARGET_CFLAGS) \
	  test-programs/test_skb_reclaim.c src/util.c $(TARGET_PIE_LDFLAGS) \
	  -o $@ -pthread
	sha256sum $@

$(TEST_PSELECT_ROUTE): test-programs/test_pselect_route.c src/util.c $(TARGET_HEADER) src/offset.h src/kernelsnitch/*.h | $(OUTDIR)
	$(TARGET_CC) $(TARGET_FLAGS) $(PIE_CFLAGS) $(WARN_CFLAGS) $(TARGET_CFLAGS) \
	  test-programs/test_pselect_route.c src/util.c $(TARGET_PIE_LDFLAGS) \
	  -o $@ -pthread
	sha256sum $@

info:
	@echo "PROJECT=$(PROJECT)"
	@echo "TARGET_DIR=$(TARGET_DIR)"
	@echo "TARGET_CC=$(TARGET_CC)"
	@echo "TARGET_FLAGS=$(TARGET_FLAGS)"
	@echo "TARGET_COMMON_LDFLAGS=$(TARGET_COMMON_LDFLAGS)"
	@echo "TARGET_PIE_LDFLAGS=$(TARGET_PIE_LDFLAGS)"
	@echo "PRELOAD=$(PRELOAD)"
	@echo "EXPLOIT_EXEC=$(EXPLOIT_EXEC)"
	@echo "ROOT_HELPER=$(ROOT_HELPER)"
	@echo "TEST_SKB_RECLAIM=$(TEST_SKB_RECLAIM)"
	@echo "CORE_SRCS=$(CORE_SRCS)"

list-projects:
	@find src/targets -mindepth 2 -maxdepth 2 -name target.h -printf '%h\n' | sed 's#src/targets/##' | sort

clean:
	rm -rf build
