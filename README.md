# MPI-Checker

A static analysis checker to verify the correct usage of the
[MPI](https://en.wikipedia.org/wiki/Message_Passing_Interface)
API in C and C++ code, based on Clang’s
[Static Analyzer](http://clang-analyzer.llvm.org/).

<img src="https://github.com/0ax1/MPI-Checker/blob/master/screenshots/missingwait.jpg" height="450">

<br>MPI-Checker is currently in the process of [being merged into Clang](http://reviews.llvm.org/D12761).

## Integrated checks
#### Path-Sensitive-Checks
- `double nonblocking`: Double request usage of nonblocking calls without intermediate wait.
- `missing wait`: Nonblocking call without matching wait.
- `unmatched wait`: Waiting for a request that was never used by a nonblocking call.

#### AST-Checks
- `type mismatch`: Buffer type and specified MPI type do not match.
- `incorrect buffer referencing`: B-type is incorrectly referenced when passed to an MPI function.
- `invalid argument type`: Non integer type used where only integer types are allowed.

## Limitations
- No assumption about run time dependent results can be made.
- Waitany or wildcards like `MPI_ANY_SOURCE` cannot be taken into account..
- Heap allocated or global `MPI_Request` variables are not taken into account.
- Analysis is limited to the scope of a translation unit.

A general limitation of static analysis to keep in mind is that basically no
assumption about run time dependent results can be made. In consequence,
functions like `MPI_Waitany` or wildcards like `MPI_ANY_SOURCE` cannot be taken
into account. Further, the reasoning about heap allocated memory is limited
which is why heap allocated requests are not considered. The problem is that the
element count held by the memory region must be known, in order to reason about
the single requests. Requests that are not heap allocated themselves but a
member of a heap allocated struct or class instance are therefore taken into
account. Requests used as global variables are not taken into account. Using the
Clang Static Analyzer, the analysis is limited to the scope of a translation
unit. Analyzing logic across translation units to detect bugs is therefore not
possible. This includes that there is limited knowledge about memory regions
returned from functions which are defined in a different translation unit than
they are used in. Regarding the current range of checks integrated in
MPI-Checker, those memory regions cannot be taken into account.  Definitions in
header files are conceptually inlined and therefore considered during analysis
in the translation units that include the headers.
