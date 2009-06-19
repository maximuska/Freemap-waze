# ============================================================================
#
#  Description: This is file for copying resoures 
#   required for the program to run correctly in WINSCW emulator. 
# ============================================================================


ifeq (WINS,$(findstring WINS, $(PLATFORM)))
CDIR=$(EPOCROOT)epoc32\winscw\c\Data\Freemap
ZDIR=$(EPOCROOT)epoc32\release\$(PLATFORM)\udeb\Z
endif

SOURCEDIR=..\..\resources
TARGETDIR=$(ZDIR)\private\E001EB29

do_nothing :
	@rem do_nothing

MAKMAKE :  do_nothing

BLD :
ifeq (WINS,$(findstring WINS, $(PLATFORM)))
	echo xcopy /Y /E /I $(SOURCEDIR)\* $(TARGETDIR)

	if NOT exist $(TARGETDIR)\\quick.menu xcopy /Y $(SOURCEDIR)\* $(TARGETDIR)
	if NOT exist $(TARGETDIR)\\Skins xcopy /Y /E /I $(SOURCEDIR)\\Skins $(TARGETDIR)\\Skins

	if NOT exist $(CDIR)\\Maps  xcopy /Y /E /I ..\\..\\Maps $(CDIR)\\Maps
	if NOT exist $(CDIR)\\Sound	xcopy /Y /E /I $(SOURCEDIR)\\Sound $(CDIR)\\Sound
else
	@rem do_nothing
endif

CLEAN :
	del /Q $(TARGETDIR) $(CDIR)

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE :	do_nothing

FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES : do_nothing

FINAL : do_nothing

