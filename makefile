CONFIGURE := configure

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
	--without-capi \
	--without-cups \
	--without-gphoto \
	--without-hal \
	--without-oss \
	--without-sane \
	CFLAGS="$(CFLAGS)" \
	FAUDIO_LIBS="$(FAUDIO_LIBS)" \
	FAUDIO_CFLAGS="$(FAUDIO_CFLAGS)" \
	VKD3D_LIBS="$(VKD3D_LIBS)" \
	VKD3D_CFLAGS="$(VKD3D_CFLAGS)"

ifndef VERBOSE
CONFIGURE += --quiet
.SILENT:
endif

dist:: dxvk-dist.tar.xz


# Just rebuild

rebuild-dxvk::
	+$(MAKE) -C vendor/dxvk rebuild

rebuild-faudio::
	+$(MAKE) -C vendor/faudio rebuild

rebuild-vkd3d::
	+$(MAKE) -C vendor/vkd3d rebuild

rebuild-wine::
	+$(MAKE) -C build/wine64
	+$(MAKE) -C build/wine32 PKG_CONFIG_PATH=/usr/lib32/pkgconfig

rebuild:: rebuild-dxvk rebuild-faudio rebuild-vkd3d rebuild-wine


# Clean projects

clean-dxvk::
	+$(MAKE) -C vendor/dxvk clean

clean-faudio::
	+$(MAKE) -C vendor/faudio clean

clean-vkd3d::
	+$(MAKE) -C vendor/vkd3d clean

clean-wine::
	rm -Rf build/

clean:: clean-dxvk clean-faudio clean-vkd3d clean-wine
	rm -Rf dist.tar.xz vendor/*/dist/ vendor/*/dist.tar.gz
	echo "Cleaned up..."


# Make makefiles of projects

makefiles:: build/wine64/Makefile build/wine32/Makefile
	+$(MAKE) -C vendor/dxvk makefiles
	+$(MAKE) -C vendor/faudio makefiles
	+$(MAKE) -C vendor/vkd3d makefiles


# FAudio dependencies

build/wine32/Makefile: FAUDIO_LIBS = -L$(FAUDIO_DIST)/lib -lFAudio -lavcodec -lavutil -lswresample
build/wine64/Makefile: FAUDIO_LIBS = -L$(FAUDIO_DIST)/lib64 -lFAudio -lavcodec -lavutil -lswresample

vendor/faudio/dist/%.so::
	+$(MAKE) -C vendor/faudio $(@:vendor/faudio/%=%)


# vkd3d dependencies

build/wine32/Makefile: VKD3D_LIBS = -L$(VKD3D_DIST)/lib -lvkd3d -lvkd3d-shader
build/wine64/Makefile: VKD3D_LIBS = -L$(VKD3D_DIST)/lib64 -lvkd3d -lvkd3d-shader

# serialize the configure target
vendor/vkd3d/configure::
	+$(MAKE) -C vendor/vkd3d configure

vendor/vkd3d/dist/%.so:: vendor/vkd3d/configure
	+$(MAKE) -C vendor/vkd3d $(@:vendor/vkd3d/%=%)


# Configure distribution

configure: configure.ac
	autoconf

build/wine32/Makefile: configure makefile build/wine64/Makefile \
	vendor/faudio/dist/lib/libFAudio.so \
	vendor/vkd3d/dist/lib/libvkd3d.so
	echo "CPU is heating up..."
	mkdir -p $(dir $@)
	cd $(dir $@) && ../../$(CONFIGURE) --with-wine64=../wine64 $(CONFIGURE_FLAGS) PKG_CONFIG_PATH=/usr/lib32/pkgconfig

build/wine64/Makefile: configure makefile \
	vendor/faudio/dist/lib64/libFAudio.so \
	vendor/vkd3d/dist/lib64/libvkd3d.so
	echo "Configuring wine distribution..."
	mkdir -p $(dir $@)
	cd $(dir $@) && ../../$(CONFIGURE) --enable-win64 $(CONFIGURE_FLAGS)


# Install distribution

install-wine32: build/wine32/Makefile
	echo "Hold'ya horses while installing 32-bit wine libraries..."
	+$(MAKE) -C build/wine32 DESTDIR=$(abspath dist) PKG_CONFIG_PATH=/usr/lib32/pkgconfig install-lib

install-wine64: build/wine64/Makefile
	echo "Installing 64-bit wine libraries..."
	+$(MAKE) -C build/wine64 DESTDIR=$(abspath dist) install-lib

install-wine: install-wine64 install-wine32
	echo "Done installing wine libraries!"


# Bundle vendor project into the distribution

vendor/faudio/dist.tar.xz::
	echo "Adding the FAudio ingredient..."
	+$(MAKE) -C $(dir $@) dist.tar.xz

vendor/vkd3d/dist.tar.xz::
	echo "Stirring some vkd3d into wine..."
	+$(MAKE) -C $(dir $@) dist.tar.xz

vendor/dxvk/dist.tar.xz::
	echo "Spicing up with DXVK..."
	+$(MAKE) -C $(dir $@) dist.tar.xz

bundle-dxvkdist: bundle-dist
	+$(MAKE) vendor/dxvk/dist.tar.xz
	tar xf vendor/dxvk/dist.tar.xz

bundle-dist: install-wine
	echo "Bundling vendor libraries..."
	+$(MAKE) vendor/faudio/dist.tar.xz vendor/vkd3d/dist.tar.xz
	tar xf vendor/faudio/dist.tar.xz
	tar xf vendor/vkd3d/dist.tar.xz


# Create distribution

dist-cleanup::
	echo "Removing cruft from distribution..."
	rm -f $(filter-out $(patsubst %,dist/bin/%,$(PROTON_BINS)),$(wildcard dist/bin/*))
	rm -rf dist/share/man dist/share/applications dist/include dist/lib/pkgconfig

dxvk-dist.tar.xz::
	echo "Building a spicy wine distribution just for you, pleae wait..."
	rm -Rf dist/
	+$(MAKE) bundle-dxvkdist
	+$(MAKE) dist-cleanup
	tar cf - dist/ | xz -T0 >dxvk-$(PROTON_DIST).tar.xz
	echo "We are almost there..."
	ln -snf dxvk-$(PROTON_DIST).tar.xz dist.tar.xz
	echo "Current version symlinked to dist.tar.xz. Done. Have fun!"

dist.tar.xz::
	echo "Please chill, building a fine wine distribution..."
	rm -Rf dist/
	+$(MAKE) bundle-dist
	+$(MAKE) dist-cleanup
	echo "We are almost there..."
	tar cf - dist/ | xz -T0 >$(PROTON_DIST).tar.xz
	ln -snf $(PROTON_DIST).tar.xz dist.tar.xz
	echo "Current version symlinked to dist.tar.xz. Done. Have fun!"
