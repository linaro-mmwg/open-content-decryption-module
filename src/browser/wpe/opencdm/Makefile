 #
 # Copyright (C) 2016 TATA ELXSI
 # Copyright (C) 2016 Metrological
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 # 1. Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 # 2. Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 # "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 # LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 # PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 # HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 # SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 # LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 # DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 # THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 #
RM := rm  -rf
PWD = $(shell pwd)
OCDM_DIR = $(PWD)/../../..
OCDM_WPE_DIR = $(PWD)/..
CFLAGS_L := -fPIC -D_REENTRANT -Wall $(LDFLAGS)

MODULES   := cdm mediaengine common com/cdm/rpc com/cdm  com/mediaengine/rpc com/common/shmemsem
OCDM_SOURCES_C =  $(wildcard $(OCDM_DIR)/com/common/rpc/*.c)
OCDM_SOURCES_CPP = $(wildcard $(OCDM_DIR)/browser/wpe/opencdm/*.cpp)
OCDM_SOURCES_CC_DIR   := $(addprefix $(OCDM_DIR)/,$(MODULES))
OCDM_SOURCES_CC     := $(foreach sdir,$(OCDM_SOURCES_CC_DIR),$(wildcard $(sdir)/*.cc))
OCDM_HEADERS = $(wildcard $(OCDM_DIR)/com/common/rpc/*.h)
OCDM_HEADERS := $(OCDM_HEADERS) $(wildcard $(OCDM_DIR)/browser/wpe/opencdm/*.h)
OCDM_HEADERS := $(OCDM_HEADERS) $(foreach sdir,$(OCDM_SOURCES_CC_DIR),$(wildcard $(sdir)/*.h))
OCDM_HEADER_PATH := $(foreach sdir,$(OCDM_SOURCES_CC_DIR),$(addprefix -I,$(sdir)/))

OCDM_OBJECTS_C = $(OCDM_SOURCES_C:.c=.o)
OCDM_OBJECTS_CC = $(OCDM_SOURCES_CC:.cc=.o)
OCDM_OBJECTS_CPP = $(OCDM_SOURCES_CPP:.cpp=.o)
OCDM_INCLUDES :=  $(OCDM_HEADER_PATH) \
	         -I $(OCDM_DIR)/com/common/rpc \
		 -I $(OCDM_DIR)/browser/wpe/opencdm

CXFLAGS := -std=c++11 -g $(OCDM_INCLUDES) -pthread $(CXXFLAGS)
CFLAGS := -g $(OCDM_INCLUDES) -pthread $(CPPFLAGS)


all: ocdm install

ocdm: $(OCDM_OBJECTS_C)  $(OCDM_OBJECTS_CC)  ${OCDM_OBJECTS_CPP}
	@echo "Compiling ..."
	@mkdir -p $(OCDM_WPE_DIR)/lib
	$(CXX)  ${OCDM_OBJECTS_C}  ${OCDM_OBJECTS_CC} ${OCDM_OBJECTS_CPP} -shared -o $(OCDM_WPE_DIR)/lib/lib$@.so

%.o: %.c
	$(CC) $(CFLAGS)  $(CFLAGS_L) -o $@ -c $<
%.o: %.cc
	$(CXX) $(CXFLAGS) $(CFLAGS_L) -o $@ -c $<
%.o: %.cpp
	$(CXX) $(CXFLAGS) $(CFLAGS_L) -o $@ -c $<

install:$(OCDM_HEADERS)
	@mkdir -p $(OCDM_WPE_DIR)/include
	@cp -r $(OCDM_HEADERS) $(OCDM_WPE_DIR)/include/

.PHONY: clean
clean:
	@echo "Cleaning ..."
	@$(RM)  ${OCDM_OBJECTS_C}  ${OCDM_OBJECTS_CC} ${OCDM_OBJECTS_CPP}
	@rm -f $(OCDM_WPE_DIR)/lib/libocdm.so
	@rm -f $(OCDM_WPE_DIR)/include/*
