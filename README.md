# Unofficial SteamPlay Proton

This is an unofficial version of Proton rebased to more current wine version,
including various patches to fix bugs and improve performance. It is also a
testbed for work-in-progress implementations.

Do not report problems with this version upstream to Valve, report them here
instead.


## Summary / Headlines

- Based on Proton 3.16 and vanilla wine 4.x
- Performance-optimized one-size-fits-all SteamPlay compatibility layer
- Easy drop-in installation into an existing Proton installation
- Easy build-from-source workflow


## Target Audience

This distribution targets people with some technical skills to benefit from
an optimized Proton built. It contains experimental patches and minor
problems should be expected. Thus, it is expected that users are comfortable
with analyzing and reporting bugs patiently. This project is a spare-time
project. I am not able to fix all problems on my own and need your help and
cooperation.

If you're not willing to accept one or another patch, and to participate in
analyzing, fixing, and testing a problem, this project is not for you.


## Goal of this Project

The goal of this project is primarily to fix selected games so we can run
them with SteamPlay properly and with good performance. A secondary goal is
to grow and improve some patches that could hopefully be upstreamed to proton
or wine.

It also aims at providing more consistent performance in games. This means it
doesn't try to squeeze out the last few fps possible but instead reduce lag
and stutter, thus having a better overall average fps rate with more
consistent frame times.


## Troubleshooting

First, check this README (it may have been updated between version of my
build) if you find anything about the particular issue you're experiencing.
If it is not listed, check https://github.com/kakra/wine-proton/issues for
known/solved problems (look for closed issues, too). You may comment on
closed issues, I'll follow up on those.

If you didn't find anything, let's start with logging. I need the output
of `cat /proc/cpuinfo` and the SteamPlay log file. Edit `user_settings.py`
in the Proton folder to include:

```python
"WINEDEBUG": "fixme+all",
```

Now, run the game. It will create a log file, pick the latest, it has the
steam game id of the game you've run in its name:

```bash
ls -lt $HOME/steam-*.log
```

Check the file for obvious problems, the origin cause may be listed somewhere
in the middle of the file, don't just look at the end. The first serious
problem shown is probably causing your problem.

You may leave the logging enabled as `WINEDEBUG=fixme+all` has only a minor
performance overhead with my build currently.

Here's an example from https://github.com/kakra/wine-proton/issues/8:

The log file shows:

```
info:  Game: PathOfExile_x64Steam.exe
info:  DXVK: v0.95-25-g4f58c2b
warn:  OpenVR: Failed to locate module
info:  Required Vulkan extension VK_KHR_get_physical_device_properties2 not supported
info:  Required Vulkan extension VK_KHR_surface not supported
info:  Required Vulkan extension VK_KHR_win32_surface not supported
DxvkInstance: Failed to create instance
err:   DxvkInstance: Failed to create instance
```

Wine crashes with a read fault later due to this:

```
Unhandled exception: page fault on read access to 0x0002fc50 in 64-bit code (0x0000000140614095).
[...]
System information:
    Wine build: wine-4.0-rc7
    Platform: x86_64
    Version: Windows 7
    Host system: Linux
    Host version: 4.20.3-arch1-1-ARCH
```

So the origin cause is the DXVK error. Essentially it says that Vulkan is not
fully supported. The solution was to install the required support drivers
for `AMD Readeon`.

If you still need help or think your findings are worth a troubleshooting
example here, feel free to open an issue report.


## Development Workflow

The patches are regularly rebased onto wine/proton updates. Thus, the main
branch is not suitable for git-merge workflows.

Tagged releases are available to checkout specific repository versions.
Patches should be sent based on either vanilla wine, wine-proton or this
repository branch. Monolithic patches are not acceptable (you would find
those most commonly in Lutris or similar distributions). The atomic-commit
approach allows you to filter out any SteamPlay-related patches easily
through an interactive rebase. That way you could craft a version that runs
outside of Steam.

Patch series introducing too many rebase conflicts will most likely be
dropped from this repository, i.e. it used to have the D3D11 deferred context
patches and the PBA patches. But wine master seems to work in this area a lot
currently, and thus these patches were dropped. Dropped patches are unlikely
to return into this repository if they don't or cannot provide a benefit,
i.e. DXVK as a winelib renders D3D11 deferred context patches and PBA patches
mostly useless. I later tweaked the winelib build to properly work with
Proton wined3d/dxvk selection but I currently have no plans in putting the
other patches back in.


## Hints to 32-bit Users (applies also to SysWOW64)

This repository includes DXVK. This can be a major problem for 32-bit users
because DXVK uses more memory than the native wine implementation of
DX10/DX11. You will most probably run out of address space early. Games won't
run well in 32-bit mode. It is thus recommended to use 64-bit games only.
However, turning DXVK off the Proton-way works now.

Pulseaudio users can somewhat improve the situation by limiting the SHM size
of pulseaudio to 1 MB. Simply add `shm-size-bytes=1048576` in
`/etc/pulseaudio/daemon.conf` and restart. This version is also compiled
without debug support to reduce the memory footprint.

With transparent hugepages it could make more sense to use bigger shared
memory sizes to align nicely with hugepages: `shm-size-bytes=2097152`. But
I'm not sure if this could provide any benefit.


## Hints regarding Memory Allocation Issues

While testing, I found that DXVK can interact badly with the transparent
huge pages of the kernel. While overall performance seems a little bit
smoother, it fails early to allocate more DirectX memory even before you've
run out of VRAM. In my tests, this situation can be mostly avoided by running
a kernel with `CONFIG_TRANSPARENT_HUGEPAGE_MADVISE=y` instead of
`CONFIG_TRANSPARENT_HUGEPAGE_ALWAYS=y`. This still results in a little less
smooth experience while keeping the benefits of huge pages for applications
that actively ask for it. So far, DXVK crashes due to memory allocation are
gone for me with this setting. If your kernel doesn't set either of these
options, you're probably safe.

Hint: To properly support my large pages patch, you need to enable
`CONFIG_TRANSPARENT_HUGEPAGE_MADVISE=y`.


## Getting Started with Transparent Hugepages

If you want to experiment with this, I can recommend starting with the
following settings:

```bash
#!/bin/sh
# /etc/local.d/transparent_hugepages.start
echo within_size >/sys/kernel/mm/transparent_hugepage/shmem_enabled
echo always >/sys/kernel/mm/transparent_hugepage/enabled
echo defer+madvise >/sys/kernel/mm/transparent_hugepage/defrag
echo 128 >/sys/kernel/mm/transparent_hugepage/khugepaged/max_ptes_none
```

This would enable a sane feature set with the potential of exposing the
memory allocation issue observed above.


## Reducing Stutters due to "allocstall"

Games tend to allocate and free vast amounts of memory especially during
loading screens. This puts the memory manager of Linux under a lot of
pressure. When this becomes a problem, you may see multi-second stalls and
the system freezes for short periods of time (coming back to life with HDD
activity). Audio usually also stops for the duration of the freeze. What
you're seeing is probably an effect called "allocstall" in Linux.

You can watch the numbers here:

```bash
grep allocstall /proc/vmstat
```

I found that there's great potential of removing this effect by adjusting
kernel parameters but YMMV depending on your system. This effect is most
visible to me in SOTTR and PoE. To improve this issue, create the following
file:

```ini
# /etc/sysctl.d/99-gaming.conf
vm.vfs_cache_pressure = 50
vm.watermark_scale_factor = 200
```

The first parameter (default 100) will instruct the kernel to prefer keeping
cache memory around and instead swap out process memory you currently don't
need during gaming (idle background applications).

The second parameter (default 10) will instruct the kernel to wake kswapd
more early and more often to keep a minimum amount of free memory for
latency-free allocations. This is the most prominent parameter to reduce
allocation stalls. As far as I could research, the scale of this parameter
has changed at some kernel version, it's now a factor of 0.1%. If you're
running an old kernel, a value of "200" may be a bad idea. Kernel 4.9 or
later should be safe, this is around the time when I started testing this.

You can activate the settings immediatly by running

```bash
sysctl -p /etc/sysctl.d/99-gaming.conf
```

or you just reboot. If you want to test the settings first, don't put the file
into `/etc/sysctl.d` just yet.

Be careful to not adjust the parameters too far away from the defaults or too
extreme as this may have very negative effects (it could OOM-kill your whole
session, leaving you unable to login even after reboot).

Users of btrfs benefit especially from this as btrfs is quite a heavy user of
memory allocations. If you don't think you're experiencing allocation stalls,
I recommend to not fiddle with these settings. If you're low on memory, I
recommend to adjust these settings only very carefully (8 GB or less of RAM).

As a result, you should see the kernel keeping more unused memory around (so
neither cached nor used). This memory is then available to latency free
allocations. Having reduced latency for memory allocations comes at the cost
of loosing some otherwise available memory: It's not really lost because it's
available when needed but the kernel takes care of freeing some other memory
instead early then in the background - so it's kind of a buffer for memory
allocations that doesn't involve swapping or compacting (which introduces
latency spikes otherwise).

To return to defaults, just remove the file and reboot.


## Changelog and Patch Status

This is a summary of the current patch series status of patches applied to
vanilla wine:

- [dxvk] Add more verbose error reporting
- [dxvk] Add triple-buffering by default if vsync is enabled for more consistent FPS rates, reduces stuttering especially under compositors
- [dxvk] Build an optimized winelib by default for the native CPU (obviously mostly a benefit if you compile yourself)
- [hack] Pretend the user has SeLockMemoryPrivilege to support games with large page support
- [local] Custom makefile to build the complete distribution with one or two easy commands, optimized for the native CPU
- [local] Pre-defined wine.inf DLL overrides specific to fixing problems in games
- [local] Raised priority on winebus.sys threads to reduce input lag and prevent missed button/axis events
- [local] Reworked scheduler priorities implementation to fight priority inversion problems
- [local] Silenced some very noisy FIXMEs and ERRs for better performance with default logging
- [local] Simple/limited AVRT implementation to support native xaudio2 to gain realtime priority
- [local] Simple/limited implementation of SetThreadIdealProcessor and friends for better cache utilization in some games (especially with modern CPU designs)
- [local] Some cleanups obviously forgotten by the proton devs
- [proton] Based on Proton 3.16 rebased to Wine 4.x
- [proton] Includes updated esync patches from zfigura
- [staging] Patch to enable Path of Exile to use ping for automatic gateway selection
- [staging] Patches to enable games detecting more information of the hardware configuration
- [staging] Realtime priorities support patches from wine-staging
- [staging] Reduced CPU overhead of DirectSound audio mixing
- [staging] Shared-memory performance patches from wine-staging
- [staging] Staging patch series for windowscodecs to correctly load Path of Exile in D3D11 mode
- [staging] Use explicit windows activation to prevent some games from starting with an initially invisible window
- [vulkan] Current vulkan headers to correctly build wine/vkd3d
- [wip] Current vkd3d library to support D3D12 games (not ready for use, not working yet)
- [zfigura] Some patches by zfigura to fix windows minimize/iconize/restore


## Subproject Patch Status

This is a summary of the patches in the subprojects. The objective here is to
keep the amount of patches small, if possible they should be upstreamed
instead:

- [dxvk] Add more verbose error reporting
- [dxvk] Add triple-buffering by default if vsync is enabled for more consistent FPS rates, reduces stuttering especially under compositors
- [dxvk] Build an optimized winelib by default for the native CPU (obviously mostly a benefit if you compile yourself)


## Upstreamed Patches

The following patches have been upstreamed so everyone running Proton can
benefit from it:

- [dxvk] Add `SCHED_BATCH` scheduling and low priority to the state cache shader compiler, reduces audio stutter in loading screens and increases throughput of the compiler (DXVK 0.93, needs wine patches included here)
- [fix] Corrected Xbox One S wireless controller support in winebus.sys/dinput (Wine 4.0 RC2)
- [proton] winex11.drv: Bypass compositor in full-screen mode (Proton 3.16)
- [staging] Locking and threading improvements for a small performance benefit (Proton 3.16 Beta)
- [staging] Patches to enable games to see each pulseaudio devices individually (Proton 3.16 Beta)


## Dropped Patches

The following patches have been dropped because either an issue has been
fixed upstream, or the patch conflicted and wasn't important:

- [hack] Refined CoInitialize hack to correctly load Path of Exile in D3D11 mode
  - Path of Exile: Fixed upstream in Wine and PoE itself (Wine 4.1)
  - Sound issues in some games fixes


## Patch Tooling

A work-in-progress list of applied patches is kept in `docs/patches` in YAML
format. The current design is a manually crafted inverse dependency list. In
the future it should generate this tree automatically from staging definition
files. The leafs of this tree are the patche series applied to this
repository.

Running `tools/mkpatchlist.rb` will generate a list of files suitable to be
fed to `git am`.

You need to point the tool to the staging patches repository by creating a
symlink to its patches directory, e.g.:

```bash
echo "/patches" >>.git/info/exclude
ln -s ../wine-staging/patches
```

Now run `tools/mkpatchlist.rb` to compile a list of patch files for `git am`.
In the future this tool will be part of a toolchain to recreate this wine
branch from scratch on top of a vanilla wine branch. Also in the future, it
will list the Proton patches as individual patch series to make crafting an
individual wine version more easy. I'll then push a wine patches repository
forked from the staging patches repository.


## Help Needed

There are a few patches and features I'd appreciate one or another helping
hand for:

- Add Feral GameMode support to spawning the main process
- Add the missing bits to create a self-contained proton distribution suitable to be made available in the SteamPlay menu selection
- Improve AVRT implementation and make wine multimedia DLL implementations use it
- Improve scheduler priorities implementation to make it acceptable for upstream
- Look into the large page support patches


## How to Benefit from the Scheduler Patches

This version of wine-proton includes a reworked realtime priorities patchset
based on staging. It enables setting thread priorities by default and can use
`SCHED_ISO` to support multimedia realtime workloads better. But to take full
advantage of high priorities, you need to adjust pam limits and rtkit. Given
that your login user is member of the `users` group:

```
# /etc/security/limits.d/99-wine-proton.conf
@users - rtprio 90
@users - nice -15
```

Depending on your distribution, you may also need to install and configure
rtkit or RealtimeKit. How to do this in your distribution is out of the scope
of this project. Google is your friend.

Changes to these configuration files require a restart of your login session.

Games may also benefit running a `SCHED_ISO`-enabled kernel. Distributions
usually ship an alternative kernel you'd need to install that has the CK or
PF patchset. Both variants offer the `SCHED_ISO` feature (through the MuQSS
or PDS-MQ schedulers). How to install and switch the kernel is also not the
scope of this project. Your favorite distribution should have a wiki
explaining that in more detail than this project could ever do.

There's also Feral GameMode which can temporary switch your CPU from a
powersave mode to performance mode. DXVK recommends to do exactly that. To
use GameMode, you have to install the daemon, then add the relevant library
preload instruction to the Steam launcher commandline in the options of the
specific game. I'm currently planning to integrate support for GameMode
natively into wine-proton so all games can benefit once you installed
GameMode.


## Games Status

This repository was developed while actively playing and testing the
following games and journalling my development progress.

My system is a medium-aged system with the following properties:

- Gentoo/Linux Base System (default/linux/amd64/17.0/desktop/plasma/systemd)
- ASrock Z68 Pro3 mainboard
- Intel Core i7-3770K @ 3.5 GHz (turbo boost 4.0 GHz, 8 threads)
- 16 GB DDR3-1333 system memory
- Nvidia Geforce GTX-1050 Ti 4 GB VRAM (proprietary driver)
- Xbox One S Wireless Controller (bluetooth mode, xpadneo driver)


### Middle-Earth: Shadow of War

My game progress: Late into Act 2, no crashes observed yet

The game runs exceptionally well even with higher graphics settings. I've
added a patch to make ntdll.dll support large page mode (which translates to
transparent huge pages in Linux). This setting can be enabled in the game
options and theoretically improves performance. Memory throughput of huge
pages can be up to 10% better.

But a deeper look at system metrics and wine logs shows that the game
currently doesn't seem to do the correct VirtualAlloc() call, probably it was
removed due to a bug in Windows 10 handling this properly (memory corruption
due to delayed memory zeroing). It would also explain why Steam forums often
report absolutely no difference between both settings in the game. OTOH, my
patch may just be incomplete: Help very much appreciated.

If the game uses scaled-down render resolution, find `render.cfg` and edit
the following lines to show your desired resolution:

```
"ScreenHeight16by9" "1080.000000"
"ScreenWidth16by9" "1920.000000"
```

After this change, do not open the graphics settings menu again, or your
change will be overwritten.

- Added large page mode to ntdll.dll
- Hacked advapi32 to announce the needed privilege
- The game may use scaled-down render resolutions due to detecting the display setup wrong


### Shadow of the Tomb Raider (SOTTR)

My game progess: 100% completion, rare crashes or freezes

The game generally runs at only half the speed it could achieve, CPU and GPU
are only used 50% at maximum. This is probably a CPU/GPU-sync problem. You
probably need to reduce the graphics settings a lot if that happens. Also
switch to SMAA if you experience the black pixels bleeding bug. There are
reports that setting texture quality too high may result in slowly decreasing
performance over time during gameplay.

- Improved in-game benchmark performance from ~19 to ~33 fps
- Solved priority inversion problems resulting in multi-second freezes around every minute in some areas
- No more freezes during OOM conditions of the GPU
- Reduced/eliminated fps stuttering
- Gamepad triggers may hang sometimes resulting in your bow not shooting
- Game complains about a missing graphics card, just ignore that and switch to DX11

**See also:** Notes about "allocstalls" above.


### The Witcher 3: Wild Hunt (TW3)

My game progress: Complete walk-through including almost all side quests,
very rare crashes/freezes observed

My installation uses the unofficial game-developer mod and HD texture patch.
The HD Reworked Project includes configuration updates to provide better
performing texture streaming:
https://www.nexusmods.com/witcher3/mods/1021?tab=files

- Reduced stuttering / fps dips while moving around
- Gamepad triggers may hang sometimes resulting in wrong actions during combat which should be fixed by my input-lag patches


### DOOM 2016

My game progress: Somehwere mid-game, multiplayer has been tested and worked,
no crashes or performance issues yet

I'm running this game in Vulkan mode. Game startup after switching the
graphics engine takes a huge amount of time, be patient. The game runs at a
hard 60 fps limit (due to vsync) with CPU/GPU usage around 25%.

- Reduced (if not eliminated) constant sound stutters / crackling
- Improved susceptibility to priority inversion (showed only with staging real-time patches)


### Ori and the Blind Forest: Definitive Edition (OriDE)

My game progress: Somewhere around 85%, no glitches or crashes yet

Initial loading screen may take a very long time with a very slow animation
of Ori. Be patient, it will eventually continue after 2-3 minutes. Subsequent
runs do not show this problem. In-game feature videos make the game crash
here which probably results from an improperly compiled steam runtime (tries
to load a non-existing video decoder driver).

- Runs just fine besides the above mentioned minor issues


### Mirror's Edge

My game progress: Complete story walk-through, rare crashes/freezes
observed

The game may freeze in a busy-loop during some scripted scenes, the exact
problem is yet unknown. Try to progress with vanilla wine if that happens.
There's no other chance around that currently.

- Reduced/eliminated fps stutters
- Fixed window minimize/restore
- Fixed game window starting up invisible


### Path of Exile (PoE)

My game progress: Beginning of Act 9, no crashes observed but one or another
graphic/audio glitch, including bad performance/lags (which occur only once
per new effect/map/enemy model)

I'm running the game windowed maximized in 4k resolution with around 40 fps
on a GTX 1050 Ti. The game needs a warm-up phase before it runs smoothly.
Subsequent runs benefit from the pipeline state cache of DXVK, greatly
reducing or even eliminating lags during gameplay. If you experience login
problems wait for the harddisk to calm down before logging in. Resizing the
game window or switching resolution most likely results in a crash. In that
case edit the config file in an editor.

There seem to be audio glitches since inclusion of FAudio which results
in improperly placed sound sources and/or audio sources disappearing from
playback altogether, eventually resulting in a muted game at some point. A
restart of the game is required then. Crashes were not observed due to this.

- More consistent fps
- Reduced fps dips while walking around
- Support for pinging server gateways in the login lobby
- Reduced audio stutters during the logo video
- Reduced audio stutters early in the menu screen
- Improved timeout situations in the login process
- It actually runs in DX11 64-bit mode (yay)

**See also:** Notes about "allocstalls" above.


### Hellblade: Senua's Sacrifice (Hellblade)

My game progress: Somewhere late in the game, only minor audio issues
observed (which may be a bit annoying given a nature of this game) and one
or two crashes/freezes, nothing game breaking tho

- Reduced sound stutters
- More consistent fps


### Skyrim Special Edition (SkyrimSE)

My game progress: I'm not sure, this game is huge. I used to play the
original Skyrim 2011 and eventually completed all the DLCs with most side
quests and this one is just a different walk-through. The game tends to
freeze once in a while which is probably related to DXVK memory allocations.

My installation uses a hand full (or two) of immersion and graphic mods. The
Creation Engine tends to partly freeze if subsystems do not work correctly
(either in scripted scenes or during loading screens). This problem still
exists even with the inclusion of FAudio. Thus, a registry patch is included
which forces xaudio2_7 on the game. The scheduler improvements seem to get rid
of most audio cracklings and stutters. Another problem may be DXVK memory
allocations, in that case reduce the texture pressure by lowering settings.

- Reduced sound stutters/cracklings
- Forced xaudio2_7 usage for proper audio support


### Kingdoms of Amalur: The Reckoning

My game progress: Some DLCs completed, first continent almost completed,
probably somewhere around 50% of the game, no glitches or crashes yet

- Fixed window minimize/restore
- Fixed game window starting up invisible
- Fixed resolution switching
- Fixed gamepad support
- Fixed audio distortion in the intro video


## Installation

Currently, just unpack the dist.tar.xz into your existing Proton folder.
This version includes DXVK as a winelib but modified to properly work with
the Proton-way of disabling `user_settings.py`. This is done by renaming
the `.dll.so` files to just `.dll` and moving them to the expected location.
Wine seems to have no objections in loading ELF binaries as native DLLs. You
don't have to take care of this, it's built this way out-of-the-box.

To make full use of the scheduler improvements, I recommend running a kernel
with support for `SCHED_ISO`. The CK kernel is one such example. You also
need to adjust your pam limits for `NICE` and `RTPRIO`. In contrast to the
staging patchset this distribution automatically enables and tunes for
available priorities after you adjusted the limits - no need to set
environment variables. The patches were not tested with a non-MuQSS kernel
(those without `SCHED_ISO` support), especially session-based task
auto-grouping may have an impact on performance behavior.


## Cloning the Repository and Setup

The first time you want to use this repository, you have to clone it and
initialize the submodules:

```bash
git clone https://github.com/kakra/wine-proton.git
cd wine-proton
git submodule update --init
```


## Compiling from Source

This should be easy... unless not. Let me know if you're having troubles.
This project uses a custom build system to build most dependencies from
scratch without chroot or cross-compile toolchains (except `winegcc`). Start
by checking out the git source and submodules. Then run:

```bash
# Run some preparation scripts first
pushd vendor/vkd3d && ./autogen.sh && popd

# Configure the project (optional, missing makefiles will be built automatically)
make configure

# Option 1: Build a tarball with bundled DXVK (default)
make dxvk-dist.tar.xz

# Option 2: Build a tarball without bundled DXVK
make dist.tar.xz

# If something went wrong, start over:
make clean
```

You can use `make -j$(nproc) ...` to speed up compilation. The dependencies
between subprojects should be safe.

Updates to build instructions (generated makefiles) are not tracked correctly
in most upstream build systems. Thus, if you change optimization flags, I
recommend doing `make clean` followed by `make configure` to rebuild from
scratch. Otherwise, you may experience strange issues.


## Working with the Repository

For this project, I decided to use a rebase workflow because it fits me best
for following multiple upstreams: The main upstream will be followed using
`rebase`, then I selectively `cherry-pick` commits from other upstreams. I
can checkout git worktree of those other projects if I plan to pick bigger
chunks of commits into other branches: Just rebase onto an upstream commit,
copy multiple lines from the editor, then paste those into another rebase.
I usually follow such a workflow to follow up on staging patchsets.

Since the development workflow is based on a rebase pattern, you'd need to
add the upstream wine project as a git remote if you intend to rebase parts
of the commits:

```bash
git remote add winehq git://source.winehq.org/git/wine.git
```

To follow up on updates, just run the second line. It will pull updates from
both my github project and wine upstream. This alone only fetches the commits
without applying anything to the source code branch yet.

After updating the remotes, you can run an interactive rebase to do whatever
you like to do with the commits, i.e. you could manually filter out all
Steam API commits to remove the dependency to Steam:

```bash
git remote update
git rebase -i winehq/master
```

To follow up on updates from my repository, given you named it `origin`,
instead use:

```bash
git remote update
git rebase origin/rebase/proton_3.16
git submodule update --init --rebase
```

If you do not need to float your own patches/commits around and did not make
changes to the source files, you can simply reset instead:

```bash
git remote update
git reset --hard origin/rebase/proton_3.16
git submodule update --init --force
```

An editor opens: Put a hash sign `#` in front of each commit you'd like to
omit. If something doesn't work during rebase and you're not sure how to
proceed, you can abort the rebase:

```bash
git rebase --abort
```

If you screwed up, just run

```bash
git reset --hard origin/rebase/proton_3.16
```

where `origin` is my remote - replace with whatever you named it. The default
is `origin` if you just cloned directly from my github project to your
computer. This is also a command that you could use to reset your clone to
my latest version but be aware that it will throw away any changes you made.

I keep my own patches floating at the top of the upstream commits so I can
easily track them. Before resetting to an upstream branch, you may want to
first backup your own branch so you can cherry-pick changes back into
upstream:

```bash
git branch backup/some-backup-name
```

Be sure to have all your local changes committed to git before resetting,
because `reset --hard` will also throw away uncommitted changes.

To make using this workflow a little better and comfortable, I can recommend
looking into the `tig` tool and make yourself familiar with its usage. It's
a small CUI utility to browse git repositories, do cherry-picks, and resolve
conflicts (and probably a lot more).

Also, I recommend looking into `git worktree` to create fast worktree copies
sharing the same commit database.


## Getting Started with Gentoo Linux

This repository is natively developed using Gentoo Linux. You'll need to
install some basic dependencies to start using this:

```bash
# Be sure that you're running a multilib system, every desktop profile
# should do, e.g. default/linux/amd64/17.0/desktop:
eselect profile list

# Install the building and development environment:
USE="mp3 sdl opengl vulkan vkd3d" ABI_X86="32 64" emerge -DNua git '>=wine-vanilla-3.16'
```

You should add the needed useflags to your `make.conf`. Because Gentoo is a
source-based distribution, all required headers and libraries are installed
now.


## Getting Started with Arch Linux

This repository is developed using Gentoo Linux. But due to the popularity
of Arch Linux, I'll try listing the steps to get this started on Arch Linux.
These steps have been tested in a fresh 64-bit Arch Linux nspawn container:

```bash
# Follow the instructions for enabling Arch Linux multilib at
# https://wiki.archlinux.org/index.php/official_repositories, then
# the following command should work:
pacman -Sl multilib

# Install all building essentials:
pacman -Syu base-devel meson

# Install development dependencies:
pacman -Syu glslang opencl-headers

# Install multilib and feature dependencies:
pacman -Syu krb5 libxslt
pacman -Syu lib32-krb5 lib32-libxslt
pacman -Syu gst-plugins-base mpg123 openal sdl2
pacman -Syu lib32-gst-plugins-base lib32-mpg123 lib32-openal lib32-sdl2
pacman -Syu vulkan-icd-loader libxcomposite libxinerama
pacman -Syu lib32-vulkan-icd-loader lib32-libxcomposite lib32-libxinerama

# Install dependencies from AUR:
# (HINT: You may need to set JAVA_HOME to jdk8)
pakku -Syu ffmpeg spirv-tools
pakku -Syu lib32-ffmpeg lib32-spirv-tools

# Install wine development toolchains to easily bootstrap the project:
pacman -Syu wine
```

**HINT:** You will need to get some package from AUR, this guide uses pakku
to install the following packages:

  * lib32-spirv-tools
  * lib32-ffmpeg

I'm not familiar with the Arch ecosystem, feel free to suggest a better
alternative.


## Getting Started with Ubuntu (WIP)

This repository is developed using Gentoo Linux. But due to the popularity
of Ubuntu, I'll try listing the steps to get this started on Ubuntu. These
steps have been tested in a fresh 64-bit Ubuntu Cosmic nspawn container:

```bash
# Install all building essentials:
dpkg --add-architecture i386
apt-get update
apt-get install autoconf build-essential cmake gcc-multilib g++-multilib make meson pkg-config

# Install development dependencies:
apt-get install glslang-tools libavcodec-dev libsdl2-dev libvulkan-dev spirv-tools
apt-get install libavcodec-dev:i386 libvulkan-dev:i386 libsdl2-2.0-0:i386

# Install wine development toolchains to easily bootstrap the project:
apt-get install bison flex wine64-tools
apt-get install libfreetype6-dev xorg-dev
```

The current state is "not working" because libsdl2-dev is not compatible
to a multilib development install in Ubuntu yet, read: You cannot install
both architecture packages at the same time. But there's a somewhat hacky
work-around:

```bash
cd /tmp
apt-get download libsdl2-dev:i386 spirv-tools:i386
dpkg -x libsdl2-dev_*_i386.deb .
dpkg -x spirv-tools_*_i386.deb .
cp -r usr/lib/i386-linux-gnu /usr/local/lib/
```

Current status:

- FAudio builds multilib: `make dist-faudio`
- vkd3d builds multilib: `make dist-vkd3d`
- wine does not configure due to missing 32-bit dev dependencies
	- 64-bit misses headers: `make configure-wine64`
	- 32-bit fails: `make configure-wine32`
- wine probably won't build
- DXVK builds but fails to link due to winevulkan: `make dist-dxvk`
	- it requires wine to be built first
	- this probably needs more adjustments, my system provides `winevulkan`


## Caveats

If you experience audio issues, try first reverting your pulseaudio changes
if you applied some. Things coming to mind are adjusted pulseaudio fragment
buffers and `PULSEAUDIO_LATENCY_MSEC`.

Steam may randomly decide to replace parts of this distribution with its own
version of wine. Just unpack the archive again if that happens.

The pre-built binary version was optimized for an i7-3770K. If your CPU is
incompatible with this instruction set, you'll have to compile your own
version. If you see errors about invalid instructions in `dmesg`, then this
is what caused the crash.


## Disclaimer

This is not official Valve work. I take no responsibility for damages
affecting your system, saved games, including game server bans due to using
modified DLLs. Use at your own risk.
