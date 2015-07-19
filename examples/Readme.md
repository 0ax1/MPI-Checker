### Examples

Remember that only built source files are analyzed. To do a complete analysis
you have to do a complete build. Clean your analysis target to ensure this. All
example builds work with a flag `ANALYZE=x` passed to trigger the analysis.
If there are any bugs found your browser will automatically open showing an
overview of the detected erros. Those can be clicked to inspect them in detail.

#### CMake
Inspect one of the `CMakeLists.txt` files to see how it is set up for MPI and
static analysis.  To invoke the MPI-Checker add [this]
(https://github.com/0ax1/MPI-Checker/blob/master/setup/analyze.sh) to your
`.zshrc`. Then run `checkMPI` in the projects `CMakeLists.txt` path.

#### Make
If you use Make you can do ```scan-build --use-analyzer `which clang`
-enable-checker lx.MPIChecker -V make debug ANALYZE=1```.
