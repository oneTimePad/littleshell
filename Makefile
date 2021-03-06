all: shell id ls setfacl getfacl setfattr getfattr 



jobmanager.o : bool.h path.h tokenizer.h internal.h errors.h sensitive.h embryos.h jobmanager.h
	gcc -g -c jobmanager.c

embryos.o: internal.h errors.h sensitive.h embryos.h
	gcc -g -c embryos.c

errors.o: bool.h errors.h
	gcc -g -c errors.c

internal.o: bool.h jobmanager.h tokenizer.h internal.h
	gcc -g -c internal.c

path.o: bool.h path.h
	gcc -g -c path.c

sensitive.o: bool.h embryos.h sensitive.h
	gcc -g -c sensitive.c

tokenizer.o: bool.h tokenizer.h
	gcc -g -c tokenizer.c

init.o: jobmanager.h init.h
	gcc -g -c init.c

execute.o: bool.h tokenizer.h embryos.h jobmanager.h
	gcc -g -c execute.c

shell.o: bool.h path.h errors.h init.h internal.h tokenizer.h jobmanager.h embryos.h execute.h shell.h
	gcc -g -c shell.c


shell: shell.o execute.o init.o tokenizer.o path.o internal.o errors.o embryos.o jobmanager.o sensitive.o
	gcc -g -o shell shell.o execute.o init.o tokenizer.o path.o internal.o errors.o embryos.o jobmanager.o sensitive.o


shell_e: shell.h execute.h init.h tokenizer.h path.h internal.h errors.h embryos.h jobmanager.h sensitive.h
	gcc -g -o shel_e shell.c execute.c init.c tokenizer.c path.c internal.c errors.c embryos.c jobmanager.c sensitive.c


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
