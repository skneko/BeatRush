# Beat Rush
Beat Rush (BTR) is a simple rhythm game made for the Nintendo 3DS console using [devkitPro], based on the Japanese rhythm game [Muse Dash](https://ja.wikipedia.org/wiki/Muse_Dash).
Playing this game in a real 3DS requires a homebrew loader.

**Maintenance and scope notice:**
This is a university project. Features and termination are not guaranteed and are subject to change at any time. 
It is very likely this project will be archived after said university course is done.

## Building from source
### Targets overview
This project uses [GNU Make] in order to build the different targets available for the 3DS homebrew scene. Use the command `make [target]` with one of the targets specified in the following table after making sure you comply with the requirements. Some targets have particular additional requirements.

| Targets     | Action                                                                                    |
| ------------| ----------------------------------------------------------------------------------------- |
| 3ds         | Builds `BeatRush.3ds`. <sup>1</sup>
| 3dsx        | Builds `BeatRush.3dsx` and `BeatRush.smdh`.
| cia         | Builds `BeatRush.cia`. <sup>1</sup>
| citra       | Builds and automatically runs `citra` for testing.<sup>1,2</sup>
| citra-qt    | Builds and automatically runs `citra-qt` for testing and debugging.<sup>1,2</sup>
| elf         | Builds `BeatRush.elf`.
| release     | Release build, creates a `cia`, `3ds`, and a zip file containing the `smdh` and `3dsx`. <sup>3</sup>

**Notes:** 
* <sup>1</sup> This requires having [makerom] and [bannertool] in your `$PATH`.
* <sup>2</sup> This requires having [citra] installed and in your `$PATH`.
* <sup>3</sup> Requires [zip] in your `$PATH`. If you are on Windows you will need both [zip] and [libbz2.dll] in your `$PATH`.

### Instructions for 3DSX and ELF
#### Unix-like / macOS
1. If not already available, install [GNU Make].
2. Install [devkitPro], making sure the `DEVKITPRO` and `DEVKITARM` environment variables are set.
3. Run `make 3dsx` or `make elf` in the root directory. The recommended compiler is GCC.

#### Windows
Use a platform such as MinGW or msys2 in order to install [GNU Make] and the GCC compiler (recommended). Note that [devkitPro]
has installation instructions specific to Windows, including a graphical installer.
Once the toolchain has been setup, follow the instructions for Unix-like systems.

### Instructions for CIA and 3DS
#### All platforms
1. Follow instructions for 3DSX and ELF.
2. Install [makerom] and [bannertool], and then add them both to your `$PATH`.
3. Run `make cia` or `make 3ds` in the root directory.

### Instructions for release
#### Unix-like / macOS
1. Follow instructions for CIA and 3DS.
2. Run `make release` in the root directory.

#### Windows
1. Follow instructions for CIA and 3DS.
2. Install [zip] and [libbz2.dll], and then add them both to your `$PATH`.
3. Run `make release` in the root directory.


### Cleanup
To remove compiler generated files, run `make clean`.

## Launching
In order to launch the game in your PC you will need an emulator such as [citra]. Citra supports all formats generated. A shorthand for building and launching citra is available through the `make citra` command.

If you want to load this file in an actual 3DS, you will need homebrew capabilities. You have several options:
- *(recommended)* Build a CIA file and then install it using a title manager such as [FBI].
- Build a 3DSX file and then use a launcher such as Homebrew Launcher. Follow the [normal procedure for loading 3DSX files](https://www.cfwaifu.com/3ds-install-games-homebrew/)
or [convert it to a CIA file](https://www.cfwaifu.com/3ds-to-cia/).

[devkitPro]: <https://devkitpro.org/>
[citro2d]: <https://citro2d.devkitpro.org/>
[GNU Make]: <https://www.gnu.org/software/make/>
[citra]: <https://citra-emu.org/download/>
[makerom]: <https://github.com/profi200/Project_CTR>
[bannertool]: <https://github.com/Steveice10/buildtools>
[zip]: <http://downloads.sourceforge.net/gnuwin32/zip-3.0-bin.zip>
[libbz2.dll]: <http://downloads.sourceforge.net/gnuwin32/zip-3.0-dep.zip>
[FBI]: <https://github.com/Steveice10/FBI>
