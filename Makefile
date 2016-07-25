all: shell

shell: executable.h processmanager.h tokenizer.h bool.h
	gcc -o shell executable.c processmanager.c tokenizer.c shell.c -pthread 


debug: executable.h processmanager.h tokenizer.h bool.h
	gcc -g -o shell executable.c processmanager.c tokenizer.c shell.c -pthread

clean:
	rm -rf shell
