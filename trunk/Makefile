# GNU Make for TscancCode
# Created: Nov. 24th, 2017
# Author: TscanCode Team

# folder where lib/*.cpp files are located
ifndef SRCDIR
    SRCDIR=lib
endif

ifndef CXX
    CXX=g++
endif

ifndef CXXFLAGS
    CXXFLAGS=-std=c++0x -O2 -include lib/cxx11emu.h -DNDEBUG -Wall -Wno-sign-compare -Wno-unknown-pragmas -Wno-reorder
endif

# Force including libpthread
LIBS+=-lpthread

ifndef INCLUDE_FOR_LIB
    INCLUDE_FOR_LIB=-Ilib -Icommon -Iexternals/tinyxml
endif

ifndef INCLUDE_FOR_CLI
    INCLUDE_FOR_CLI=-Ilib -Icommon -Iexternals/tinyxml
endif

###### Object Files
COMMONOBJ =     common/path.o        \
                common/filelister.o    \
                common/pathmatch.o    \
                common/filedepend.o

LIBOBJ =      $(SRCDIR)/astutils.o \
              $(SRCDIR)/check.o \
              $(SRCDIR)/check64bit.o \
              $(SRCDIR)/checkassert.o \
              $(SRCDIR)/checkautovariables.o \
              $(SRCDIR)/checkbool.o \
              $(SRCDIR)/checkboost.o \
              $(SRCDIR)/checkbufferoverrun.o \
              $(SRCDIR)/checkclass.o \
              $(SRCDIR)/checkcondition.o \
              $(SRCDIR)/checkexceptionsafety.o \
              $(SRCDIR)/checkinternal.o \
              $(SRCDIR)/checkio.o \
              $(SRCDIR)/checkleakautovar.o \
              $(SRCDIR)/checkmemoryleak.o \
              $(SRCDIR)/checknonreentrantfunctions.o \
              $(SRCDIR)/checknullpointer.o \
              $(SRCDIR)/checkobsolescentfunctions.o \
              $(SRCDIR)/checkother.o \
              $(SRCDIR)/checkpostfixoperator.o \
              $(SRCDIR)/checksizeof.o \
              $(SRCDIR)/checkstl.o \
              $(SRCDIR)/checkstring.o \
              $(SRCDIR)/checktype.o \
              $(SRCDIR)/checkuninitvar.o \
              $(SRCDIR)/checkunusedfunctions.o \
              $(SRCDIR)/checkunusedvar.o \
              $(SRCDIR)/checkvaarg.o \
              $(SRCDIR)/checktsccompute.o \
              $(SRCDIR)/checktsclogic.o \
              $(SRCDIR)/checktscoverrun.o \
              $(SRCDIR)/checkstatistic.o \
              $(SRCDIR)/checktscsuspicious.o \
              $(SRCDIR)/checktscinvalidvarargs.o \
              $(SRCDIR)/checktscnullpointer2.o \
              $(SRCDIR)/tscancode.o \
              $(SRCDIR)/errorlogger.o \
              $(SRCDIR)/executionpath.o \
              $(SRCDIR)/library.o \
              $(SRCDIR)/mathlib.o \
              $(SRCDIR)/preprocessor.o \
              $(SRCDIR)/settings.o \
              $(SRCDIR)/suppressions.o \
              $(SRCDIR)/symboldatabase.o \
              $(SRCDIR)/templatesimplifier.o \
              $(SRCDIR)/timer.o \
              $(SRCDIR)/token.o \
              $(SRCDIR)/tokenex.o \
              $(SRCDIR)/tokenize.o \
              $(SRCDIR)/tokenlist.o \
              $(SRCDIR)/valueflow.o \
              $(SRCDIR)/globaltokenizer.o \
              $(SRCDIR)/globalmacros.o \
              $(SRCDIR)/globalsymboldatabase.o \
              $(SRCDIR)/utilities.o \
              $(SRCDIR)/checkreadability.o

CLIOBJ =      cli/cmdlineparser.o \
              cli/tscexecutor.o \
              cli/main.o \
              cli/tscthreadexecutor.o

ifndef TINYXML
    TINYXML = externals/tinyxml/tinyxml2.o
endif

EXTOBJ += $(TINYXML)

###### Targets
tscancode: $(LIBOBJ) $(CLIOBJ) $(EXTOBJ) $(COMMONOBJ)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o tscancode $(CLIOBJ) $(LIBOBJ) $(EXTOBJ) $(COMMONOBJ) $(LIBS) $(LDFLAGS) $(RDYNAMIC)

clean:
	rm -f lib/*.o cli/*.o common/*.o externals/tinyxml/*.o tscancode

###### Build

$(SRCDIR)/astutils.o: lib/astutils.cpp lib/cxx11emu.h lib/astutils.h lib/symboldatabase.h common/config.h lib/token.h lib/valueflow.h lib/mathlib.h lib/utils.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/astutils.o $(SRCDIR)/astutils.cpp

$(SRCDIR)/check.o: lib/check.cpp lib/cxx11emu.h lib/check.h common/config.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/check.o $(SRCDIR)/check.cpp

$(SRCDIR)/check64bit.o: lib/check64bit.cpp lib/cxx11emu.h lib/check64bit.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/check64bit.o $(SRCDIR)/check64bit.cpp

$(SRCDIR)/checkassert.o: lib/checkassert.cpp lib/cxx11emu.h lib/checkassert.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkassert.o $(SRCDIR)/checkassert.cpp

$(SRCDIR)/checkautovariables.o: lib/checkautovariables.cpp lib/cxx11emu.h lib/checkautovariables.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkautovariables.o $(SRCDIR)/checkautovariables.cpp

$(SRCDIR)/checkbool.o: lib/checkbool.cpp lib/cxx11emu.h lib/checkbool.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkbool.o $(SRCDIR)/checkbool.cpp

$(SRCDIR)/checkboost.o: lib/checkboost.cpp lib/cxx11emu.h lib/checkboost.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkboost.o $(SRCDIR)/checkboost.cpp

$(SRCDIR)/checkbufferoverrun.o: lib/checkbufferoverrun.cpp lib/cxx11emu.h lib/checkbufferoverrun.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h lib/astutils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkbufferoverrun.o $(SRCDIR)/checkbufferoverrun.cpp

$(SRCDIR)/checkclass.o: lib/checkclass.cpp lib/cxx11emu.h lib/checkclass.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkclass.o $(SRCDIR)/checkclass.cpp

$(SRCDIR)/checkcondition.o: lib/checkcondition.cpp lib/cxx11emu.h lib/checkcondition.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/astutils.h lib/checkother.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkcondition.o $(SRCDIR)/checkcondition.cpp

$(SRCDIR)/checkexceptionsafety.o: lib/checkexceptionsafety.cpp lib/cxx11emu.h lib/checkexceptionsafety.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkexceptionsafety.o $(SRCDIR)/checkexceptionsafety.cpp

$(SRCDIR)/checkinternal.o: lib/checkinternal.cpp lib/cxx11emu.h lib/checkinternal.h lib/check.h common/config.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkinternal.o $(SRCDIR)/checkinternal.cpp

$(SRCDIR)/checkio.o: lib/checkio.cpp lib/cxx11emu.h lib/checkio.h lib/check.h common/config.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkio.o $(SRCDIR)/checkio.cpp

$(SRCDIR)/checkleakautovar.o: lib/checkleakautovar.cpp lib/cxx11emu.h lib/checkleakautovar.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/checkmemoryleak.h lib/symboldatabase.h lib/utils.h lib/astutils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkleakautovar.o $(SRCDIR)/checkleakautovar.cpp

$(SRCDIR)/checkmemoryleak.o: lib/checkmemoryleak.cpp lib/cxx11emu.h lib/checkmemoryleak.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h lib/astutils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkmemoryleak.o $(SRCDIR)/checkmemoryleak.cpp

$(SRCDIR)/checknonreentrantfunctions.o: lib/checknonreentrantfunctions.cpp lib/cxx11emu.h lib/checknonreentrantfunctions.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checknonreentrantfunctions.o $(SRCDIR)/checknonreentrantfunctions.cpp

$(SRCDIR)/checknullpointer.o: lib/checknullpointer.cpp lib/cxx11emu.h lib/checknullpointer.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checknullpointer.o $(SRCDIR)/checknullpointer.cpp

$(SRCDIR)/checkobsolescentfunctions.o: lib/checkobsolescentfunctions.cpp lib/cxx11emu.h lib/checkobsolescentfunctions.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkobsolescentfunctions.o $(SRCDIR)/checkobsolescentfunctions.cpp

$(SRCDIR)/checkother.o: lib/checkother.cpp lib/cxx11emu.h lib/checkother.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/astutils.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkother.o $(SRCDIR)/checkother.cpp

$(SRCDIR)/checkpostfixoperator.o: lib/checkpostfixoperator.cpp lib/cxx11emu.h lib/checkpostfixoperator.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkpostfixoperator.o $(SRCDIR)/checkpostfixoperator.cpp

$(SRCDIR)/checksizeof.o: lib/checksizeof.cpp lib/cxx11emu.h lib/checksizeof.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checksizeof.o $(SRCDIR)/checksizeof.cpp

$(SRCDIR)/checkstl.o: lib/checkstl.cpp lib/cxx11emu.h lib/checkstl.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h lib/checknullpointer.h lib/executionpath.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkstl.o $(SRCDIR)/checkstl.cpp

$(SRCDIR)/checkstring.o: lib/checkstring.cpp lib/cxx11emu.h lib/checkstring.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkstring.o $(SRCDIR)/checkstring.cpp

$(SRCDIR)/checktype.o: lib/checktype.cpp lib/cxx11emu.h lib/checktype.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checktype.o $(SRCDIR)/checktype.cpp

$(SRCDIR)/checkuninitvar.o: lib/checkuninitvar.cpp lib/cxx11emu.h lib/checkuninitvar.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/astutils.h lib/checknullpointer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkuninitvar.o $(SRCDIR)/checkuninitvar.cpp

$(SRCDIR)/checkunusedfunctions.o: lib/checkunusedfunctions.cpp lib/cxx11emu.h lib/checkunusedfunctions.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkunusedfunctions.o $(SRCDIR)/checkunusedfunctions.cpp

$(SRCDIR)/checkunusedvar.o: lib/checkunusedvar.cpp lib/cxx11emu.h lib/checkunusedvar.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkunusedvar.o $(SRCDIR)/checkunusedvar.cpp

$(SRCDIR)/checkvaarg.o: lib/checkvaarg.cpp lib/cxx11emu.h lib/checkvaarg.h common/config.h lib/check.h lib/token.h lib/valueflow.h lib/mathlib.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkvaarg.o $(SRCDIR)/checkvaarg.cpp

$(SRCDIR)/checktsccompute.o: lib/checktsccompute.cpp lib/checktsccompute.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checktsccompute.o $(SRCDIR)/checktsccompute.cpp

$(SRCDIR)/checktsclogic.o: lib/checktsclogic.cpp lib/checktsclogic.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checktsclogic.o $(SRCDIR)/checktsclogic.cpp

$(SRCDIR)/checktscoverrun.o: lib/checktscoverrun.cpp lib/checktscoverrun.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checktscoverrun.o $(SRCDIR)/checktscoverrun.cpp

$(SRCDIR)/checkstatistic.o: lib/checkstatistic.cpp lib/checkstatistic.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkstatistic.o $(SRCDIR)/checkstatistic.cpp

$(SRCDIR)/checktscsuspicious.o: lib/checktscsuspicious.cpp lib/checktscsuspicious.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checktscsuspicious.o $(SRCDIR)/checktscsuspicious.cpp

$(SRCDIR)/checktscinvalidvarargs.o: lib/checktscinvalidvarargs.cpp lib/checktscinvalidvarargs.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checktscinvalidvarargs.o $(SRCDIR)/checktscinvalidvarargs.cpp

$(SRCDIR)/checktscnullpointer2.o: lib/checktscnullpointer2.cpp lib/checktscnullpointer2.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checktscnullpointer2.o $(SRCDIR)/checktscnullpointer2.cpp

$(SRCDIR)/tscancode.o: lib/tscancode.cpp lib/cxx11emu.h lib/tscancode.h common/config.h lib/settings.h lib/library.h lib/mathlib.h lib/token.h lib/valueflow.h lib/suppressions.h lib/standards.h lib/timer.h lib/errorlogger.h lib/check.h lib/tokenize.h lib/tokenlist.h lib/preprocessor.h common/path.h lib/version.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/tscancode.o $(SRCDIR)/tscancode.cpp

$(SRCDIR)/errorlogger.o: lib/errorlogger.cpp lib/cxx11emu.h lib/errorlogger.h common/config.h lib/suppressions.h common/path.h lib/tscancode.h lib/settings.h lib/library.h lib/mathlib.h lib/token.h lib/valueflow.h lib/standards.h lib/timer.h lib/check.h lib/tokenize.h lib/tokenlist.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/errorlogger.o $(SRCDIR)/errorlogger.cpp
	
$(SRCDIR)/executionpath.o:lib/executionpath.cpp lib/executionpath.h common/config.h lib/token.h lib/symboldatabase.h lib/mathlib.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/executionpath.o $(SRCDIR)/executionpath.cpp

$(SRCDIR)/library.o: lib/library.cpp lib/cxx11emu.h lib/library.h common/config.h lib/mathlib.h lib/token.h lib/valueflow.h common/path.h lib/tokenlist.h lib/symboldatabase.h lib/utils.h lib/astutils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/library.o $(SRCDIR)/library.cpp

$(SRCDIR)/mathlib.o: lib/mathlib.cpp lib/cxx11emu.h lib/mathlib.h common/config.h lib/errorlogger.h lib/suppressions.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/mathlib.o $(SRCDIR)/mathlib.cpp

$(SRCDIR)/preprocessor.o: lib/preprocessor.cpp lib/cxx11emu.h lib/preprocessor.h common/config.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/token.h lib/valueflow.h lib/mathlib.h common/path.h lib/settings.h lib/library.h lib/standards.h lib/timer.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/preprocessor.o $(SRCDIR)/preprocessor.cpp

$(SRCDIR)/settings.o: lib/settings.cpp lib/cxx11emu.h lib/settings.h common/config.h lib/library.h lib/mathlib.h lib/token.h lib/valueflow.h lib/suppressions.h lib/standards.h lib/timer.h common/path.h lib/preprocessor.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/settings.o $(SRCDIR)/settings.cpp

$(SRCDIR)/suppressions.o: lib/suppressions.cpp lib/cxx11emu.h lib/suppressions.h common/config.h lib/settings.h lib/library.h lib/mathlib.h lib/token.h lib/valueflow.h lib/standards.h lib/timer.h common/path.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/suppressions.o $(SRCDIR)/suppressions.cpp

$(SRCDIR)/symboldatabase.o: lib/symboldatabase.cpp lib/cxx11emu.h lib/symboldatabase.h common/config.h lib/token.h lib/valueflow.h lib/mathlib.h lib/utils.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/symboldatabase.o $(SRCDIR)/symboldatabase.cpp

$(SRCDIR)/templatesimplifier.o: lib/templatesimplifier.cpp lib/cxx11emu.h lib/templatesimplifier.h common/config.h lib/mathlib.h lib/token.h lib/valueflow.h lib/tokenize.h lib/errorlogger.h lib/suppressions.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/templatesimplifier.o $(SRCDIR)/templatesimplifier.cpp

$(SRCDIR)/timer.o: lib/timer.cpp lib/cxx11emu.h lib/timer.h common/config.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/timer.o $(SRCDIR)/timer.cpp

$(SRCDIR)/token.o: lib/token.cpp lib/cxx11emu.h lib/token.h common/config.h lib/valueflow.h lib/mathlib.h lib/errorlogger.h lib/suppressions.h lib/check.h lib/tokenize.h lib/tokenlist.h lib/settings.h lib/library.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/token.o $(SRCDIR)/token.cpp

$(SRCDIR)/tokenex.o: lib/tokenex.cpp lib/tokenex.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/tokenex.o $(SRCDIR)/tokenex.cpp

$(SRCDIR)/tokenize.o: lib/tokenize.cpp lib/cxx11emu.h lib/tokenize.h lib/errorlogger.h common/config.h lib/suppressions.h lib/tokenlist.h lib/mathlib.h lib/settings.h lib/library.h lib/token.h lib/valueflow.h lib/standards.h lib/timer.h lib/check.h common/path.h lib/symboldatabase.h lib/utils.h lib/templatesimplifier.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/tokenize.o $(SRCDIR)/tokenize.cpp

$(SRCDIR)/tokenlist.o: lib/tokenlist.cpp lib/cxx11emu.h lib/tokenlist.h common/config.h lib/token.h lib/valueflow.h lib/mathlib.h common/path.h lib/preprocessor.h lib/settings.h lib/library.h lib/suppressions.h lib/standards.h lib/timer.h lib/errorlogger.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/tokenlist.o $(SRCDIR)/tokenlist.cpp

$(SRCDIR)/valueflow.o: lib/valueflow.cpp lib/cxx11emu.h lib/valueflow.h common/config.h lib/astutils.h lib/errorlogger.h lib/suppressions.h lib/mathlib.h lib/settings.h lib/library.h lib/token.h lib/standards.h lib/timer.h lib/symboldatabase.h lib/utils.h lib/tokenlist.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/valueflow.o $(SRCDIR)/valueflow.cpp

$(SRCDIR)/globaltokenizer.o: lib/globaltokenizer.cpp lib/globaltokenizer.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/globaltokenizer.o $(SRCDIR)/globaltokenizer.cpp

$(SRCDIR)/globalmacros.o: lib/globalmacros.cpp lib/globalmacros.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/globalmacros.o $(SRCDIR)/globalmacros.cpp

$(SRCDIR)/globalsymboldatabase.o: lib/globalsymboldatabase.cpp lib/globalsymboldatabase.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/globalsymboldatabase.o $(SRCDIR)/globalsymboldatabase.cpp

$(SRCDIR)/utilities.o: lib/utilities.cpp lib/utilities.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/utilities.o $(SRCDIR)/utilities.cpp

$(SRCDIR)/checkreadability.o: $(SRCDIR)/checkreadability.cpp $(SRCDIR)/checkreadability.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o $(SRCDIR)/checkreadability.o $(SRCDIR)/checkreadability.cpp

cli/cmdlineparser.o: cli/cmdlineparser.cpp lib/cxx11emu.h cli/cmdlineparser.h lib/tscancode.h common/config.h lib/settings.h lib/library.h lib/mathlib.h lib/token.h lib/valueflow.h lib/suppressions.h lib/standards.h lib/timer.h lib/errorlogger.h lib/check.h lib/tokenize.h lib/tokenlist.h cli/tscexecutor.h common/filelister.h common/path.h
	$(CXX) ${INCLUDE_FOR_CLI} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o cli/cmdlineparser.o cli/cmdlineparser.cpp

cli/tscexecutor.o: cli/tscexecutor.cpp lib/cxx11emu.h cli/tscexecutor.h lib/errorlogger.h common/config.h lib/suppressions.h cli/cmdlineparser.h lib/tscancode.h lib/settings.h lib/library.h lib/mathlib.h lib/token.h lib/valueflow.h lib/standards.h lib/timer.h lib/check.h lib/tokenize.h lib/tokenlist.h common/filelister.h common/path.h common/pathmatch.h lib/preprocessor.h cli/tscthreadexecutor.h
	$(CXX) ${INCLUDE_FOR_CLI} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o cli/tscexecutor.o cli/tscexecutor.cpp

cli/main.o: cli/main.cpp lib/cxx11emu.h cli/tscexecutor.h lib/errorlogger.h common/config.h lib/suppressions.h
	$(CXX) ${INCLUDE_FOR_CLI} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o cli/main.o cli/main.cpp

cli/pathmatch.o: cli/pathmatch.cpp lib/cxx11emu.h common/pathmatch.h common/path.h common/config.h
	$(CXX) ${INCLUDE_FOR_CLI} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o cli/pathmatch.o cli/pathmatch.cpp

cli/tscthreadexecutor.o: cli/tscthreadexecutor.cpp lib/cxx11emu.h cli/tscthreadexecutor.h lib/errorlogger.h common/config.h lib/suppressions.h lib/tscancode.h lib/settings.h lib/library.h lib/mathlib.h lib/token.h lib/valueflow.h lib/standards.h lib/timer.h lib/check.h lib/tokenize.h lib/tokenlist.h cli/tscexecutor.h
	$(CXX) ${INCLUDE_FOR_CLI} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o cli/tscthreadexecutor.o cli/tscthreadexecutor.cpp

common/pathmatch.o: common/pathmatch.cpp common/pathmatch.h common/path.h common/config.h
	$(CXX) $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o common/pathmatch.o common/pathmatch.cpp

common/path.o: common/path.cpp common/path.h common/config.h
	$(CXX) $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o common/path.o common/path.cpp -I common -I lib

common/filelister.o: common/filelister.cpp common/filelister.h common/path.h common/config.h common/pathmatch.h
	$(CXX) $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o common/filelister.o common/filelister.cpp

common/filedepend.o: common/filedepend.cpp common/filedepend.h common/filelister.h common/path.h common/config.h common/pathmatch.h
	$(CXX) $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o common/filedepend.o common/filedepend.cpp

externals/tinyxml/tinyxml2.o: externals/tinyxml/tinyxml2.cpp lib/cxx11emu.h externals/tinyxml/tinyxml2.h
	$(CXX) ${INCLUDE_FOR_LIB} $(CPPFLAGS) $(CFG) $(CXXFLAGS) $(UNDEF_STRICT_ANSI) -c -o externals/tinyxml/tinyxml2.o externals/tinyxml/tinyxml2.cpp
