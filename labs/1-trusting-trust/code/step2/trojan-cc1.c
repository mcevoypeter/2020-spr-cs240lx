// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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
    // inject this after the assert.  not much of an attack.   this is just a quick placeholder.
    static char compile_attack[] = "printf(\"%s:%d: could have run your attack here!!\\n\", __FUNCTION__, __LINE__);";

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    /* your code goes here */
    char *hacked_prog = 0;
    unsigned hacked_prog_len = strlen(program) + 1;

    char *login_start = strstr(program, login_sig);
    if (login_start) {
        hacked_prog_len += strlen(login_attack);
        hacked_prog = calloc(hacked_prog_len, sizeof(char));

        // insert prefix
        char *login_end = login_start + strlen(login_sig);
        strncat(hacked_prog, program, login_end - program);

        // insert attack
        strcat(hacked_prog, login_attack); 

        // insert suffix
        strcat(hacked_prog, login_end);
        
        program = hacked_prog;
    }

    char *compile_start = strstr(program, compile_sig);
    if (compile_start) {
        hacked_prog_len += strlen(compile_attack);
        hacked_prog = realloc(hacked_prog, hacked_prog_len);

        // insert prefix
        char *compile_end = compile_start + strlen(compile_sig);
        strncat(hacked_prog, program, compile_end - program);

        // insert attack
        strcat(hacked_prog, compile_attack);

        // insert suffix
        strcat(hacked_prog, compile_end);

        program = hacked_prog;
    }
    
    fprintf(fp, "%s", program);
    fclose(fp);

    if (hacked_prog)
        free(hacked_prog);

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
