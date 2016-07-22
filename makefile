all: shell

shell: executable.h processmanager.h tokenizer.h
	gcc -o shell  shell.c executable.c processmanager.c  tokenizer.c -lrt -pthread

debug: executable.h processmanager.h tokenizer.h
	gcc -g -o shell  shell.c executable.c processmanager.c  tokenizer.c -lrt -pthread

clean:
	rm -rf shell 
