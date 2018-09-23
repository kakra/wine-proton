PROTON_BINS := wine wine64 wine64-preloader wine-preloader wineserver

PROTON_DIST := "dist-$(shell git describe --tag)"
FAUDIO_DIST := $(abspath vendor/faudio/dist)
VKD3D_DIST  := $(abspath vendor/vkd3d/dist)

CCFLAGS := -O2 -march=native -fomit-frame-pointer -g0 -fipa-pta

CFLAGS += $(CCFLAGS)

FAUDIO_CFLAGS := -I$(abspath vendor/faudio/include)
VKD3D_CFLAGS := -I$(abspath vendor/vkd3d/include)

CONFIGURE_FLAGS = \
	--prefix=/ \
	--with-faudio \
	--with-vkd3d \
	--without-hal \
	--without-capi \
	--without-oss \
	CFLAGS="$(CFLAGS)" \
	FAUDIO_LIBS="$(FAUDIO_LIBS)" \
	FAUDIO_CFLAGS="$(FAUDIO_CFLAGS)" \
	VKD3D_LIBS="$(VKD3D_LIBS)" \
	VKD3D_CFLAGS="$(VKD3D_CFLAGS)"

default:: dxvkdist

configure-wine32: FAUDIO_LIBS = -L$(FAUDIO_DIST)/lib -lFAudio -lavcodec -lavutil -lswresample
configure-wine64: FAUDIO_LIBS = -L$(FAUDIO_DIST)/lib64 -lFAudio -lavcodec -lavutil -lswresample

configure-wine32: VKD3D_LIBS = -L$(VKD3D_DIST)/lib -lvkd3d -lvkd3d-shader
configure-wine64: VKD3D_LIBS = -L$(VKD3D_DIST)/lib64 -lvkd3d -lvkd3d-shader

rebuild-dxvk::
	+$(MAKE) -C vendor/dxvk rebuild

rebuild-faudio::
	+$(MAKE) -C vendor/faudio rebuild

rebuild-vkd3d::
	+$(MAKE) -C vendor/vkd3d rebuild

rebuild-wine32:: rebuild-wine64
	+$(MAKE) -C build/wine32 PKG_CONFIG_PATH=/usr/lib32/pkgconfig

rebuild-wine64::
	+$(MAKE) -C build/wine64

rebuild:: rebuild-dxvk rebuild-faudio rebuild-vkd3d rebuild-wine64 rebuild-wine32

clean-dist::
	rm -Rf dist/

clean-devel::
	rm -Rf devel/
	cp -Rpl dist devel

clean-dxvk::
	+$(MAKE) -C vendor/dxvk clean

clean-faudio::
	+$(MAKE) -C vendor/faudio clean

clean-vkd3d::
	+$(MAKE) -C vendor/vkd3d clean

clean-wine::
	rm -Rf build/

clean:: clean-dxvk clean-faudio clean-vkd3d clean-wine

configure-dxvk:: clean-dxvk
	+$(MAKE) -C vendor/dxvk configure

configure-faudio:: clean-faudio
	+$(MAKE) -C vendor/faudio configure

configure-vkd3d:: clean-vkd3d
	+$(MAKE) -C vendor/vkd3d configure

configure-wine32:: configure-wine64
	mkdir -p build/wine32
	cd build/wine32 && ../../configure --with-wine64=../wine64 $(CONFIGURE_FLAGS) PKG_CONFIG_PATH=/usr/lib32/pkgconfig

configure-wine64:: clean-wine dist-faudio dist-vkd3d
	mkdir -p build/wine64
	cd build/wine64 && ../../configure --enable-win64 $(CONFIGURE_FLAGS)

configure-wine:: | configure-wine64 configure-wine32

configure::
	+$(MAKE) configure-dxvk
	+$(MAKE) configure-faudio
	+$(MAKE) configure-vkd3d
	+$(MAKE) configure-wine

install-dxvk::
	+$(MAKE) -C vendor/dxvk install

install-faudio::
	+$(MAKE) -C vendor/faudio install

install-vkd3d::
	+$(MAKE) -C vendor/vkd3d install

devel-wine32:: devel-wine64
	+$(MAKE) -C build/wine32 DESTDIR=$(abspath devel) PKG_CONFIG_PATH=/usr/lib32/pkgconfig install-dev

devel-wine64:: clean-devel
	+$(MAKE) -C build/wine64 DESTDIR=$(abspath devel) install-dev

install-wine32:: install-wine64
	+$(MAKE) -C build/wine32 DESTDIR=$(abspath dist) PKG_CONFIG_PATH=/usr/lib32/pkgconfig install-lib

install-wine64:: clean-dist
	+$(MAKE) -C build/wine64 DESTDIR=$(abspath dist) install-lib

install-lib:: install-wine64 install-wine32

dist-dxvk:: install-dxvk
	+$(MAKE) -C vendor/dxvk dist

dist-faudio:: install-faudio
	+$(MAKE) -C vendor/faudio dist

dist-vkd3d:: install-vkd3d
	+$(MAKE) -C vendor/vkd3d dist

bundle-dxvkdist:: bundle-dist dist-dxvk
	tar xf vendor/dxvk/dist.tar.xz

bundle-dist:: clean-dist dist-faudio dist-vkd3d install-lib
	tar xf vendor/faudio/dist.tar.xz
	tar xf vendor/vkd3d/dist.tar.xz

devel:: devel-wine32 devel-wine64

test:: rebuild-wine64
	mkdir -p prefix/test64
	$(MAKE) -C build/wine64 WINEPREFIX=$(abspath prefix/test64) test

dist-cleanup::
	rm -f $(filter-out $(patsubst %,dist/bin/%,$(PROTON_BINS)),$(wildcard dist/bin/*))
	rm -rf dist/share/man dist/share/applications dist/include dist/lib/pkgconfig

dxvkdist:: bundle-dxvkdist
	+$(MAKE) dist-cleanup
	tar cf - dist/ | xz -T0 >dxvk-$(PROTON_DIST).tar.xz
	ln -snf dxvk-$(PROTON_DIST).tar.xz dist.tar.xz

dist:: bundle-dist
	+$(MAKE) dist-cleanup
	tar cf - dist/ | xz -T0 >$(PROTON_DIST).tar.xz
	ln -snf $(PROTON_DIST).tar.xz dist.tar.xz
