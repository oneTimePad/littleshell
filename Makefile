all: shell

shell: executable.o processmanager.o tokenizer.o errors.o internal.o
	gcc -pthread -o shell shell.c executable.o processmanager.o tokenizer.o errors.o internal.o


executable.o: bool.h tokenizer.h processmanager.h executable.h
	gcc -c executable.c

processmanager.o: bool.h processmanager.h
	gcc -c processmanager.c

tokenizer.o: bool.h tokenizer.h
	gcc -c tokenizer.c

errors.o: bool.h errors.h
	gcc -c errors.c

internal.o: bool.h processmanager.h tokenizer.h
	gcc -c internal.c

clean:
	rm -rf shell *.o
