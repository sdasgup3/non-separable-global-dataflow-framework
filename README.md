##DIRECTORY STRUCTURE

* Reports
  * Report.pdf                
  * Presentation.pdf
* Documents
  * gdfa-v1.2-gcc-4.3.0.patch : _gdfa patch for gcc 4.3.0_ 
* Test
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

* mkdir gdfa
* cd gdfa
* Download gcc-4.3.0-tar.gz from https://gcc.gnu.org/mirrors.html  in here.
* Copy gdfa-v1.2-gcc-4.3.0.patch from Documents in here. This path can be downloaded from http://www.cse.iitb.ac.in/grc/index.php?page=gdfa.
* tar -xvf gcc-4.3.0-tar.gz
* mkdir gcc-4.3.0.obj gcc-4.3.0.install
* patch -p0   < gdfa-v1.2-gcc-4.3.0.patch
* cd gcc-4.3.0.obj
* ../gcc-4.3.0/configure --enable-languages=c --prefix=<path to gcc-4.3.0.install>
* make all-gcc TARGET-gcc=cc1 BOOT_CFLAGS='-O0 -g3' -j 2
* make install

##TESTING

The Test directory contains the following testcases:

        Test_1     : Contain the case test_1.c and the expected output of all the dump files as  gold_test_1.c.*

                   To run testing,
                                 $GDFA_HOME/gcc/cc1 -fgdfa -fdump-tree-all test_1.c
                                                                 /*   This will generate many dump files as test_1.c.*                         */
                                                                 /*To verify if the generated output is same as expected do the following:     */
                                ./compute_diff.sh                /*If no message comes diff is ok  */
                                ./remove.sh                      /*Remove the redundant files.     */


        Test_2 : Contain the case test_2.c and the expected output of all the dump files as  gold_test_2.c.*

                  To run testing,
                                  $GDFA_HOME/gcc/cc1 -fgdfa -fdump-tree-all test_2.c
                                                                 /*   This will generate many dump files as test_2.c.*                           */
                                                                 /*   To verify if the generated output is same as expected do the following:    */
                                  ./compute_diff.sh              /*   If no message comes diff is ok  */
                                  ./remove.sh                    /*   Remove the redundant files.     */


        Test_3 : Contain the case test_3_1.c, test_3_2.c, test_3_3.c and the expected output of all the dump files as  gold_test_3_*.c.*

                   To run testing,
                                  $GDFA_HOME/gcc/cc1 -fgdfa -fdump-tree-all test_3_1.c
                                  $GDFA_HOME/gcc/cc1 -fgdfa -fdump-tree-all test_3_2.c
                                  $GDFA_HOME/gcc/cc1 -fgdfa -fdump-tree-all test_3_3.c
                                                               /*   This will generate many dump files as test_3_*.c.*                             */
                                                               /*   To verify if the generated output is same as expected do the following:        */
                                ./compute_diff.sh              /*   If no message comes diff is ok    */
                                ./remove.sh                    /*   Remove the redundant files.        */

