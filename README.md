# Wind Waker Synth

This is a highly experimental tool for decoding and playing the sequenced music files from the Wind Waker.
The major difficulty with this project is that the sound system used by this game (and a few other GameCube titles)
is very complex. Rather than using MIDI or a similar format, the engine runs the sequence files through a
fully custom 16-bit processor architecture complete with registers, conditional branches, and interrupts.
These features allow the music to change in real time in response to in-game events. While this program doesn't
currently support any of these special features (yet), the data still has to be interpreted as bytecode.

The lim(t-\>infinity) goal of this project is to fully support several formats:
* JAudio version 1 (Wind Waker and other first-party GC games)
* JAudio version 2 (Twilight Princess, Super Mario Galaxy 1 and 2, other first-party Wii titles)
* NW4R RSEQ/RBNK/RWAV/RWSD formats (Mario Kart Wii and many other first- and 3rd-party Wii titles)

Currently only JAudio v1 is supported; the codebase will need to be rewritten to be more generic before more
formats can be added.

## Building and Running

This project has only ever been compiled/tested on my own system, so it may not compile at all.
However, if you would like to try, here are some instructions:

* Dependencies:
  * SDL2
  * [STK](https://github.com/thestk/stk)
* This project uses CMake to build, use
  ```
  mkdir build
  cd build
  cmake ..
  ```
  to set up the project.

### Data files required

Data files should be placed in the `data` directory, which should be placed directly inside
this project (alongside `build`). For Wind Waker, the following disc files are required:
* `Audiores/Banks/` --> `data/Banks/` 
* `Audiores/JaiInit.aaf` --> `data/JaiInit.aaf`
* `Audiores/Seqs/JaiSeqs.arc` should be extracted manually and can be placed anywhere
  * There are many tools available to extract ARC archives; I recommend using Wiimm's SZS Tools [https://szs.wiimm.de/wszst/](https://szs.wiimm.de/wszst/)

### Running the program

This project generates three executables:
* `synth` plays the sequence file and exports the result into a WAV file (`{inputFile}.wav`). Playing stops after the music loops 2 times.
* `player` plays the sequence file directly to the user's audio output. If the sequence is looped, it will play indefinitely until cancelled.
* `disassembler` dumps a full disassembly of the input sequence file. However, it will stop disassembling the file as soon as an unknown opcode is detected.

## License

This project is MIT licensed. See the `LICENSE` file for more details.

