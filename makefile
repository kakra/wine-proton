-include Makefile

PROTON_DIST := "dist-$(shell git describe --tag)"

CCFLAGS := -O3 -march=native -fomit-frame-pointer

CFLAGS += $(CCFLAGS)
VKD3D_CFLAGS := -I$(shell pwd)/vendor/vkd3d/include

CONFIGURE_FLAGS = \
	--disable-tests \
	--prefix=/ \
	--with-vkd3d \
	CFLAGS="$(CFLAGS)" \
	VKD3D_CFLAGS="$(VKD3D_CFLAGS)"

default: rebuild

rebuild-vkd3d:
	$(MAKE) -C vendor/vkd3d rebuild

rebuild-wine32: rebuild-wine64
	$(MAKE) -C build/wine32 PKG_CONFIG_PATH=/usr/lib32/pkgconfig

rebuild-wine64:
	$(MAKE) -C build/wine64

rebuild: rebuild-vkd3d rebuild-wine64 rebuild-wine32

clean-dist:
	rm -Rf dist/

clean-vkd3d:
	$(MAKE) -C vendor/vkd3d clean

clean-wine32:
	$(MAKE) -C build/wine32 PKG_CONFIG_PATH=/usr/lib32/pkgconfig clean

clean-wine64:
	$(MAKE) -C build/wine64 clean

clean: clean-vkd3d clean-wine64 clean-wine32

configure-vkd3d: clean-vkd3d
	$(MAKE) -C vendor/vkd3d configure

configure-wine32: clean-wine32 configure-wine64
	mkdir -p build/wine32
	cd build/wine32 && ../../configure --with-wine64=../wine64 $(CONFIGURE_FLAGS) PKG_CONFIG_PATH=/usr/lib32/pkgconfig

configure-wine64: clean-wine64
	mkdir -p build/wine64
	cd build/wine64 && ../../configure --enable-win64 $(CONFIGURE_FLAGS)

configure: configure-vkd3d configure-wine64 configure-wine32

install-vkd3d:
	$(MAKE) -C vendor/vkd3d install

install-wine32: clean-dist install-wine64
	$(MAKE) -C build/wine32 DESTDIR=$(shell pwd)/dist PKG_CONFIG_PATH=/usr/lib32/pkgconfig install-lib

install-wine64: clean-dist
	$(MAKE) -C build/wine64 DESTDIR=$(shell pwd)/dist install-lib

install-lib: clean-dist install-wine64 install-wine32

dist-vkd3d: install-vkd3d
	$(MAKE) -C vendor/vkd3d dist

dist: install-lib dist-vkd3d
	tar xf vendor/vkd3d/dist.tar.xz
	tar cf - dist/ | xz -T0 >$(PROTON_DIST).tar.xz
	ln -snf $(PROTON_DIST).tar.xz dist.tar.xz
