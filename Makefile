all: shell id ls setfacl

shell: executable.o processmanager.o tokenizer.o errors.o internal.o utils.o
	gcc -pthread -g -o shell shell.c executable.o processmanager.o tokenizer.o errors.o internal.o utils.o


executable.o: bool.h tokenizer.h processmanager.h utils.h executable.h
	gcc -g -c executable.c

utils.o: bool.h errors.h
	gcc -g -c utils.c

processmanager.o: bool.h processmanager.h
	gcc -g -c processmanager.c

tokenizer.o: bool.h tokenizer.h
	gcc -g -c tokenizer.c

errors.o: bool.h errors.h
	gcc -c errors.c

internal.o: bool.h processmanager.h tokenizer.h
	gcc -g -c internal.c

ls: bool.h errors.h ./bsource/ls/ls.h
	gcc  -o ./bin/ls ./bsource/ls/ls.c errors.c -lacl
id: bool.h errors.h ./bsource/id/id.h
	gcc -o ./bin/id ./bsource/id/id.c errors.c

setfacl: bool.h errors.h  ./bsource/setfacl/setfacl.h ./bsource/setfacl/acl_entry.h
	gcc -g -o ./bin/setfacl ./bsource/setfacl/setfacl.c ./bsource/setfacl/acl_entry.c errors.c -lacl

clean:
	rm -rf shell *.o ./bin/*
