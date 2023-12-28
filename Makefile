export
CC=g++
LD=g++
INCLUDE=-I $(shell pwd)/include -I $(shell pwd)/src/include
CF=-O2 --std=c++17
CFLAG=${CF} ${INCLUDE}

.PHONY: all clean
all:
	${MAKE} -C lib all
	${MAKE} -C src all
	$(shell chmod +x ./*.out)
	@echo -e '\n'Build Finished OK

clean:
	${MAKE} -C lib clean
	${MAKE} -C src clean
	$(shell rm -rf ./*.out)
	@echo -e '\n'Clean Finished
