#! /bin/csh -f

foreach file (`cat fileList`)
diff $file gold_$file > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
end

diff test_3_1.c.*.cplxlower0 	gold_test_3_1.c.*.cplxlower0 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_1.c.*.veclower 		gold_test_3_1.c.*.veclower   > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_1.c.*.cleanup_cfg1 	gold_test_3_1.c.*.cleanup_cfg1 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_1.c.*.apply_inline 	gold_test_3_1.c.*.apply_inline > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif

diff test_3_2.c.*.cplxlower0 	gold_test_3_2.c.*.cplxlower0 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_2.c.*.veclower 		gold_test_3_2.c.*.veclower   > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_2.c.*.cleanup_cfg1 	gold_test_3_2.c.*.cleanup_cfg1 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_2.c.*.apply_inline 	gold_test_3_2.c.*.apply_inline > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif

diff test_3_3.c.*.cplxlower0 	gold_test_3_3.c.*.cplxlower0 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_3.c.*.veclower 		gold_test_3_3.c.*.veclower   > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_3.c.*.cleanup_cfg1 	gold_test_3_3.c.*.cleanup_cfg1 > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
diff test_3_3.c.*.apply_inline 	gold_test_3_3.c.*.apply_inline > /dev/null
	if($status == 0) then
		#echo "$file passed"
	else
		echo "$file failed"
	endif
