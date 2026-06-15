include make/config.mk
include make/target.mk
include make/shared.mk
include make/flags.mk
include make/paths.mk
include make/sources.mk
include make/objects.mk

.PHONY: all clean run run-demo vk-renderer-lib test run-headless-smoke run-data-path-contract-checks run-graph-contract-checks run-detail-relationship-contract-checks run-relationship-mutation-test run-browse-filter-contract-checks visual-harness memory-check-build memory-check-run memory-check-audit package-build-lane package-desktop package-desktop-smoke package-desktop-self-test package-desktop-copy-desktop package-desktop-sync package-desktop-open package-desktop-remove package-desktop-refresh release-contract release-clean release-build release-bundle-audit release-sign release-verify release-verify-signed release-notarize release-staple release-verify-notarized release-artifact release-distribute release-desktop-refresh FORCE

include make/rules-memory-check.mk
include make/rules-build.mk
include make/rules-test.mk
include make/package-macos.mk
include make/release.mk
