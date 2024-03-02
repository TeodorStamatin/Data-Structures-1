CC=gcc
CFLAGS=-I -Wall -Wextra -std=c99

build:
	gcc vma.c -c -g
	gcc vma.o main.c -o vma

run_vma:
	./vma

clean:
	rm -f vma.o

pack:
	zip -FSr 315CA_StamatinTeodor_Tema1SD.zip README Makefile *.c *.h