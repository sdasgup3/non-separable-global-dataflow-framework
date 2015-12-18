PROG_PREFIX=test_3_1 test_3_2 test_3_3

CC1=/home/sdasgup3/Install/gdfa/gcc-4.3.0.obj/gcc/cc1
GDFA_OPTS=-fgdfa -fdump-tree-all
PROGS=$(PROG_PREFIX:%=%.c)

REMOVE_S=$(PROG_PREFIX:%=%.s)
REMOVE_C_STAR=$(PROG_PREFIX:%=%.c.*)
REMOVE=$(REMOVE_S) $(REMOVE_C_STAR)  *.mod

all:	compile

compile:$(PROGS)
	@for test in $(PROGS) ; do \
		$(CC1) $(GDFA_OPTS) $$test;\
	done	  

test:
	@for file in `cat fileList` ; do \
	  gold="gold_$$file" ; \
	  echo Diff $$gold  $$file  ; \
	  sed -e 's/D\.[0-9]*/D.GDFA/g' $$gold > $$gold.mod; \
	  sed -e 's/D\.[0-9]*/D.GDFA/g' $$file > $$file.mod; \
	  diff -q  $$gold.mod $$file.mod; \
	  echo;\
	done  

clean:
	rm -rf $(REMOVE)

.PHONY: clean run	
