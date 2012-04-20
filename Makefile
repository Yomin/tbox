.PHONY: clean

tbox: tbox.c
	gcc -Wall -pedantic -o tbox tbox.c -lasound -lm -ggdb

clean:
	rm -rf tbox
