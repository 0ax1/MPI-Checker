# MPI-Checker
A static analysis checker for [MPI](https://en.wikipedia.org/wiki/Message_Passing_Interface) code
written in C using Clang's [Static Analyzer](http://clang-analyzer.llvm.org/).

<img src="https://github.com/0ax1/MPI-Checker/blob/master/screenshots/double_wait.jpg" width="500">

## Prerequisites
- zsh
- svn
- git
- cmake
- ninja
- sed (install gnu-sed with homebrew if you're on osx)

## Installation
Download and run this [script] (https://raw.githubusercontent.com/0ax1/MPI-Checker/master/setup/llvmSetupFull.sh) to setup LLVM 3.6 with Clang and the MPI-Checker.<br>
This will download, config and build all components in ./llvm36.

Add these locations to your path:<br>
`.../llvm36/build/release/bin`<br>
`.../llvm36/repo/tools/clang/tools/scan-build`<br>
`.../llvm36/repo/tools/clang/tools/scan-view`<br>

## Example
Try the `basic` example from the examples folder. Have a look at the CMakeLists.txt file to see how it is set up.
To invoke the MPI-Checker add [this](https://github.com/0ax1/MPI-Checker/blob/master/setup/analyse.sh) function and alias to your .zsh|bashrc. Then run `checkMPI` in your `CMakeLists.txt` path.
