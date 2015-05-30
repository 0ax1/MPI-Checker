# MPI-Checker
A static analysis checker for [MPI](https://en.wikipedia.org/wiki/Message_Passing_Interface) code
written in C using Clang's [Static Analyzer](http://clang-analyzer.llvm.org/).

<img src="https://github.com/0ax1/MPI-Checker/blob/master/screenshots/double_wait.jpg" width="500">

## Integrated checks
#### AST-Checks
- `unmatched point to point call`: Point to point schema validation.
- `unreachable call`: Unreachable calls caused by deadlocks from blocking calls.
- `type mismatch`: Buffer type and specified MPI type do not match.
- `invalid argument type`: Non integer type used where only integer types are allowed.
- `collective call in rank branch`: Collective call inside a rank branch.

#### Path-Sensitive-Checks
- `double nonblocking`: Double request usage of nonblocking calls without intermediate wait.
- `double wait` : Double wait on the same request without intermediate nonblocking call.
- `missing wait`: Nonblocking call without matching wait.
- `unmatched wait`: Waiting for a request that was never used by a nonblocking call.

All of these checks should produce zero false positives. Bug reports are only emitted if the checker
is sure that an invariant was violated.

###Point to Point Schema Validation
If only additions for an argument are used operands are accepted as a match if they appear as a permutation.
Else if subtractions are used operands have to be in the same order. 
To match point to point calls the checker has to make some assumptions how 
arguments are specified. To match variables across cases and functions the same variable name has to be used.
For the rank argument the checker expects the last operator to be "inverse"
and the last operand to be equal.
If none except the last operator are additions also rank operands (excluding the last) are matched
if they appear as a permutation:
<br>`MPI_Isend(&buf, 1, MPI_INT, f() + N + 3 + rank + 1, 0, MPI_COMM_WORLD, &sendReq1);`<br>
`MPI_Irecv(&buf, 1, MPI_INT, N + f() + 3 + rank - 1, 0, MPI_COMM_WORLD, &recvReq1);`<br>


## Prerequisites
Current versions of: `zsh`, `svn`, `git`, `cmake`, `ninja`, `sed` (install
gnu-sed with brew if you're on osx)

## Installation
Download [`fullSetup.sh`]
(https://raw.githubusercontent.com/0ax1/MPI-Checker/master/setup/fullSetup.sh),
make it executable with `chmod +x` and run it to setup LLVM 3.6 with Clang and
the MPI-Checker.  This will download, config and build all components in
`./llvm36`. <br>In "one line": `wget
https://raw.githubusercontent.com/0ax1/MPI-Checker/master/setup/fullSetup.sh &&
chmod +x fullSetup.sh && ./fullSetup.sh`

If you have the LLVM 3.6 source already, `cd` to the top of the repository,
execute [`addMPI-Checker.sh`]
(https://raw.githubusercontent.com/0ax1/MPI-Checker/master/setup/addMPI-Checker.sh)
and then rerun your build system manually.

After that add these locations to your search path:<br>
`.../llvm36/build/release/bin`<br>
`.../llvm36/repo/tools/clang/tools/scan-build`<br>
`.../llvm36/repo/tools/clang/tools/scan-view`<br>

## Examples
Have a look at the `examples` folder. Inspect one of the `CMakeLists.txt` files
to see how it is set up for MPI and static analysis. To invoke the MPI-Checker
add [this] (https://github.com/0ax1/MPI-Checker/blob/master/setup/analyze.sh) to
your `.zshrc`. Then run `checkMPI` in the projects `CMakeLists.txt` path.  If
there are any bugs found your browser will automatically open showing an
overview of the detected erros. Those can be clicked to inspect them in detail.

## Tests
See [tests folder](https://github.com/0ax1/MPI-Checker/tree/master/tests).
