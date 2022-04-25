# TODO: CFG_TEE_TA_LOG_LEVEL needs to be specified
CFG_TEE_TA_LOG_LEVEL ?= 2
CPPFLAGS += -O3 -DCFG_TEE_TA_LOG_LEVEL=$(CFG_TEE_TA_LOG_LEVEL) \
	-DSQLITE_OS_OTHER -DSQLITE_ENABLE_MEMSYS3 -DSQLITE_OMIT_FLOATING_POINT -DSQLITE_OMIT_DATETIME_FUNCS -DSQLITE_OMIT_DEPRECATED

# The UUID for the Trusted Application
BINARY=b4c9d236-cb68-45a5-b218-4266ca43b237

# The directory of additional static trusted libraries
# NOTE: using LDADD does not include the static (trusted) library between the
# --start-group and --end-group arguments. Hence, the static library will only
# be searched once for undefined references.
#LDADD=-L../lib -l4a8dc7a0-3fc7-4a7b-936d-ffdd24e9cd05
#LDADD=-L../lib -lmwe

# See sub.mk for an alternative method

# TODO: TA_DEV_KIT_DIR needs to be specified
-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
