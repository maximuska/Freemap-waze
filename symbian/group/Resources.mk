# ============================================================================
#
#  Description: This is file for ..
# 
# ============================================================================


ifeq (WINS,$(findstring WINS, $(PLATFORM)))
CDIR=$(EPOCROOT)epoc32\winscw\c\Data\Others\Maps
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\udeb\Z
else
ZDIR=$(EPOCROOT)epoc32\data\z
endif

SOURCEDIR=..\\..\\resources
TARGETDIR=$(ZDIR)\private\E001EB29

do_nothing :
	@rem do_nothing

MAKMAKE :  do_nothing

BLD :
ifeq (WINS,$(findstring WINS, $(PLATFORM)))
	echo xcopy /Y /E $(SOURCEDIR)\\* $(TARGETDIR)
	if NOT exist $(TARGETDIR) mkdir $(TARGETDIR)
	xcopy /Y /E $(SOURCEDIR)\\* $(TARGETDIR)

	if NOT exist $(CDIR) mkdir $(CDIR)
	xcopy /Y /E ..\\..\\Maps\\* $(CDIR)
else
	@rem do_nothing
endif

CLEAN :
	del /Q $(TARGETDIR)

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE :	do_nothing

FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES : do_nothing

FINAL : do_nothing

