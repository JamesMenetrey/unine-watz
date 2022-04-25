ifdef PROFILING
	PROFILING_FLAG = -DPROFILING_$(PROFILING)
endif

ifdef TA_DATA_SIZE
	TA_DATA_SIZE_FLAG = -DTA_DATA_SIZE=$(TA_DATA_SIZE)
endif

CFG_TEE_TA_LOG_LEVEL ?= 4
CPPFLAGS += -O3 -DCFG_TEE_TA_LOG_LEVEL=$(CFG_TEE_TA_LOG_LEVEL) $(TA_DATA_SIZE_FLAG) $(PROFILING_FLAG)

# The UUID for the Trusted Application
BINARY=bc20728a-6a28-49d8-98d8-f22e7535f137

# TODO: TA_DEV_KIT_DIR needs to be specified
-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	rm -f *.o .*.o.d .*.o.cmd .*.ld.d *.dmp *.elf *.map *.ta *.lds dyn_list
#@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
#@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
