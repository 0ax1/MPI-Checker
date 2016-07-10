# MPI-Checker

Verify the correct usage of the
[MPI](https://en.wikipedia.org/wiki/Message_Passing_Interface) API in C and C++
code, with checks based on [Clang’s Static
Analyzer](http://clang-analyzer.llvm.org/) and
[Clang-Tidy](http://clang.llvm.org/extra/clang-tidy/).

<img src="https://github.com/0ax1/MPI-Checker/blob/master/screenshots/missingwait.jpg" height="450">

<br>

## Integrated checks
#### Path-Sensitive-Checks
- `double nonblocking`: Double request usage of nonblocking calls without intermediate wait.
- `missing wait`: Nonblocking call without matching wait.
- `unmatched wait`: Waiting for a request that was never used by a nonblocking call.

The path-sensitive checks have been merged into Clang's Static Analyzer and are
available since LLVM 3.9.

#### AST-Checks
- `type mismatch`: Buffer type and specified MPI type do not match.
- `incorrect buffer referencing`: Buffer is incorrectly referenced when passed to an MPI function.

The AST-based checks have been merged into Clang-Tidy and are available in
the LLVM trunk.

## Limitations
- No assumption about run time dependent results can be made.
  <br>-> (`MPI_Waitany` is not taken into account.)
- Heap allocated `MPI_Request` variables are not taken into account.
- Analysis is limited to the scope of a translation unit.

A general limitation of static analysis to keep in mind is that basically no
assumption about run time dependent results can be made. In consequence,
functions like `MPI_Waitany` cannot be taken into account. Further, the
reasoning about heap allocated memory is limited which is why heap allocated
requests are not considered. The problem is that the element count held by the
memory region must be known, in order to reason about the single requests.
Requests that are not heap allocated themselves but a member of a heap allocated
struct or class instance can be taken into account, as their array size can be
statically determined. Using the Clang Static Analyzer, the analysis is limited
to the scope of a translation unit.  Analyzing logic across translation units to
detect bugs is therefore not possible. This includes that there is limited
knowledge about memory regions returned from functions which are defined in a
different translation unit than they are used in. Regarding the current range of
checks integrated in MPI-Checker, those memory regions cannot be taken into
account.  Definitions in header files are conceptually inlined and therefore
considered during analysis in the translation units that include the headers.

## Usage
[How to apply the checks](https://github.com/0ax1/MPI-Checker/tree/master/examples)

## Paper
[MPI-Checker – Static Analysis for MPI](https://dl.acm.org/citation.cfm?id=2833157.2833159&coll=DL&dl=GUIDE&CFID=562722438&CFTOKEN=16030439)

## Slides
[The Second Workshop on the LLVM Compiler Infrastructure in HPC](https://llvm-hpc2-workshop.github.io/)
