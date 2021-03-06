# Darwin rules
all: .autoconf .gnumake .automake .libtool .intl .pkgcfg .yasm .freetype \
    .fribidi .a52 .mpeg2 .id3tag .mad .ogg .vorbis .vorbisenc .theora \
    .flac .speex .shout .faad .lame .twolame .ebml .matroska .ffmpeg \
    .dvdcss .libdvdread .dvdnav .dvbpsi .live .caca .mod .fontconfig \
    .png .jpeg .tiff .gpg-error .gcrypt .gnutls .cddb .cdio .vcdimager \
    .glib .gecko .mpcdec .dirac_encoder .dirac_decoder \
    .dca .tag .x264 .lua .zvbi .fontconfig .ncurses .liboil \
    .schroedinger .libass .libupnp .kate .Sparkle .aclocal
# .expat .clinkcc don't work with SDK yet
# .glib .IDL .gecko are required to build the mozilla plugin
# .mozilla-macosx will build an entire mozilla. it can be used if we need to create a new .gecko package

