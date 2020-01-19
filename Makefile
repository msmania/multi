!IF "$(PLATFORM)"=="X64" || "$(PLATFORM)"=="x64"
ARCH=amd64
!ELSE
ARCH=x86
!ENDIF

OUTDIR=bin\$(ARCH)
OBJDIR=obj\$(ARCH)
GENDIR=gen\$(ARCH)
SRCDIR=src
GTEST_SRC_DIR=D:\src\googletest
GTEST_BUILD_DIR=D:\src\googletest\build\$(ARCH)
DETOURS_DIR=D:\src\detours

CC=cl
RD=rd/s/q
RM=del/q
LINKER=link
TARGET=t.exe

CSTUB=client_stub
SSTUB=server_stub

OBJS=\
	$(OBJDIR)\$(CSTUB).obj\
	$(OBJDIR)\$(SSTUB).obj\
	$(OBJDIR)\blob.obj\
	$(OBJDIR)\client_impl.obj\
	$(OBJDIR)\server_impl.obj\
	$(OBJDIR)\main.obj\
	$(OBJDIR)\policy.obj\
	$(OBJDIR)\rpcutils.obj\
	$(OBJDIR)\utils.obj\

LIBS=\
	detours.lib\
	rpcrt4.lib\
	user32.lib\

CFLAGS=\
	/nologo\
	/c\
	/Od\
	/W4\
	/Zi\
	/EHsc\
	/DUNICODE\
	/DRPC_USE_NATIVE_WCHAR\
	/Zc:wchar_t\
	/Fo"$(OBJDIR)\\"\
	/Fd"$(OBJDIR)\\"\
	/I"$(GTEST_SRC_DIR)\googletest\include"\
	/I"$(GTEST_SRC_DIR)\googlemock\include"\
	/I"$(DETOURS_DIR)\include"\
	/I"$(GENDIR)"\

LFLAGS=\
	/NOLOGO\
	/DEBUG\
	/SUBSYSTEM:WINDOWS\
	/LIBPATH:"$(GTEST_BUILD_DIR)\lib\Release"\
	/LIBPATH:"$(DETOURS_DIR)\lib.$(PLATFORM)"\

MIDL_FLAGS=\
	/nologo\
	/prefix client c_\
	/prefix server s_\

all: $(OUTDIR)\$(TARGET)

$(OUTDIR)\$(TARGET): $(OBJS)
	@if not exist $(OUTDIR) mkdir $(OUTDIR)
	$(LINKER) $(LFLAGS) $(LIBS) /PDB:"$(@R).pdb" /OUT:$@ $**

{$(SRCDIR)}.cpp{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

{$(GENDIR)}.c{$(OBJDIR)}.obj:
	@if not exist $(OBJDIR) mkdir $(OBJDIR)
	$(CC) $(CFLAGS) $<

$(GENDIR)\$(CSTUB).c: $(SRCDIR)\sandbox.idl
	@if not exist $(GENDIR) mkdir $(GENDIR)
	midl $(MIDL_FLAGS)\
    /h $(GENDIR)\sandbox.h\
    /cstub $(GENDIR)\$(CSTUB).c\
    /sstub $(GENDIR)\$(SSTUB).c $**

clean:
	@if exist $(OBJDIR) $(RD) $(OBJDIR)
	@if exist $(GENDIR) $(RD) $(GENDIR)
	@if exist $(OUTDIR)\$(TARGET) $(RM) $(OUTDIR)\$(TARGET)
	@if exist $(OUTDIR)\$(TARGET:exe=ilk) $(RM) $(OUTDIR)\$(TARGET:exe=ilk)
	@if exist $(OUTDIR)\$(TARGET:exe=pdb) $(RM) $(OUTDIR)\$(TARGET:exe=pdb)
