MEMORY_CHECK_BUILD_TOOLCHAIN := fisics
MEMORY_CHECK_FISICS_OVERLAY := memory-check
MEMORY_CHECK_REPORT_DIR := build/memory_check
MEMORY_CHECK_STDOUT := $(MEMORY_CHECK_REPORT_DIR)/mem_console.stdout
MEMORY_CHECK_STDERR := $(MEMORY_CHECK_REPORT_DIR)/mem_console.stderr
MEMORY_CHECK_BIN := $(TOOLCHAIN_BUILD_ROOT)/$(MEMORY_CHECK_BUILD_TOOLCHAIN)/bin/mem_console
MEMORY_CHECK_TEST_BIN := $(MEMORY_CHECK_REPORT_DIR)/mem_console_memory_check_audit
MEMORY_CHECK_TEST_OBJ_DIR := $(MEMORY_CHECK_REPORT_DIR)/obj
MEMORY_CHECK_TEST_OBJS := \
	$(MEMORY_CHECK_TEST_OBJ_DIR)/mem_console_memory_check_audit.o \
	$(MEMORY_CHECK_TEST_OBJ_DIR)/mem_console_db_graph_sort.o
MEMORY_CHECK_REPORT_POLICY ?= always
FISICS_MEMCHECK_RUNTIME ?= /Users/calebsv/Desktop/CodeWork/fisiCs/build/unsanitized/libfisics_memcheck_runtime.a
FISICS_MEMCHECK_LINK_LIBS :=

ifeq ($(BUILD_TOOLCHAIN),fisics)
ifneq ($(findstring --overlay=memory-check,$(FISICS_FLAGS)),)
FISICS_MEMCHECK_LINK_LIBS += $(FISICS_MEMCHECK_RUNTIME)
endif
endif

memory-check-build:
	@$(MAKE) BUILD_TOOLCHAIN="$(MEMORY_CHECK_BUILD_TOOLCHAIN)" FISICS_FLAGS="--overlay=$(MEMORY_CHECK_FISICS_OVERLAY)" -B all

$(MEMORY_CHECK_TEST_OBJ_DIR)/mem_console_memory_check_audit.o: tests/mem_console_memory_check_audit.c \
		$(APP_HEADERS)
	mkdir -p "$(MEMORY_CHECK_TEST_OBJ_DIR)"
	$(FISICS_CC) --overlay=$(MEMORY_CHECK_FISICS_OVERLAY) $(CFLAGS) $(INC) -Isrc/db -c "$<" -o "$@"

$(MEMORY_CHECK_TEST_OBJ_DIR)/mem_console_db_graph_sort.o: src/db/mem_console_db_graph_sort.c \
		$(APP_HEADERS)
	mkdir -p "$(MEMORY_CHECK_TEST_OBJ_DIR)"
	$(FISICS_CC) --overlay=$(MEMORY_CHECK_FISICS_OVERLAY) $(CFLAGS) $(INC) -Isrc/db -c "$<" -o "$@"

$(MEMORY_CHECK_TEST_BIN): $(MEMORY_CHECK_TEST_OBJS) \
		src/db/mem_console_db_graph_sort.c \
		$(APP_HEADERS)
	mkdir -p "$(MEMORY_CHECK_REPORT_DIR)"
	$(HOST_CC) $(ARCH_FLAGS) $(CFLAGS) $(INC) -Isrc/db $(MEMORY_CHECK_TEST_OBJS) $(FISICS_MEMCHECK_RUNTIME) -o "$@"

memory-check-run: memory-check-build
	@$(MAKE) BUILD_TOOLCHAIN="$(MEMORY_CHECK_BUILD_TOOLCHAIN)" FISICS_FLAGS="--overlay=$(MEMORY_CHECK_FISICS_OVERLAY)" "$(MEMORY_CHECK_TEST_BIN)"
	mkdir -p "$(MEMORY_CHECK_REPORT_DIR)"
	set +e; FISICS_MEMCHECK_REPORT="$(MEMORY_CHECK_REPORT_POLICY)" "$(MEMORY_CHECK_TEST_BIN)" > "$(MEMORY_CHECK_STDOUT)" 2> "$(MEMORY_CHECK_STDERR)"; status=$$?; \
	echo "memory-check stdout: $(MEMORY_CHECK_STDOUT)"; \
	echo "memory-check stderr: $(MEMORY_CHECK_STDERR)"; \
	exit $$status

memory-check-audit: memory-check-build
	@$(MAKE) BUILD_TOOLCHAIN="$(MEMORY_CHECK_BUILD_TOOLCHAIN)" FISICS_FLAGS="--overlay=$(MEMORY_CHECK_FISICS_OVERLAY)" "$(MEMORY_CHECK_TEST_BIN)"
	mkdir -p "$(MEMORY_CHECK_REPORT_DIR)"
	set +e; FISICS_MEMCHECK_REPORT="$(MEMORY_CHECK_REPORT_POLICY)" "$(MEMORY_CHECK_TEST_BIN)" > "$(MEMORY_CHECK_STDOUT)" 2> "$(MEMORY_CHECK_STDERR)"; status=$$?; \
	echo "memory-check stdout: $(MEMORY_CHECK_STDOUT)"; \
	echo "memory-check stderr: $(MEMORY_CHECK_STDERR)"; \
	echo "memory-check summary:"; \
	grep -E "\\[fisics:memory-check\\] (summary|leak|double free|unknown pointer free)" "$(MEMORY_CHECK_STDERR)" || true; \
	exit $$status
