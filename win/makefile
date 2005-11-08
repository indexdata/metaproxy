# Copyright (C) 2005, Index Data ApS
# All rights reserved.
# $Id: makefile,v 1.3 2005-11-08 08:56:39 adam Exp $

# MS NMAKE makefile for yp2

# Parameters that we customize often.
DEBUG=0   # 0 for release, 1 for debug

# YAZ and YAZ++ directories
YAZ_DIR=..\..\yaz
YAZPP_DIR=..\..\yaz++

# Boost
BOOST=\boost
BOOSTVER=1_33

# libxml2 + libxslt + associates
ZLIB_DIR=\zlib-1.2.3.win32
ICONV_DIR=\iconv-1.9.1.win32
LIBXML2_DIR=\libxml2-2.6.20.win32
LIBXSLT_DIR=\libxslt-1.1.14.win32

# The targets

default: all

all: dirs config yaz yazpp libxslt dll progs

progs: ex_libxml2_conf ex_filter_frontend_net test_pipe

NSIS="c:\program files\nsis\makensis.exe"

nsis: all
	$(NSIS) yp2.nsi

# Directories
# The current directory is supposed to be something like
# ..../yaz/win, everything is relative to that
ROOTDIR=..   # The home of our project

INCLDIR=$(ROOTDIR)\include  # our includes
LIBDIR=$(ROOTDIR)\lib       # We produce .lib, .exp etc there
BINDIR=$(ROOTDIR)\bin       # We produce exes and dlls there
WINDIR=$(ROOTDIR)\win       # all these Win make things
OBJDIR=$(WINDIR)\obj        # where we store intermediate files
SRCDIR=$(ROOTDIR)\src       # for the case we move them under src

TMPDIR=$(ROOTDIR)\win\tmp
TMP=$(TMPDIR)

###########################################################
############### Targets - what to make
###########################################################

PROJECT_DLL=$(BINDIR)\yp2.dll
PROJECT_IMPLIB=$(LIBDIR)\yp2.lib
EX_FILTER_FRONTEND_NET=$(BINDIR)\ex_filter_frontend_net.exe
EX_LIBXML2_CONF=$(BINDIR)\ex_libxml2_conf.exe
TEST_PIPE=$(BINDIR)\test_pipe.exe

# shortcut names defined here
dll: $(PROJECT_DLL)
ex_filter_frontend_net: $(EX_FILTER_FRONTEND_NET)
ex_libxml2_conf: $(EX_LIBXML2_CONF)
test_pipe: $(TEST_PIPE)

###########################################################
############### Compiler and linker options 
###########################################################

# YAZ includes & libs
YAZ_LIB="$(YAZ_DIR)\lib\yaz.lib"
YAZ_DEF=/DYAZ_USE_NEW_LOG=1 /I"$(YAZ_DIR)\include"

# Copy yaz.dll to our bin
yaz: $(BINDIR)\yaz.dll
$(BINDIR)\yaz.dll: $(YAZ_DIR)\bin\yaz.dll
  copy "$(YAZ_DIR)\bin\yaz.dll" $(BINDIR)

# YAZ++ includes & libs
YAZPP_LIB="$(YAZPP_DIR)\lib\yazpp.lib"
YAZPP_DEF=/I"$(YAZPP_DIR)\include"

# Copy yazpp.dll to our bin
yazpp: $(BINDIR)\yazpp.dll
$(BINDIR)\yazpp.dll: $(YAZPP_DIR)\bin\yazpp.dll
  copy "$(YAZPP_DIR)\bin\yazpp.dll" $(BINDIR)

# Copy libxslt and associates to our bin directory
libxslt: $(BINDIR)\libxslt.dll

$(BINDIR)\libxslt.dll: $(LIBXSLT_DIR)\bin\libxslt.dll
	copy $(ICONV_DIR)\bin\iconv.dll $(BINDIR)
	copy $(LIBXML2_DIR)\bin\libxml2.dll $(BINDIR)
	copy $(LIBXSLT_DIR)\bin\libxslt.dll $(BINDIR)

config: $(SRCDIR)\config.hpp

$(SRCDIR)\config.hpp:
	echo /*windows*/ >$(SRCDIR)\config.hpp

# Boost 
#
BOOST_DEF = /I"$(BOOST)\include\boost-$(BOOSTVER)"
BOOST_LIB = \
    "$(BOOST)\lib\libboost_date_time-vc71-mt-$(BOOSTVER).lib" \
    "$(BOOST)\lib\libboost_program_options-vc71-mt-$(BOOSTVER).lib" \
    "$(BOOST)\lib\libboost_thread-vc71-mt-$(BOOSTVER).lib" \
    "$(BOOST)\lib\libboost_unit_test_framework-vc71-mt-$(BOOSTVER).lib" 


LIBXSLT_DEF = \
	      /I"$(ICONV_DIR)\include" \
	      /I"$(LIBXML2_DIR)\include" \
	      /I"$(LIBXSLT_DIR)\include"

LIBXSLT_LIB = \
    "$(ICONV_DIR)\lib\iconv.lib" \
    "$(LIBXML2_DIR)\lib\libxml2.lib" \
    "$(LIBXSLT_DIR)\lib\libxslt.lib"

COMMON_C_OPTIONS=          \
  /nologo /W3 /GX /FD /c   \
  $(YAZ_DEF)               \
  $(YAZPP_DEF)             \
  $(BOOST_DEF)             \
  $(LIBXSLT_DEF)	   \
  /D "_WINDOWS"            \
  /D "WIN32"               \
  /EHsc \
  /FR"$(OBJDIR)\\"         \
  /Fo"$(OBJDIR)\\"         \
  /Fd"$(OBJDIR)\\" 	   \
  /I"$(ROOTDIR)\include" 

DEBUG_C_OPTIONS=  \
  /D "_DEBUG"      \
  /MDd  /Od /YX /Zi /Gm

RELEASE_C_OPTIONS=  \
  /D "NDEBUG"        \
  /MD /O2

# /W3  = warning level
# /GX  = Enable exception handling
# /FD  = Generate file dependencies (what ever they are)
# /c   = compile without linking
# /FR  = Generate browse info (.sbr file that gets combined into .bsc)
# /Fo  = object file name (or at least path)
# /Fd  = debug database name (or path)
# /MD  = Runtime library: Multithread DLL
# /MDd = Runtime library: Multithread DLL (debug)
# /Od  = Disable optimising (debug)
# /O2  = Optimize for speed
# /YX  = Automatic use of precomipled headers
# /Gm  = Minimal rebuild (some cpp class stuff)
# /Zi  = Program database for debuggers
# /ZI  = Pgm database with special "edit&continue" stuff - not available in C5


### Linker options
LINK=link.exe

LINK_LIBS= kernel32.lib user32.lib   gdi32.lib   \
           advapi32.lib uuid.lib \
           wsock32.lib  advapi32.lib \
	   $(BOOST_LIB) \
           $(YAZ_LIB) $(YAZPP_LIB) $(ICONV_LIB) $(LIBXSLT_LIB) \
	   $(BOOST_LIB) 

COMMON_LNK_OPTIONS= /nologo \
                  /subsystem:windows \
                  /machine:i386 \
		  /incremental:no

DEBUG_LNK_OPTIONS= /debug 

RELEASE_LNK_OPTIONS=

DLL_LINK_OPTIONS= /dll  
PROG_LINK_OPTIONS = /subsystem:console  

# Final opt variables
!if $(DEBUG)
COPT=   $(COMMON_C_OPTIONS)   $(DEBUG_C_OPTIONS)
RCOPT=  $(COMMON_RC_OPTIONS)  $(DEBUG_RC_OPTIONS)
LNKOPT= $(COMMON_LNK_OPTIONS) $(DEBUG_LNK_OPTIONS) 

!else
COPT=   $(COMMON_C_OPTIONS)   $(RELEASE_C_OPTIONS) 
RCOPT=  $(COMMON_RC_OPTIONS)  $(RELEASE_RC_OPTIONS)
LNKOPT= $(COMMON_LNK_OPTIONS) $(RELEASE_LNK_OPTIONS)
!endif

###########################################################
###############  Source and object modules
###########################################################

PROJECT_DLL_OBJS = \
        $(OBJDIR)\session.obj \
        $(OBJDIR)\router_chain.obj \
        $(OBJDIR)\router_flexml.obj \
        $(OBJDIR)\filter.obj \
	$(OBJDIR)\filter_frontend_net.obj \
        $(OBJDIR)\filter_log.obj \
        $(OBJDIR)\filter_virt_db.obj \
        $(OBJDIR)\filter_z3950_client.obj \
        $(OBJDIR)\filter_backend_test.obj \
	$(OBJDIR)\thread_pool_observer.obj \
	$(OBJDIR)\pipe.obj \
        $(OBJDIR)\util.obj 

EX_FILTER_FRONTEND_NET_OBJS = $(OBJDIR)\ex_filter_frontend_net.obj
EX_LIBXML2_CONF_OBJS = $(OBJDIR)\ex_libxml2_conf.obj
TEST_PIPE_OBJS = $(OBJDIR)\test_pipe.obj

###########################################################
############### Compiling 
###########################################################

# Note: This defines where to look for the necessary
# source files. Funny way of doing it, but it works.

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	$(CPP) $(COPT) $< 

###########################################################
############### Resources
###########################################################

### The RC compiler (resource files)
RSC=rc.exe
COMMON_RC_OPTIONS= /l 0x406 /i"$(ROOTDIR)" 
DEBUG_RC_OPTIONS=/d "_DEBUG"
RELEASE_RC_OPTIONS=/d "NDEBUG"

PROJECT_RES=$(OBJDIR)\yp2.res
PROJECT_RC=$(WINDIR)\yp2.rc

!if $(DEBUG)
RSOPT=/d_DEBUG
!else
RSOPT=/d_NDEBUG
!endif

$(PROJECT_RES): $(PROJECT_RC)
	$(RSC) $(RSOPT) /fo"$(PROJECT_RES)" $(PROJECT_RC) 

###########################################################
############### Linking
###########################################################

$(PROJECT_DLL) $(PROJECT_IMPLIB): "$(BINDIR)" $(PROJECT_DLL_OBJS)
	@echo Linking $(PROJECT_DLL)
	$(LINK) $(LNKOPT) $(LINK_LIBS) $(DLL_LINK_OPTIONS) $(PROJECT_DLL_OBJS) /out:"$(PROJECT_DLL)" /implib:"$(PROJECT_IMPLIB)"

$(EX_FILTER_FRONTEND_NET) : "$(BINDIR)" $(EX_FILTER_FRONTEND_NET_OBJS) $(PROJECT_IMPLIB)
	@echo Linking $(EX_FILTER_FRONTEND_NET)
	$(LINK) $(LNKOPT) $(PROG_LINK_OPTIONS) $(LINK_LIBS) $(PROJECT_DLL_OBJS) $(EX_FILTER_FRONTEND_NET_OBJS) /out:$(EX_FILTER_FRONTEND_NET) 

$(EX_LIBXML2_CONF) : "$(BINDIR)" $(EX_LIBXML2_CONF_OBJS) $(PROJECT_DLL_OBJS)
	@echo Linking $(EX_LIBXML2_CONF)
	$(LINK) $(LNKOPT) $(PROG_LINK_OPTIONS) $(LINK_LIBS) $(PROJECT_IMPLIB) $(EX_LIBXML2_CONF_OBJS) /out:$(EX_LIBXML2_CONF)

$(TEST_PIPE) : "$(BINDIR)" $(TEST_PIPE_OBJS) $(PROJECT_IMPLIB)
	@echo Linking $(TEST_PIPE)
	$(LINK) $(LNKOPT) $(PROG_LINK_OPTIONS) $(LINK_LIBS) $(TEST_PIPE_OBJS) $(PROJECT_DLL_OBJS) /out:$(TEST_PIPE)

############## clean
clean:
	-del $(BINDIR)\*.exe
	-del $(BINDIR)\*.dll
	-del $(TMPDIR)\*.
	-del $(LIBDIR)\*.MAP
	-del $(LIBDIR)\*.LIB
	-del $(OBJDIR)\*.OBJ

realclean: clean
	-del $(ZOOMDIR)\zoom.h

dirs: $(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR)

$(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR):
	if not exist "$@/$(NUL)" mkdir "$@"

