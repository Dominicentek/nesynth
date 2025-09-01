include config.mk

is_true = $(if $(filter-out 0,$(1)),1,)
define sources
	$(eval $(2)_SRC := $(1))
	$(eval $(2)_OBJ := $(patsubst %.c,$(BUILD)/%.o,$(1)))
endef

$(call sources,$(shell find $(FRONTEND) -name "*.c"),FRONTEND)
$(call sources,nesynth.c,NESYNTH)

IMAGES = $(shell find $(IMG_SOURCE) -name "*.png")
IMAGES_H = $(patsubst $(IMG_SOURCE)/%,$(IMG_TARGET)/%.h,$(IMAGES))

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
	::setmsg Cleaning up
	rm -rf $(LIBRARY_TARGET) $(FRONTEND_TARGET) $(IMG_TARGET)/* $(BUILD)

-include $(NESYNTH_OBJ:.o=.d) $(FRONTEND_OBJ:.o=.d)

$(IMG_TARGET)/%.png.h: $(IMG_SOURCE)/%.png
	::setmsg Registering image $<
	mkdir -p $(dir $@)
	echo "{ $$(wc -c < $<), \"$<\", (char[]){" > $@
	hexdump -e '16/1 "0x%02X," "\n"' $< | sed 's/0x\x20\x20,//g' >> $@
	echo "}}," >> $@

$(IMG_TARGET)/all.h: $(IMAGES_H)
	::setmsg Merging images
	echo "" > $@
	for i in $^; do echo "#include \"$$i\"" >> $@; done

$(BUILD)/$(FRONTEND)/imageloader/imageloader.o: $(IMG_TARGET)/all.h

$(BUILD)/%.o: %.c
	::setmsg Compiling $<
	mkdir -p $(dir $@)
	::compile -c $(CFLAGS) -o $@ $<
	::compile $(CFLAGS) -MM -MT $@ $< -o $(@:.o=.d)

$(FRONTEND_TARGET): $(FRONTEND_OBJ) $(LIBRARY_TARGET)
	::setmsg Linking frontend
	::compile $(FRONTEND_OBJ) $(if $(call is_true,$(FRONTEND_LINK_STATICALLY)),$(NESYNTH_OBJ),-L. -l$(NAME)) $(LFLAGS_FRONTEND) -o $@

$(LIBRARY_TARGET): $(NESYNTH_OBJ)
	::setmsg Linking library
	::compile $(NESYNTH_OBJ) $(LFLAGS) -shared -o $@
