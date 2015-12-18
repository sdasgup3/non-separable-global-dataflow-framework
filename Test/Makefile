
.PHONY: all
all: compile

compile:
	make -C Test_1 all
	make -C Test_2 all
	make -C Test_3 all

test:
	make -C Test_1 test
	make -C Test_2 test
	make -C Test_3 test

.PHONY: clean
clean:
	@make -C Test_1 clean
	@make -C Test_2 clean
	@make -C Test_3 clean

printvars::
	@echo "CC1    		: " '$(CC1)'
@	@echo "GDFA_OPTS    	: " '$(GDFA_OPTS)'
