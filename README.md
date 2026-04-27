[![C/C++ CI](https://github.com/ivanria/knx_sample/actions/workflows/c-build.yml/badge.svg)](https://github.com/ivanria/knx_sample/actions/workflows/c-build.yml)

# KNX Generic Telegram Parser

An educational C project designed to parse generic KNX telegrams. The application reads ascii data in hex base, processes the telegram structure, and extracts key information according to KNX standards.

## Features
- Classic **Makefile** based build system.
- Automated testing and memory leak detection via **Valgrind** in GitHub Actions.
- Strict compilation with `-Wall -Wextra -Wpedantic`.

## How to build
To compile the project, simply run:
```bash
make
```
For debug build with symbols for Valgrind:
```bash
make DEBUG=1
```

## Usage
The program reads data from a predefined file in the repository:
```bash
./knx_sample
```
