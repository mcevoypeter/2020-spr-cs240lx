# you should not need to modify this.
CC =gcc
CFLAGS = -Og -g -Wall -Wno-unused-variable

all: clone

clone:
	../step1/string-to-char-array < attack.c > attack.h
	cat attack.h >> replicate.c
	cat trojan-cc2.c >> replicate.c
	$(CC) $(CFLAGS) replicate.c -o trojan-cc2
	./trojan-cc2 ../step2/identity-cc.c -o cc-attacked
	mv temp-out.c 1-temp-out.c
	./cc-attacked ../step2/identity-cc.c -o cc-attacked2
	mv temp-out.c 2-temp-out.c

clean: 
	rm -f trojan-cc2 replicate.c 1-temp-out.c 2-temp-out.c
	rm -rf *.dSYM
