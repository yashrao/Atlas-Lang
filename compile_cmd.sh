#zig c++ main.cpp -o atlas && ./atlas $1 && gcc -nostdlib -s out.c -o out
#g++ main.cpp -o atlas && ./atlas $1 && gcc -nostdlib -s out.c -o out
#zig c++ main.cpp -o atlas && ./atlas $1 && zig cc -nostdlib -s out.c -o out
zig c++ src/*.cpp -g -o atlas && ./atlas --debug -I . $@ -r
