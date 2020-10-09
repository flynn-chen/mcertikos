# -*-Makefile-*-

OBJDIRS += $(KERN_OBJDIR)/vmm/MPTCopy

KERN_SRCFILES += $(KERN_DIR)/vmm/MPTCopy/MPTCopy.c

$(KERN_OBJDIR)/vmm/MPTCopy/%.o: $(KERN_DIR)/vmm/MPTCopy/%.c
	@echo + $(COMP_NAME)[KERN/vmm/MPTCopy] $<
	@mkdir -p $(@D)
	$(V)$(CCOMP) $(CCOMP_KERN_CFLAGS) -c -o $@ $<

$(KERN_OBJDIR)/vmm/MPTCopy/%.o: $(KERN_DIR)/vmm/MPTCopy/%.S
	@echo + as[KERN/vmm/MPTCopy] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(KERN_CFLAGS) -c -o $@ $<
