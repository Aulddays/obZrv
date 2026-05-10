bin_PROGRAMS = obZrv

obZrv_SOURCES = \
	main.cpp \
	App.h       App.cpp \
	WndBase.h   WndBase.cpp \
	MainWnd.h   MainWnd.cpp \
	Menu.h      Menu.cpp \
	Toolbar.h   Toolbar.cpp \
	StatusBar.h StatusBar.cpp \
	MainView.h  MainView.cpp \
	FileList.h  FileList.cpp \
	dpi.h       dpi.cpp \
	config.h    config.cpp \
	strutil.h \
	ConnectDlg.h  ConnectDlg.cpp \
	RemoteBrowserDlg.h RemoteBrowserDlg.cpp \
	Codec.h \
	GdiPlusCodec.h  GdiPlusCodec.cpp \
	WebpCodec.h     WebpCodec.cpp \
	BasicBitmap/BasicBitmap.h   BasicBitmap/BasicBitmap.cpp \
	Doc.h           Doc.cpp \
	ImageView.h     ImageView.cpp

obZrv_CPPFLAGS = -I$(top_srcdir)

obZrv_CXXFLAGS = \
	-std=c++11 \
	-Wall \
	-ffunction-sections \
	-fdata-sections \
	$(AM_CXXFLAGS)

# Files that include asio headers are compiled without -fno-exceptions
# because asio's system_context.ipp uses catch(...).
# The remaining sources keep the flag via obz_CXXFLAGS above (no -fno-exceptions
# is now global; size impact is negligible with --gc-sections).

# BasicBitmap_SSE2.cpp requires -msse2; compile separately and link the .o
obZrv_LDADD = \
	$(top_builddir)/unifs/libunifs.a \
	resource.o \
	BasicBitmap_SSE2.o \
	webp/libwebpdemux.a \
	webp/libwebp.a \
	$(WINDOWS_SOCKET_LIBS) \
	-luser32 \
	-lgdi32 \
	-lcomctl32 \
	-lshell32 \
	-lshlwapi \
	-lole32 \
	-lgdiplus

BasicBitmap_SSE2.o: $(srcdir)/BasicBitmap/BasicBitmap_SSE2.cpp \
                    $(srcdir)/BasicBitmap/BasicBitmap.h
	$(CXX) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) $(CPPFLAGS) \
	  $(obZrv_CXXFLAGS) -msse2 -c -o $@ $<

obZrv_DEPENDENCIES = libwinpthread_nover.a resource.o BasicBitmap_SSE2.o \
                    webp/libwebpdemux.a webp/libwebp.a \
                    $(top_builddir)/unifs/libunifs.a

# -static-libgcc/-static-libstdc++ embed the GCC runtime so we don't need to
# ship libstdc++/libgcc DLLs. Windows API and webp DLLs remain dynamic.
obZrv_LDFLAGS = -mwindows -static-libgcc -static-libstdc++ -Wl,--gc-sections \
              -Wl,--whole-archive \
              libwinpthread_nover.a \
              -Wl,--no-whole-archive

# Copy libwinpthread.a without version.o (which carries a .rsrc VERSION resource
# that would conflict with our own resource.o).
WINPTHREAD_SRC = /usr/i686-w64-mingw32/sys-root/mingw/lib/libwinpthread.a
libwinpthread_nover.a: $(WINPTHREAD_SRC)
	cp $(WINPTHREAD_SRC) $@
	$(AR) d $@ version.o

DLLTOOL = i686-w64-mingw32-dlltool
GENDEF  = gendef

# Generate MinGW import libraries from the shipped DLLs
# gendef extracts the export table to a .def, then dlltool builds the .a
webp/libwebp.a: $(srcdir)/dll/libwebp_x86.dll
	$(GENDEF) $<
	$(DLLTOOL) -D libwebp_x86.dll -d libwebp_x86.def -l $@
	$(RM) libwebp_x86.def

webp/libwebpdemux.a: $(srcdir)/dll/libwebpdemux_x86.dll
	$(GENDEF) $<
	$(DLLTOOL) -D libwebpdemux_x86.dll -d libwebpdemux_x86.def -l $@
	$(RM) libwebpdemux_x86.def

# Compile Windows resource file with windres
resource.o: $(srcdir)/obZrv.rc $(srcdir)/resource.h \
            $(srcdir)/res/obZrv.ico $(srcdir)/res/obZrv.manifest \
            $(srcdir)/res/Toolbar.20.png $(srcdir)/res/Toolbar.25.png \
            $(srcdir)/res/Toolbar.30.png $(srcdir)/res/Toolbar.40.png
	$(WINDRES) -I$(srcdir) -I$(srcdir)/res -i $< -o $@

CLEANFILES = resource.o BasicBitmap_SSE2.o obZrv-rls.exe \
             webp/libwebp.a webp/libwebpdemux.a libwinpthread_nover.a

EXTRA_DIST = obZrv.rc resource.h \
             res/obZrv.ico res/obZrv.manifest \
             res/Toolbar.20.png res/Toolbar.25.png \
             res/Toolbar.30.png res/Toolbar.40.png \
             dll/libwebp_x86.dll dll/libwebpdemux_x86.dll dll/libsharpyuv_x86.dll

# Release binary: strip all debug symbols and symbol table
obZ-rls.exe: obZ.exe
	$(STRIP) --strip-all $< -o $@
