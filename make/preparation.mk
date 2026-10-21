# Bounded entrypoint for the snapshot_sandbox_v1 producer declaration.
PREPARATION_GOALS := $(if $(MAKECMDGOALS),$(MAKECMDGOALS),all)
PREPARATION_ALLOWED := all release-contract run-headless-smoke release-package-self-test release-artifact-disposable package-desktop-main-edit-self-test
ifneq ($(filter-out $(PREPARATION_ALLOWED),$(PREPARATION_GOALS)),)
$(error Unsupported contained preparation goal: $(filter-out $(PREPARATION_ALLOWED),$(PREPARATION_GOALS)))
endif
.DEFAULT_GOAL := preparation-dispatch
.PHONY: preparation-dispatch $(PREPARATION_GOALS)
$(PREPARATION_GOALS): preparation-dispatch
preparation-dispatch:
	@test "$(abspath $(BUILD_ROOT))" = "$(abspath $(PREPARATION_ROOT))" || (echo "Preparation/build roots differ"; exit 1)
	@python3 tools/packaging/prepare_shared_copy.py --source third_party/codework_shared --output "$(PREPARATION_ROOT)/preparation-shared"
	@$(MAKE) PREPARATION_CHILD=1 PREPARATION_DISPATCH=0 SHARED_ROOT="$(abspath $(PREPARATION_ROOT))/preparation-shared" $(PREPARATION_GOALS)
