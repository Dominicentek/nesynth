include config.mk

task = $(info ::setmsg $(1))
is_true = $(if $(filter-out 0,$(1)),1,)
define sources
	$(eval $(2)_SRC := $(1))
	$(eval $(2)_OBJ := $(patsubst %.c,$(BUILD)/%.o,$(1)))
endef

$(call sources,$(shell find $(FRONTEND) -name "*.c"),FRONTEND)
$(call sources,nesynth.c,NESYNTH)

LFLAGS_FRONTEND = $(LFLAGS)
ifeq ($(OS),Windows_NT)
	CFLAGS += -DWINDOWS
	LFLAGS_FRONTEND += $(shell pkgconf --static --libs $(LIBS)) -static
	FRONTEND_TARGET = $(NAME).exe
	LIBRARY_TARGET = $(NAME).dll
else
	LFLAGS_FRONTEND += $(shell pkgconf --libs sdl3)
	FRONTEND_TARGET = $(NAME)
	LIBRARY_TARGET = lib$(NAME).so
endif

.PHONY: all clean

all: $(LIBRARY_TARGET) $(if $(call is_true,$(FRONTEND_ENABLE)),$(FRONTEND_TARGET),)

clean:
	$(call task,Cleaning up)
	rm -rf $(LIBRARY_TARGET) $(FRONTEND_TARGET) $(BUILD)

-include $(NESYNTH_OBJ:.o=.d) $(FRONTEND_OBJ:.o=.d)

$(BUILD)/%.o: %.c
	$(call task,Compiling $<)
	mkdir -p $(dir $@)
	::compile -c $(CFLAGS) -o $@ $<
	::compile $(CFLAGS) -MM -MT $@ $< -o $(@:.o=.d)

$(FRONTEND_TARGET): $(FRONTEND_OBJ) $(LIBRARY_TARGET)
	$(call task,Linking frontend)
	::compile $(FRONTEND_OBJ) $(if $(call is_true,$(FRONTEND_LINK_STATICALLY)),$(NESYNTH_OBJ),-L. -l$(NAME)) $(LFLAGS_FRONTEND) -o $@

$(LIBRARY_TARGET): $(NESYNTH_OBJ)
	$(call task,Linking library)
	::compile $(NESYNTH_OBJ) $(LFLAGS) -shared -o $@
