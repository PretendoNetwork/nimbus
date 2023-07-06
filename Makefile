.PHONY: all clean

OUT_FOLDER      := out

CIA_OUT_FOLDER  := $(OUT_FOLDER)/cia_out
3DSX_OUT_FOLDER := $(OUT_FOLDER)/3dsx_out

3DS_OUT	        := 3ds
CIA_OUT         := cias

LUMA_OUT	    := luma/titles

FRIENDS_TITLE_ID	:= 0004013000003202
ACT_TITLE_ID	:= 0004013000003802
SSL_TITLE_ID    := 0004013000002F02
MIIVERSE_ID_JPN     := 000400300000BC02
MIIVERSE_ID_USA     := 000400300000BD02
MIIVERSE_ID_EUR     := 000400300000BE02

FRIENDS_OUT     := $(LUMA_OUT)/$(FRIENDS_TITLE_ID)
ACT_OUT         := $(LUMA_OUT)/$(ACT_TITLE_ID)
SSL_OUT         := $(LUMA_OUT)/$(SSL_TITLE_ID)
MIIVERSE_OUT_JPN    := $(LUMA_OUT)/$(MIIVERSE_ID_JPN)
MIIVERSE_OUT_USA    := $(LUMA_OUT)/$(MIIVERSE_ID_USA)
MIIVERSE_OUT_EUR    := $(LUMA_OUT)/$(MIIVERSE_ID_EUR)

all:
	@rm -rf $(OUT_FOLDER)

	@mkdir -p $(3DSX_OUT_FOLDER)/$(FRIENDS_OUT) $(3DSX_OUT_FOLDER)/$(ACT_OUT)
	@mkdir -p $(3DSX_OUT_FOLDER)/$(SSL_OUT) $(3DSX_OUT_FOLDER)/$(MIIVERSE_OUT_JPN)
	@mkdir -p $(3DSX_OUT_FOLDER)/$(MIIVERSE_OUT_USA) $(3DSX_OUT_FOLDER)/$(MIIVERSE_OUT_EUR)
	@mkdir -p $(3DSX_OUT_FOLDER)/$(3DS_OUT) $(CIA_OUT_FOLDER)
	@mkdir -p $(CIA_OUT_FOLDER)/$(CIA_OUT)
	
	@$(MAKE) -C patches
	
	@cp -r patches/friends/out/* $(3DSX_OUT_FOLDER)/$(FRIENDS_OUT)
	@cp -r patches/act/out/* $(3DSX_OUT_FOLDER)/$(ACT_OUT)
	@cp -r patches/ssl/out/* $(3DSX_OUT_FOLDER)/$(SSL_OUT)
	@cp -r patches/miiverse/out/* $(3DSX_OUT_FOLDER)/$(MIIVERSE_OUT_JPN)
	@cp -r patches/miiverse/out/* $(3DSX_OUT_FOLDER)/$(MIIVERSE_OUT_USA)
	@cp -r patches/miiverse/out/* $(3DSX_OUT_FOLDER)/$(MIIVERSE_OUT_EUR)
	@cp -r patches/miiverse/*.pem $(3DSX_OUT_FOLDER)/$(3DS_OUT)
	
	@cp -r $(3DSX_OUT_FOLDER)/* $(CIA_OUT_FOLDER)

	@$(MAKE) -C app 3dsx
	@echo copied 3dsx to 3dsx out folder...
	@cp app/*.3dsx $(3DSX_OUT_FOLDER)/$(3DS_OUT)
	
	@$(MAKE) -C app cia
	@echo copied cia to cia out folder...
	@cp app/*.cia $(CIA_OUT_FOLDER)/$(CIA_OUT)

clean:
	@$(MAKE) -C patches clean
	@$(MAKE) -C app clean
	@rm -rf $(OUT_FOLDER)
