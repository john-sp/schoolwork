run: build
	./wsh
build: wsh

buildForce:
	gcc wsh.c -O0 -o wsh -Wall -Werror -g

test:
	~cs537-1/tests/P3/test-cd.csh -c
	~cs537-1/tests/P3/test-exec.csh -c
	~cs537-1/tests/P3/test-variables.csh -c
	~cs537-1/tests/P3/test-history.csh -c
	~cs537-1/tests/P3/test-pipe.csh -c
	~cs537-1/tests/P3/test-batch.csh -c

wsh: wsh.c
	gcc wsh.c -O3 -o wsh -Wall -Werror
clean:
	rm wsh