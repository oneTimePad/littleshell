all: shell id ls setfacl getfacl setfattr getfattr

shell: executable.o processmanager.o tokenizer.o errors.o internal.o init.o path.o
	gcc -pthread -g -o shell shell.c executable.o processmanager.o tokenizer.o errors.o internal.o path.o init.o


executable.o: bool.h tokenizer.h processmanager.h  executable.h 
	gcc -g -c executable.c

processmanager.o: bool.h path.h tokenizer.h processmanager.h internal.h errors.h
	gcc -g -c processmanager.c

tokenizer.o: bool.h tokenizer.h
	gcc -g -c tokenizer.c

errors.o: bool.h errors.h processmanager.h internal.h
	gcc -g -c errors.c

internal.o: bool.h processmanager.h tokenizer.h
	gcc -g -c internal.c

path.o: path.h bool.h
	gcc -g -c path.c

init.o: processmanager.h init.h
	gcc -g -c init.c

ls: bool.h errors.h ./bsource/ls/ls.h
	gcc  -o ./bin/ls ./bsource/ls/ls.c errors.c -lacl
id: bool.h errors.h ./bsource/id/id.h
	gcc -o ./bin/id ./bsource/id/id.c errors.c

setfacl: bool.h errors.h  ./bsource/setfacl/setfacl.h ./bsource/setfacl/acl_entry.h ./bsource/setfacl/acl_ext_fct.h
	gcc -g -o ./bin/setfacl ./bsource/setfacl/setfacl.c ./bsource/setfacl/acl_entry.c ./bsource/setfacl/acl_ext_fct.c errors.c -lacl

getfacl: bool.h errors.h ./bsource/getfacl/getfacl.h
	gcc -g -o ./bin/getfacl ./bsource/getfacl/getfacl.c errors.c -lacl

setfattr: bool.h errors.h ./bsource/setfattr/setfattr.h
	gcc -g -o ./bin/setfattr ./bsource/setfattr/setfattr.c errors.c

getfattr: bool.h errors.h ./bsource/getfattr/getfattr.h
	gcc -g -o ./bin/getfattr ./bsource/getfattr/getfattr.c errors.c

clean:
	rm -rf shell *.o ./bin/*
