
./main $1 && llc -filetype=obj module.ll -o module.o && clang module.o -o module
