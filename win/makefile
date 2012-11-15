# Copyright (C) 2005-2012 Index Data
# All rights reserved.

# MS NMAKE makefile for metaproxy

# Parameters that we customize often.
DEBUG=0   # 0 for release, 1 for debug

# Metaproxy version
VERSION=1.3.51

# YAZ and YAZ++ directories
YAZ_DIR=..\..\yaz
YAZPP_DIR=..\..\yazpp

# Boost
BOOST=\program files\boost\boost_1_51
BOOST_VERSION=1_51
BOOST_TOOLSET=vc90 # vc90 for VS2008, vc80 for VS 2005, vc71 for 2003
BOOST_MODEL=mt

# libxml2 + libxslt + associates
ZLIB_DIR=\zlib-1.2.3.win32
ICONV_DIR=\iconv-1.9.2.win32
LIBXML2_DIR=\libxml2-2.7.6.win32
LIBXSLT_DIR=\libxslt-1.1.26.win32

# The targets

default: all

all: dirs config yaz yazpp libxslt progs

progs: metaproxy ex_filter_frontend_net # test_pipe does not work
NSIS="c:\program files\nsis\makensis.exe"

nsis: all
	$(NSIS) metaproxy.nsi

# Directories
# The current directory is supposed to be something like
# ..../yaz/win, everything is relative to that
ROOTDIR=..   # The home of our project

INCLDIR=$(ROOTDIR)\include  # our includes
LIBDIR=$(ROOTDIR)\lib       # We produce .lib, .exp etc there
BINDIR=$(ROOTDIR)\bin       # We produce exes and dlls there
WINDIR=$(ROOTDIR)\win       # all these Win make things
SRCDIR=$(ROOTDIR)\src       # for the case we move them under src

!if $(DEBUG)
OBJDIR=$(WINDIR)\dobj       # where we store intermediate files
!else
OBJDIR=$(WINDIR)\obj        # where we store intermediate files
!endif

TMPDIR=$(ROOTDIR)\win\tmp
TMP=$(TMPDIR)


##########################################################
############### Targets - what to make
###########################################################

EX_FILTER_FRONTEND_NET=$(BINDIR)\ex_filter_frontend_net.exe
METAPROXY_PROG=$(BINDIR)\metaproxy.exe
TEST_PIPE=$(BINDIR)\test_pipe.exe

# shortcut names defined here
ex_filter_frontend_net: $(EX_FILTER_FRONTEND_NET)
metaproxy: $(METAPROXY_PROG)
test_pipe: $(TEST_PIPE)

###########################################################
############### Compiler and linker options 
###########################################################

!if $(DEBUG)
YAZD=yaz4d
YAZPPD=yazpp5d
!else
YAZD=yaz4
YAZPPD=yazpp5
!endif

# YAZ includes & libs
YAZ_LIB="$(YAZ_DIR)\lib\$(YAZD).lib"
YAZ_DEF=/DYAZ_HAVE_XML2=1 /DYAZ_HAVE_XSLT=1 /DYAZ_USE_NEW_LOG=1 /I"$(YAZ_DIR)\include"

# Copy yaz.dll to our bin
yaz: $(BINDIR)\$(YAZD).dll
$(BINDIR)\$(YAZD).dll: "$(YAZ_DIR)\bin\$(YAZD).dll"
  copy "$(YAZ_DIR)\bin\$(YAZD).dll" $(BINDIR)

# YAZ++ includes & libs
YAZPP_LIB="$(YAZPP_DIR)\lib\$(YAZPPD).lib"
YAZPP_DEF=/I"$(YAZPP_DIR)\include"

# Copy yazpp.dll to our bin
yazpp: $(BINDIR)\$(YAZPPD).dll
$(BINDIR)\$(YAZPPD).dll: "$(YAZPP_DIR)\bin\$(YAZPPD).dll"
  copy "$(YAZPP_DIR)\bin\$(YAZPPD).dll" $(BINDIR)

# Copy libxslt and associates to our bin directory
libxslt: $(BINDIR)\libxslt.dll

$(BINDIR)\libxslt.dll: "$(LIBXSLT_DIR)\bin\libxslt.dll"
	copy "$(ICONV_DIR)\bin\iconv.dll" $(BINDIR)
	copy "$(ZLIB_DIR)\bin\zlib1.dll" $(BINDIR)
	copy "$(LIBXML2_DIR)\bin\libxml2.dll" $(BINDIR)
	copy "$(LIBXSLT_DIR)\bin\libxslt.dll" $(BINDIR)

config: $(SRCDIR)\config.hpp

$(SRCDIR)\config.hpp:
	echo /*windows*/ >$(SRCDIR)\config.hpp

# Boost 
#
!if $(DEBUG)
BOOST_LIB_SUFFIX=$(BOOST_TOOLSET)-$(BOOST_MODEL)-gd-$(BOOST_VERSION).lib
!else
BOOST_LIB_SUFFIX=$(BOOST_TOOLSET)-$(BOOST_MODEL)-$(BOOST_VERSION).lib
!endif

BOOST_DEF = /I"$(BOOST)"
BOOST_LIB = \
	     /LIBPATH:"$(BOOST)\lib" \
    "libboost_thread-$(BOOST_LIB_SUFFIX)"  \
    "libboost_regex-$(BOOST_LIB_SUFFIX)" \
    "libboost_date_time-$(BOOST_LIB_SUFFIX)" 

LIBXSLT_DEF = \
	      /I"$(ICONV_DIR)\include" \
	      /I"$(LIBXML2_DIR)\include" \
	      /I"$(LIBXSLT_DIR)\include"

LIBXSLT_LIB = \
    "$(ICONV_DIR)\lib\iconv.lib" \
    "$(LIBXML2_DIR)\lib\libxml2.lib" \
    "$(LIBXSLT_DIR)\lib\libxslt.lib"

COMMON_C_OPTIONS=          \
  /nologo /W3 /EHsc /FD /c \
  /D "_CRT_SECURE_NO_DEPRECATE" \
  $(YAZ_DEF)               \
  $(YAZPP_DEF)             \
  $(BOOST_DEF)             \
  $(LIBXSLT_DEF)	   \
  /D VERSION=\"$(VERSION)\" \
  /D "_WINDOWS"            \
  /D "WIN32"               \
  /FR"$(OBJDIR)\\"         \
  /Fo"$(OBJDIR)\\"         \
  /Fd"$(OBJDIR)\\" 	   \
  /I"$(ROOTDIR)\include" 

DEBUG_C_OPTIONS=  \
  /D "_DEBUG"      \
  /MDd  /Od /Zi /Gm

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
# /Gm  = Minimal rebuild (some cpp class stuff)
# /Zi  = Program database for debuggers
# /ZI  = Pgm database with special "edit&continue" stuff - not available in C5


### Linker options
LINK=link.exe

LINK_LIBS=kernel32.lib user32.lib gdi32.lib advapi32.lib uuid.lib \
          wsock32.lib  advapi32.lib \
          $(YAZ_LIB) $(YAZPP_LIB) $(ICONV_LIB) $(LIBXSLT_LIB) \
	  $(BOOST_LIB) 

COMMON_LNK_OPTIONS= /nologo /subsystem:windows /incremental:no

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
	$(OBJDIR)\factory_filter.obj \
	$(OBJDIR)\factory_static.obj \
        $(OBJDIR)\filter.obj \
	$(OBJDIR)\filter_auth_simple.obj \
        $(OBJDIR)\filter_backend_test.obj \
        $(OBJDIR)\filter_bounce.obj \
        $(OBJDIR)\filter_cql_to_rpn.obj \
	$(OBJDIR)\filter_frontend_net.obj \
	$(OBJDIR)\filter_http_file.obj \
        $(OBJDIR)\filter_limit.obj \
        $(OBJDIR)\filter_load_balance.obj \
        $(OBJDIR)\filter_log.obj \
	$(OBJDIR)\filter_multi.obj \
	$(OBJDIR)\filter_query_rewrite.obj \
        $(OBJDIR)\filter_record_transform.obj \
        $(OBJDIR)\filter_session_shared.obj \
        $(OBJDIR)\filter_sort.obj \
        $(OBJDIR)\filter_sru_to_z3950.obj \
        $(OBJDIR)\filter_template.obj \
        $(OBJDIR)\filter_virt_db.obj \
        $(OBJDIR)\filter_z3950_client.obj \
        $(OBJDIR)\filter_zeerex_explain.obj \
        $(OBJDIR)\filter_zoom.obj \
	$(OBJDIR)\gduutil.obj \
	$(OBJDIR)\origin.obj \
	$(OBJDIR)\package.obj \
	$(OBJDIR)\pipe.obj \
	$(OBJDIR)\plainfile.obj \
        $(OBJDIR)\router_chain.obj \
        $(OBJDIR)\router_flexml.obj \
        $(OBJDIR)\session.obj \
        $(OBJDIR)\sru_util.obj \
	$(OBJDIR)\thread_pool_observer.obj \
        $(OBJDIR)\torus.obj \
        $(OBJDIR)\url_recipe.obj \
        $(OBJDIR)\util.obj \
        $(OBJDIR)\xmlutil.obj 

EX_FILTER_FRONTEND_NET_OBJS = $(OBJDIR)\ex_filter_frontend_net.obj
METAPROXY_OBJS = $(OBJDIR)\metaproxy_prog.obj
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

PROJECT_RES=$(OBJDIR)\metaproxy.res
PROJECT_RC=$(WINDIR)\metaproxy.rc

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

$(EX_FILTER_FRONTEND_NET) : "$(BINDIR)" $(EX_FILTER_FRONTEND_NET_OBJS) $(PROJECT_DLL_OBJS)
	$(LINK) \
		$(LNKOPT) \
		$(PROG_LINK_OPTIONS) \
		$(LINK_LIBS) \
		$(PROJECT_DLL_OBJS) \
		$(EX_FILTER_FRONTEND_NET_OBJS) \
		/out:$(EX_FILTER_FRONTEND_NET) 

$(METAPROXY_PROG) : "$(BINDIR)" $(METAPROXY_OBJS) $(PROJECT_DLL_OBJS)
	$(LINK) \
		$(LNKOPT) \
		$(PROG_LINK_OPTIONS) \
		$(LINK_LIBS) \
		$(METAPROXY_OBJS) \
		$(PROJECT_DLL_OBJS) \
		/out:$(METAPROXY_PROG) 

$(TEST_PIPE) : "$(BINDIR)" $(TEST_PIPE_OBJS) $(PROJECT_DLL_OBJS)
	$(LINK) \
		$(LNKOPT) \
		$(PROG_LINK_OPTIONS) \
		$(LINK_LIBS) \
		$(PROJECT_DLL_OBJS) \
		$(TEST_PIPE_OBJS) \
		/out:$(TEST_PIPE)

############## clean
clean:
	-del $(BINDIR)\*.exe
	-del $(BINDIR)\*.dll
	-del $(TMPDIR)\*.
	-del $(LIBDIR)\*.LIB
	-del $(OBJDIR)\*.OBJ

dirs: $(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR)

$(OBJDIR) $(WINDIR) $(LIBDIR) $(BINDIR) $(TMPDIR):
	if not exist "$@/$(NUL)" mkdir "$@"

