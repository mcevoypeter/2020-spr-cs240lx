// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "attack.h"

#define error(args...) do { fprintf(stderr, ##args); exit(1);  } while(0)

// a not very interesting compile
static void compile(char *program, char *outname) {
    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);

    // not a very robust finger-print, but what can you do.
    // in a real compiler we would do this match on the parse tree,
    // which strips out some non-essential features.
    static char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    
    /* your code goes here */
    char *compile_start = strstr(program, compile_sig);
    if (compile_start) {
        fprintf(fp, "char prog[] = {\n");
        for (int i = 0; prog[i]; i++)
            fprintf(fp, "\t%d,\n", prog[i]);
        fprintf(fp, "0 };\n");

        char *compile_end = compile_start + strlen(compile_sig);
        unsigned prefix_len = compile_end - program;
        fprintf(fp, "%*.*s", prefix_len, prefix_len, program);
        fprintf(fp, "\n\t%s\n", prog);
        program = compile_end;
    }
    
    fprintf(fp, "%s", program);
    fclose(fp);

    // gross, call gcc.
    char buf[1024];
    sprintf(buf, "gcc ./temp-out.c -o %s", outname);
    if(system(buf) != 0)
        error("system failed\n");
}

#   define N  8 * 1024 * 1024
static char buf[N+1];

int main(int argc, char *argv[]) {
    if(argc != 4)
        error("expected 4 arguments have %d\n", argc);
    if(strcmp(argv[2], "-o") != 0)
        error("expected -o as second argument, have <%s>\n", argv[2]);

    // read in the entire file.
    int fd;
    if((fd = open(argv[1], O_RDONLY)) < 0)
        error("file <%s> does not exist\n", argv[1]);

    int n;
    if((n = read(fd, buf, N)) < 1)
        error("invalid read of file <%s>\n", argv[1]);
    if(n == N)
        error("input file too large\n");

    // "compile" it.
    compile(buf, argv[3]);
    return 0;
}
