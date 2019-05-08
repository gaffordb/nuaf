CC=clang -g

all: test1 testbad test_simple test_small_objs test_complete test_worstcase

test1: test1.c
	${CC} test1.c -o ../test1
testbad: testbad.c
	${CC} testbad.c -o ../testbad
test_simple: test_simple.c
	${CC} test_simple.c -o ../test_simple
test_small_objs: test_small_objs.c
	${CC} test_small_objs.c -o ../test_small_objs
test_complete: test_complete.c
	${CC} test_complete.c -o ../test_complete
test_worstcase: test_worstcase.c
	${CC} test_worstcase.c -o ../test_worstcase
