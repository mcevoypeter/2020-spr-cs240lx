// convert the contents of stdin to their ASCII values (e.g., 
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>
#include <unistd.h>

int main(void) { 
    // put your code here.
    printf("char prog[] = {\n");
    char ch;
    while (read(STDIN_FILENO, &ch, 1) != 0) {
        printf("\t%d,\n", ch);
    }
    printf("0 };\n");
	return 0;
}
