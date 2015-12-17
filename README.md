##DIRECTORY STRUCTURE

* Reports
  * Report.pdf                
  * Presentation.pdf
* Documents
  * listOfFilesModified.txt 
  * CodeDiff.txt 
* Test
* Source : The gcc 4.3.0 source code base patched with gdfa where we have done the modifications

##HOW TO BUILD

The following steps need to be followed:
        mkdir gcc_gdfa_build             /*This is created parrallel to the source directory gcc-4.3.0*/
        setenv GDFA_HOME <absolute path>/gcc_gdfa_bulid
        cd $GDFA_HOME
        ../gcc-4.3.0/configure --prefix=/home/dsand/Compiler/GDFA/gcc_gdfa_install  --enable-languages=c --with-gmp=/usr/local --with-mpfr=/usr/local 
        make all-gcc TARGET-gcc=cc1 BOOT_CFLAGS='-O0 -g3' -j 2

                                        ** THE EXECUTABLE FILE : $GDFA_HOME/gcc/cc1 **


                                            C.HOW TO TEST THE CORRECTNESS OF BEHAVIOUR
                                            ==========================================

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

