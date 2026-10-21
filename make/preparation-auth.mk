# Stay in the same Make graph so the native owner's -o checkpoint flags survive.
# Only the initial build creates inputs; resumed signed/notary/staple prerequisites
# are skipped by the owner, never inferred here from a target name.
.PHONY: preparation-auth-inputs
preparation-auth-inputs:
	@test "$(abspath $(BUILD_ROOT))" = "$(abspath $(PREPARATION_ROOT))" || (echo "Preparation/build roots differ"; exit 1)
	@python3 tools/packaging/prepare_shared_copy.py --source third_party/codework_shared --output "$(SHARED_ROOT)"
release-build: preparation-auth-inputs
