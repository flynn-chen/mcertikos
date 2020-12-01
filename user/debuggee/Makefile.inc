# -*-Makefile-*-

OBJDIRS		+= $(USER_OBJDIR)/debuggee

USER_DEBUGGEE_SRC	+= $(USER_DIR)/debuggee/debuggee.c
USER_DEBUGGEE_OBJ	:= $(patsubst %.c, $(OBJDIR)/%.o, $(USER_DEBUGGEE_SRC))
USER_DEBUGGEE_OBJ	:= $(patsubst %.S, $(OBJDIR)/%.o, $(USER_DEBUGGEE_OBJ))
KERN_BINFILES	+= $(USER_OBJDIR)/debuggee/debuggee

debuggee: $(USER_OBJDIR)/debuggee/debuggee

$(USER_OBJDIR)/debuggee/debuggee: $(USER_LIB_OBJ) $(USER_DEBUGGEE_OBJ)
	@echo + ld[USER/debuggee] $@
	$(V)$(LD) -o $@ $(USER_LDFLAGS) $(USER_LIB_OBJ) $(USER_DEBUGGEE_OBJ) $(GCC_LIBS)
	$(V)$(OBJDUMP) -S $@ > $@.asm
	$(V)$(NM) -n $@ > $@.sym

$(USER_OBJDIR)/debuggee/%.o: $(USER_DIR)/debuggee/%.c
	@echo + cc[USER/debuggee] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(USER_CFLAGS) -c -o $@ $<

$(USER_OBJDIR)/debuggee/%.o: $(USER_DIR)/debuggee/%.S
	@echo + as[USER/debuggee] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(USER_CFLAGS) -c -o $@ $<