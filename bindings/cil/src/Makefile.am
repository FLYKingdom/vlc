EXTRA_DIST = $(SOURCES_dll)
MOSTLYCLEANFILES = $(pkglib_SCRIPTS)

pkglib_SCRIPTS = VideoLAN.LibVLC.dll

SOURCES_dll = \
	ustring.cs \
	exception.cs \
	event.cs \
	marshal.cs \
	instance.cs \
	media.cs \
	player.cs \
	libvlc.cs

VideoLAN.LibVLC.dll: $(SOURCES_dll)
	$(CSC) -target:library -out:$@ $(CSFLAGS) $^

