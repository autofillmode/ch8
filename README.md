# A Chip-8 Interpreter

This is a barebones chip-8 interpreter, made using SDL2, based on [this](https://tobiasvl.github.io/blog/write-a-chip-8-emulator/) great guide. As of now I've implemented all 38 instructions, but the emulator lacks compatibility with old 
ROMs, since it's built with "modern" behaviour in mind, and also doesn't have sound. Writing this, even if it was a simple emulator, was quite a learning experience in both SDL and low-level stuff. 

## Usage
Compile with: `cc ch8.c -o ch8 -lSDL2`
Then run a rom: `./ch8 rom.ch8`
NOTE: This emulator maps the keyboard's left side as the keypad.
