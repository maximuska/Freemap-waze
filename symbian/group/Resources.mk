# ============================================================================
#
#  Description: This is file for ..
# 
# ============================================================================



SOURCEDIR=..\\..\\resources

CDIR=$(EPOCROOT)epoc32\winscw\c
TARGETDIR=$(CDIR)\private\E001EB29
MAPS_DIR=$(CDIR)\Data\Others\Maps

do_nothing :
	@rem do_nothing

MAKMAKE :  do_nothing

BLD :
ifeq (WINS,$(findstring WINS, $(PLATFORM)))
	echo xcopy /Y /E /D $(SOURCEDIR)\\* $(TARGETDIR)
	if NOT exist $(TARGETDIR) mkdir $(TARGETDIR)
	xcopy /Y /E /D $(SOURCEDIR)\\* $(TARGETDIR)

	if NOT exist $(MAPS_DIR) mkdir $(MAPS_DIR)
	xcopy /Y /E /D ..\\..\\Maps\\* $(MAPS_DIR)
else
	@rem do_nothing
endif

CLEAN :
	del /Q $(TARGETDIR)

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE : do_nothing

FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES : do_nothing

FINAL : do_nothing

