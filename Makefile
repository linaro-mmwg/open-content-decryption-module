PWD = $(shell pwd)

all: ocdm_lib
ocdm_lib:
	$(MAKE) -C $(PWD)/src/browser/wpe/opencdm/ all


clean: ocdm_lib_clean cdm_client_clean

ocdm_lib_clean:
	$(MAKE) -C $(PWD)/src/browser/wpe/opencdm/ clean
cdm_client_clean:

install: ocdm_lib_install

ocdm_lib_install:
	$(MAKE) -C $(PWD)/src/browser/wpe/opencdm/ install
