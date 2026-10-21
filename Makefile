# The declared preparation profile routes ordinary goals through an isolated
# shared-input copy. Recursive producer commands keep their original targets.
ifneq ($(strip $(PREPARATION_ROOT)),)
ifneq ($(PREPARATION_CHILD),1)
PREPARATION_DISPATCH := 1
endif
endif
ifeq ($(PREPARATION_DISPATCH),1)
include make/preparation.mk
else
include make/application.mk
endif
