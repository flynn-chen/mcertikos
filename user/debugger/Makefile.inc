# -*-Makefile-*-

OBJDIRS		+= $(USER_OBJDIR)/debugger

USER_DEBUGGER_SRC	+= $(USER_DIR)/debugger/debugger.c
USER_DEBUGGER_OBJ	:= $(patsubst %.c, $(OBJDIR)/%.o, $(USER_DEBUGGER_SRC))
USER_DEBUGGER_OBJ	:= $(patsubst %.S, $(OBJDIR)/%.o, $(USER_DEBUGGER_OBJ))
KERN_BINFILES	+= $(USER_OBJDIR)/debugger/debugger

debugger: $(USER_OBJDIR)/debugger/debugger

$(USER_OBJDIR)/debugger/debugger: $(USER_LIB_OBJ) $(USER_DEBUGGER_OBJ)
	@echo + ld[USER/debugger] $@
	$(V)$(LD) -o $@ $(USER_LDFLAGS) $(USER_LIB_OBJ) $(USER_DEBUGGER_OBJ) $(GCC_LIBS)
	$(V)$(OBJDUMP) -S $@ > $@.asm
	$(V)$(NM) -n $@ > $@.sym

$(USER_OBJDIR)/debugger/%.o: $(USER_DIR)/debugger/%.c
	@echo + cc[USER/debugger] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(USER_CFLAGS) -c -o $@ $<

$(USER_OBJDIR)/debugger/%.o: $(USER_DIR)/debugger/%.S
	@echo + as[USER/debugger] $<
	@mkdir -p $(@D)
	$(V)$(CC) $(USER_CFLAGS) -c -o $@ $<