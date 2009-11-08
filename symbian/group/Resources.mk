# ============================================================================
#
#  Description: This is file for copying resoures 
#   required for the program to run correctly in WINSCW emulator. 
# ============================================================================



SOURCEDIR=..\\..\\resources

CDIR=$(EPOCROOT)epoc32\winscw\c
TARGETDIR=$(CDIR)\private\E001EB29
FREEMAP_DIR=$(CDIR)\Data\Freemap

do_nothing :
	@rem do_nothing

MAKMAKE :  do_nothing

BLD :
ifeq (WINS,$(findstring WINS, $(PLATFORM)))
	echo xcopy /YEID $(SOURCEDIR)\* $(TARGETDIR)
	if NOT exist $(TARGETDIR) mkdir $(TARGETDIR)
	xcopy /YID $(SOURCEDIR)\\* $(TARGETDIR)

	xcopy /YEID $(SOURCEDIR)\\Skins $(TARGETDIR)\\Skins

	xcopy /YEID ..\\..\\Maps $(FREEMAP_DIR)\\Maps
	xcopy /YEID $(SOURCEDIR)\\Sound $(FREEMAP_DIR)\\Sound
	
	del $(TARGETDIR)\preferences
	xcopy /YID $(SOURCEDIR)\\preferences $(FREEMAP_DIR)\\
else
	@rem do_nothing
endif

CLEAN :
	del /Q $(TARGETDIR) $(CDIR)

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE : do_nothing

FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES : do_nothing

FINAL : do_nothing

