##DIRECTORY STRUCTURE

* Reports
  * Report.pdf                
  * Presentation.pdf
* Documents
  * gdfa-v1.2-gcc-4.3.0.patch : _gdfa patch for gcc 4.3.0_ 
* Test
  * Test Directories containing C code to be analysed and golden outputs for comparison.
* SourceCodeInfo : 
  * listOfFilesModified.txt : _List of files Modified_ 
  * CodeDiff.txt : _This file can be patched to gcc 4.3.0 source code_ 
  * Source Files
    * gimple-pfbvdfa-driver.c
    * gimple-pfbvdfa-specs.c
    * gimple-pfbvdfa-support.c
    * gimple-pfbvdfa.h
    * passes.c
    * tree-pass.h

##HOW TO BUILD
```
mkdir gdfa
cd gdfa
Download gcc-4.3.0-tar.gz from https://gcc.gnu.org/mirrors.html  in here.
Copy gdfa-v1.2-gcc-4.3.0.patch from Documents in here. This path can be downloaded from http://www.cse.iitb.ac.in/grc/index.php?page=gdfa.
tar -xvf gcc-4.3.0-tar.gz
mkdir gcc-4.3.0.obj gcc-4.3.0.install
patch -p0   < gdfa-v1.2-gcc-4.3.0.patch
cd gcc-4.3.0.obj
../gcc-4.3.0/configure --enable-languages=c --prefix=<path to gcc-4.3.0.install>
make all-gcc TARGET-gcc=cc1 BOOT_CFLAGS='-O0 -g3' -j 2
make install
```

##HOW TO RUN
The following commands generate test_1.c.021t.gdfa_fv (faint variable analysis) and 
test_1.c.021t.gdfa_puv (possibly uninitialized variable analysis)

```
cd Test/test_1/
cc1 -fgdfa -fdump-tree-all test_1.c 
```

##TESTING AUTOMATION

```
_Run and test the testsuite_
cd Test
make       _Compile all the C source files and dumps the analysis results in respective directories_
make test  _Compare the analysis dumps with the golden output provided_
make clean _Remove the anaylsis dumps_

OR
_Run and test unit cases_
cd Test_3
make 
make test
make clean

```
