#! /bin/csh -f

foreach file (`cat fileList`)
diff $file gold_$file > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
end

diff test_2_1.c.*.cplxlower0 	gold_test_2_1.c.*.cplxlower0 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_2_1.c.*.veclower 		gold_test_2_1.c.*.veclower   > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_2_1.c.*.cleanup_cfg1 	gold_test_2_1.c.*.cleanup_cfg1 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_2_1.c.*.apply_inline 	gold_test_2_1.c.*.apply_inline > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
