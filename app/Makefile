#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# GRAPHICS is a list of directories containing graphics files
# GFXBUILD is the directory where converted graphics files will be placed
#   If set to $(BUILD), it will statically link in the converted
#   files as if they were data files.
#
# NO_SMDH: if set to anything, no SMDH file is generated.
# ROMFS is the directory which contains the RomFS, relative to the Makefile (Optional)
# APP_TITLE is the name of the app stored in the SMDH file (Optional)
# APP_DESCRIPTION is the description of the app stored in the SMDH file (Optional)
# APP_AUTHOR is the author of the app stored in the SMDH file (Optional)
# ICON is the filename of the icon (.png), relative to the project folder.
#   If not set, it attempts to use one of the following (in this order):
#     - <Project name>.png
#     - icon.png
#     - <libctru folder>/default_icon.png
#---------------------------------------------------------------------------------
APP_TITLE       := 	Nimbus
APP_DESCRIPTION := 	Nimbus
APP_AUTHOR	:=	Zaksabeast, shutterbug2000
TARGET		:=	nimbus
BUILD		:=	build
SOURCES		:=	source source/states source/sysmodules
DATA		:=	data
INCLUDES	:=	include
ROMFS		:=	romfs
GRAPHICS	:=	$(ROMFS)/gfx
GFXBUILD 	:=  $(BUILD)

RSF_FILE := meta/nimbus-cia.rsf

ICON      := meta/icon.png
BNR_IMAGE := meta/banner.png
BNR_AUDIO := meta/audio.wav

VERSION_HEADER = version.hpp

VERSION_MAJOR := 1
VERSION_MINOR := 6
VERSION_MICRO := 1

#GFXBUILD	:=	$(ROMFS)/gfx

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
OPTIMIZE    :=  -O2
ARCH	    :=	-march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS	    :=	-g -Wall $(OPTIMIZE) -mword-relocations \
			    -ffunction-sections \
			    $(ARCH)

CFLAGS	    +=	$(INCLUDE) -D__3DS__

CXXFLAGS	:=  $(CFLAGS) -fno-rtti -fno-exceptions -std=c++20

ASFLAGS	    :=	-g $(ARCH)
LDFLAGS	     =	-specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map) $(OPTIMIZE)

LIBS	    := -lcitro2d -lcitro3d -lctru -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:= $(CTRULIB)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export TOPDIR	:=	$(CURDIR)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
SHLISTFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.shlist)))
GFXFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.t3s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

#---------------------------------------------------------------------------------
ifeq ($(GFXBUILD),$(BUILD))
#---------------------------------------------------------------------------------
export T3XFILES :=  $(GFXFILES:.t3s=.t3x)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
export ROMFS_T3XFILES	:=	$(patsubst %.t3s, $(GFXBUILD)/%.t3x, $(GFXFILES))
export T3XHFILES		:=	$(patsubst %.t3s, $(BUILD)/%.h, $(GFXFILES))
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES_SOURCES 	:=	$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export OFILES_BIN	:=	$(addsuffix .o,$(BINFILES)) \
			$(PICAFILES:.v.pica=.shbin.o) $(SHLISTFILES:.shlist=.shbin.o) \
			$(addsuffix .o,$(T3XFILES))

export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)

export HFILES	:=	$(PICAFILES:.v.pica=_shbin.h) $(SHLISTFILES:.shlist=_shbin.h) \
			$(addsuffix .h,$(subst .,_,$(BINFILES))) \
			$(GFXFILES:.t3s=.h)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

export _3DSXDEPS	:=	$(if $(NO_SMDH),,$(OUTPUT).smdh)

ifeq ($(strip $(ICON)),)
	icons := $(wildcard *.png)
	ifneq (,$(findstring $(TARGET).png,$(icons)))
		export APP_ICON := $(TOPDIR)/$(TARGET).png
	else
		ifneq (,$(findstring icon.png,$(icons)))
			export APP_ICON := $(TOPDIR)/icon.png
		endif
	endif
else
	export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_SMDH)),)
	export _3DSXFLAGS += --smdh=$(CURDIR)/$(TARGET).smdh
endif

ifneq ($(ROMFS),)
	export _3DSXFLAGS += --romfs=$(CURDIR)/$(ROMFS)
endif

.PHONY: all clean

#---------------------------------------------------------------------------------
all: $(BUILD) $(GFXBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES)
	@'$(MAKE)' --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

$(BUILD):
	@mkdir -p $@

ifneq ($(GFXBUILD),$(BUILD))
$(GFXBUILD):
	@mkdir -p $@
endif

ifneq ($(DEPSDIR),$(BUILD))
$(DEPSDIR):
	@mkdir -p $@
endif

#---------------------------------------------------------------------------------
$(GFXBUILD)/%.t3x $(BUILD)/%.h: %.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $(BUILD)/$*.h -d $(DEPSDIR)/$*.d -o $(GFXBUILD)/$*.t3x

3dsx: $(BUILD) $(GFXBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES)
	@'$(MAKE)' --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile 3dsx

cia: $(BUILD) $(GFXBUILD) $(DEPSDIR) $(ROMFS_T3XFILES) $(T3XHFILES)
	@'$(MAKE)' --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile cia

3dslink: 3dsx
	@3dslink $(OUTPUT).3dsx

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@$(RM) -r $(BUILD) \
		$(TARGET).3dsx \
		$(OUTPUT).smdh \
		$(TARGET).elf \
		$(TARGET).cia

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(VERSION_HEADER) $(OUTPUT).3dsx $(OUTPUT).cia

3dsx: $(VERSION_HEADER) $(OUTPUT).3dsx

cia: $(VERSION_HEADER) $(OUTPUT).cia

ifeq ($(strip $(NO_SMDH)),)
.PHONY: $(VERSION_HEADER) all
all:            $(OUTPUT).3dsx $(OUTPUT).smdh
$(OUTPUT).smdh: $(TOPDIR)/Makefile $(TOPDIR)/Makefile
$(OUTPUT).3dsx: $(OUTPUT).smdh
endif

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)

$(OFILES_SOURCES): $(HFILES)

$(OUTPUT).elf: $(OFILES)

$(VERSION_HEADER):
	@echo generating version header...
	@mkdir -p $(BUILD)  # Create the build directory if it doesn't exist
	@echo "// Compile time constants" > $@.new
	@echo "" >> $@.new
	@echo "#define APP_TITLE \"$(APP_TITLE)\"" >> $@.new
	@echo "" >> $@.new
	@echo "#define VERSION_MAJOR $(VERSION_MAJOR)" >> $@.new
	@echo "#define VERSION_MINOR $(VERSION_MINOR)" >> $@.new
	@echo "#define VERSION_MICRO $(VERSION_MICRO)" >> $@.new
	@if ! cmp -s $@ $@.new; then mv $@.new $@; else rm $@.new; fi
	
#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o	%_bin.h :	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
.PRECIOUS	:	%.t3x
#---------------------------------------------------------------------------------
%.t3x.o	%_t3x.h :	%.t3x
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
# rules for assembling GPU shaders
#---------------------------------------------------------------------------------
define shader-as
	$(eval CURBIN := $*.shbin)
	$(eval DEPSFILE := $(DEPSDIR)/$*.shbin.d)
	echo "$(CURBIN).o: $< $1" > $(DEPSFILE)
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"_end[];" > `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u8" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`"[];" >> `(echo $(CURBIN) | tr . _)`.h
	echo "extern const u32" `(echo $(CURBIN) | sed -e 's/^\([0-9]\)/_\1/' | tr . _)`_size";" >> `(echo $(CURBIN) | tr . _)`.h
	picasso -o $(CURBIN) $1
	bin2s $(CURBIN) | $(AS) -o $*.shbin.o
endef

%.shbin.o %_shbin.h : %.v.pica %.g.pica
	@echo $(notdir $^)
	@$(call shader-as,$^)

%.shbin.o %_shbin.h : %.v.pica
	@echo $(notdir $<)
	@$(call shader-as,$<)

%.shbin.o %_shbin.h : %.shlist
	@echo $(notdir $<)
	@$(call shader-as,$(foreach file,$(shell cat $<),$(dir $<)$(file)))

#---------------------------------------------------------------------------------
%.t3x	%.h	:	%.t3s
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@tex3ds -i $< -H $*.h -d $*.d -o $*.t3x

$(OUTPUT).cia:  $(OUTPUT).elf $(OUTPUT).smdh $(TARGET).bnr $(TOPDIR)/$(RSF_FILE)
	@makerom -f cia -target t -exefslogo -o $@ \
	  -elf $(OUTPUT).elf -rsf $(TOPDIR)/$(RSF_FILE) \
	  -ver "$$(($(VERSION_MAJOR)*1024+$(VERSION_MINOR)*16+$(VERSION_MICRO)))" \
	  -banner $(TARGET).bnr \
	  -icon $(OUTPUT).smdh
	@echo "built ... $(notdir $@)"

$(TARGET).bnr:  $(TOPDIR)/$(BNR_IMAGE) $(TOPDIR)/$(BNR_AUDIO)
	@[ -d $(dir $@) ] || mkdir -p $(dir $@)
	@bannertool makebanner -o $@ -i $(TOPDIR)/$(BNR_IMAGE) -a $(TOPDIR)/$(BNR_AUDIO)
	@echo "built ... $@"

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
