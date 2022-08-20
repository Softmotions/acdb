# Clang's C/C++ compile commands .json generator for Arduino projects

This small utility program generates `compile_commands.json` file in your Arduino sketch directory.
A `compile_commands.json` is used by `cland` language server give you a proper IDE code completion.

Now you can code Arduino projects in vim/neovim editor with [CoC](https://github.com/neoclide/coc.nvim) or neovim LSP server.

## Prerequisites

* The latest `arduino-cli` tool in your path
* CMake build tool
* gcc/clang compiler

## Installation

### CoC users 

Associate `.ino` files with coc-clangd language server in `init.vim`

```vim
let g:coc_filetype_map = {
  \ 'ino': 'cpp',
  \ }
```


```sh
git clone https://github.com/Softmotions/acdb

mkdir ./acdb/build && cd ./acdb/build

cmake .. -DCMAKE_INSTALL_PREFIX=~/.local

make install
```

## Usage

```sh

acdb [options] [sketch directory

	-a, --arduino-cli=<>	Path to the arduino-cli binary.
	-b, --fqbn=<>		Fully Qualified Board Name, e.g.: arduino:avr:uno
	-V, --verbose		Print verbose output.
	-v, --version		Show program version.
	-h, --help		  Print usage help.
```


```sh
cd ./MySketch
acdb --fqbn=arduino:avr:nano:cpu=atmega328old
````

or 

```sh
cat ./sketch.json 
{
  "cpu": {
    "fqbn": "arduino:avr:nano:cpu=atmega328old"
  }
}

acdb 
```
