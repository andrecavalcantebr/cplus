# Simple Makefile
# root
#  |
#  +--- BIN_DIR
#  |
#  +--- INC_DIR
#  |
#  +--- LIB_DIR
#  |
#  +--- OBJ_DIR
#  |
#  +--- *SRC_DIR
#  |
#  +--- *TEST_DIR
#  |
#  +--- **VENDOR_DIR
#
#  * Mandatory
#  ** Third-party source code (.c and .h), one directory for vendor
#

# ============================
# Makefile - Projeto Matrix
# ============================

# Diretórios
PRJ_DIR := $(shell basename "$(CURDIR)")

BIN_DIR := .
INC_DIR := inc
LIB_DIR := lib
OBJ_BASE := obj
SRC_DIR := src
TEST_DIR := test
VENDOR_DIR := vendor

# Configuração de build: debug vs release
DEBUG ?= 1

ifeq ($(DEBUG),1)
	CFLAGS := -Wall -Wextra -std=gnu2x -g3 -O0 -DDEBUG
	BUILD_DIR := $(OBJ_BASE)/debug
else
	CFLAGS := -Wall -Wextra -std=gnu2x -O2
	BUILD_DIR := $(OBJ_BASE)/release
endif

# Código-fonte
MAIN_SRC := $(SRC_DIR)/main.c
SRC := $(wildcard $(SRC_DIR)/*.c)
LIB_SRC := $(filter-out $(MAIN_SRC), $(SRC))

# Objetos
OBJ := $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(LIB_SRC))
MAIN_OBJ := $(BUILD_DIR)/main.o

# Executável principal
EXE := $(BIN_DIR)/$(PRJ_DIR)

# Testes
TEST_SRC := $(wildcard $(TEST_DIR)/*.c)
TEST_OBJ := $(patsubst $(TEST_DIR)/%.c, $(BUILD_DIR)/%.test.o, $(TEST_SRC))
TEST_EXE := $(BIN_DIR)/run_tests

# Código fonte de terceiros
VENDOR_LIBS := mpc tgc
VENDOR_SRC := $(foreach lib,$(VENDOR_LIBS),$(wildcard $(VENDOR_DIR)/$(lib)/*.c))
VENDOR_OBJ := $(patsubst $(VENDOR_DIR)/%/%.c,$(BUILD_DIR)/vendor/%_%.o,$(VENDOR_SRC))

# Compilador e flags
CC := gcc
CPPFLAGS := -I. -I$(SRC_DIR) -I$(INC_DIR) -MMD -MP
LDFLAGS := -L$(LIB_DIR)
LDLIBS := -lm -lpthread

# includes de terceiros
CPPFLAGS += $(foreach lib,$(VENDOR_LIBS),-I$(VENDOR_DIR)/$(lib))


.PHONY: all clean vars help test

# ========================
# Alvo principal
# ========================
all: $(EXE)

$(EXE): $(OBJ) $(MAIN_OBJ) $(VENDOR_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR) $(BUILD_DIR):
	mkdir -p $@

# Compilar objetos do src
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilar main separadamente
$(MAIN_OBJ): $(MAIN_SRC) | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# Compilar qualquer ../vendor/<lib>/<file>.c -> $(BUILD_DIR)/<lib>_<file>.o
$(BUILD_DIR)/%_%.o: $(VENDOR_DIR)/%/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# ========================
# Alvo: Testes
# ========================
run_test: test
	@echo "==> Executando testes..."
	@./$(TEST_EXE)

test: $(TEST_EXE)


$(BUILD_DIR)/%.test.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(TEST_EXE): $(OBJ) $(TEST_OBJ) $(VENDOR_OBJ) | $(BIN_DIR)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# ========================
# Utilitários
# ========================
vars:
	@echo "DEBUG = $(DEBUG)"
	@echo "BUILD_DIR = $(BUILD_DIR)"
	@echo "OBJ_BASE = $(OBJ_BASE)"
	@echo " "
	@echo "SRC = $(SRC)"
	@echo "OBJ = $(OBJ)"
	@echo "TEST_SRC = $(TEST_SRC)"
	@echo "TEST_OBJ = $(TEST_OBJ)"
	@echo " "
	@echo "VENDOR_DIR = $(VENDOR_DIR)"
	@echo "VENDOR_LIBS = $(VENDOR_LIBS)"
	@echo "VENDOR_SRC = $(VENDOR_SRC)"
	@echo " "
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "CPPFLAGS = $(CPPFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"
	@echo "LDLIBS = $(LDLIBS)"

help:
	@echo "Comandos disponíveis:"
	@echo "  make          Compila o projeto (DEBUG=$(DEBUG))"
	@echo "  make test     Compila os testes"
	@echo "  make run_test Executa os testes"
	@echo "  make clean    Remove binários e objetos"
	@echo "  make vars     Mostra variáveis internas"

clean:
	-@$(RM) -rv $(BIN_DIR)/$(PRJ_DIR) $(TEST_EXE) $(OBJ_BASE)

# Dependências automáticas
ifneq (,$(filter-out clean help vars,$(MAKECMDGOALS)))
-include $(OBJ:.o=.d)
-include $(TEST_OBJ:.o=.d)
-include $(MAIN_OBJ:.o=.d)
-include $(VENDOR_OBJ:.o=.d)
endif
