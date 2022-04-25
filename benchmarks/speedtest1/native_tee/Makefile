export V ?= 0

OUTPUT_DIR := $(CURDIR)/out

# If _HOST, _LIB or _TA specific compilers are not specified, then fall back to CROSS_COMPILE
# TODO: either specify HOST_CROSS_COMPILE, LIB_CROSS_COMPILE and TA_CROSS_COMPILE or specify
# CROSS_COMPILE.
HOST_CROSS_COMPILE ?= $(CROSS_COMPILE)
TA_CROSS_COMPILE ?= $(CROSS_COMPILE)

#EXAMPLE_LIST := $(subst /,,$(dir $(wildcard */Makefile)))
# TODO: include src directories?

.PHONY: all
all: prepare-for-rootfs

.PHONY: all_mwe
all_mwe:
	$(MAKE) -C host CROSS_COMPILE="$(HOST_CROSS_COMPILE)" --no-builtin-variables
	$(MAKE) -C ta CROSS_COMPILE="$(TA_CROSS_COMPILE)" LDFLAGS=""

.PHONY: clean
clean: prepare-for-rootfs-clean
	$(MAKE) -C host clean
	$(MAKE) -C ta clean

prepare-for-rootfs: all_mwe
	@echo "Copying example CA and TA binaries to $(OUTPUT_DIR)..."
	@mkdir -p $(OUTPUT_DIR)
	@mkdir -p $(OUTPUT_DIR)/ta
	@mkdir -p $(OUTPUT_DIR)/ca
	if [ -e host/bm_speedtest1_native_tee ]; then \
		cp -p host/bm_speedtest1_native_tee $(OUTPUT_DIR)/ca/; \
	fi; \
	cp -pr ta/*.ta $(OUTPUT_DIR)/ta/; \

prepare-for-rootfs-clean:
	@rm -rf $(OUTPUT_DIR)/ta
	@rm -rf $(OUTPUT_DIR)/ca
	@rmdir --ignore-fail-on-non-empty $(OUTPUT_DIR) || test ! -e $(OUTPUT_DIR)
