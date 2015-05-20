# MPI-Checker
A static analysis checker for [MPI](https://en.wikipedia.org/wiki/Message_Passing_Interface) code
written in C using Clang's [Static Analyzer](http://clang-analyzer.llvm.org/).

<img src="https://github.com/0ax1/MPI-Checker/blob/master/screenshots/double_wait.jpg" width="500">

## Integrated checks
#### AST-Checks
- `unmatched point to point call`: Point to point schema validation.
- `unreachable call`: Unreachable calls caused by deadlocks.
- `type mismatch`: Buffer type and specified MPI type do not match.
- `invalid argument type`: Non integer values are used where only integer values are allowed.
- `redundant call`: Calls with identical arguments that can be summarized.
- `collective call in rank branch`: Collective calls inside a branch whose condition depends on a rank var.

#### Path-Sensitive-Checks 
- `double nonblocking`: Double request usage of nonblocking calls without intermediate wait. 
- `double wait` : Double wait on the same request without intermediate nonblocking call.
- `missing wait`: Nonblocking call without matching wait.
- `unmatched wait`: Waiting for a request that was never used by a nonblocking call.

## Prerequisites
Current versions of: `zsh`, `svn`, `git`, `cmake`, `ninja`, `sed` (install gnu-sed with brew if you're on osx)

## Installation
Download this [script] (https://raw.githubusercontent.com/0ax1/MPI-Checker/master/setup/llvmSetupFull.sh), make it executable with `chmod +x llvmSetupFull.sh` and run it to setup LLVM 3.6 with Clang and the MPI-Checker. This will download, config and build all components in `./llvm36`.

Add these locations to your search path:<br>
`.../llvm36/build/release/bin`<br>
`.../llvm36/repo/tools/clang/tools/scan-build`<br>
`.../llvm36/repo/tools/clang/tools/scan-view`<br>

## Example
Try the `basic` example from the examples folder. Have a look at the CMakeLists.txt file to see how it is set up.
To invoke the MPI-Checker add [this](https://github.com/0ax1/MPI-Checker/blob/master/setup/analyse.sh) to your `.zshrc`. Then run `checkMPI` in the projects `CMakeLists.txt` path.
