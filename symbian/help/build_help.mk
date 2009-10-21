# ============================================================================
#  Name	 : build_help.mk
#  Part of  : FreeMap
#
#  Description: This make file will build the application help file (.hlp)
# 
# ============================================================================

do_nothing :
	@rem do_nothing

MAKMAKE :
	cshlpcmp FreeMap.cshlp
ifeq (WINS,$(findstring WINS, $(PLATFORM)))
	copy FreeMap.hlp $(EPOCROOT)epoc32\$(PLATFORM)\c\resource\help
endif

BLD : do_nothing

CLEAN :
	del FreeMap.hlp
	del FreeMap.hlp.hrh

LIB : do_nothing

CLEANLIB : do_nothing

RESOURCE : do_nothing

FREEZE : do_nothing

SAVESPACE : do_nothing

RELEASABLES :
	@echo FreeMap.hlp

FINAL : do_nothing
