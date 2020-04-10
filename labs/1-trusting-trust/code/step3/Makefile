CC =gcc
CFLAGS = -Og -g -Wall -Wno-unused-variable

all: string-to-char-array clone test

clone:
	./string-to-char-array < attack.c > attack.h
	$(CC) $(CFLAGS) trojan-cc2.c -o trojan-cc2
	./trojan-cc2 ../step2/identity-cc.c -o cc-attacked
	rm temp-out.c
	./cc-attacked ../step2/identity-cc.c -o cc-attacked2
	rm temp-out.c 
	./cc-attacked2 ../step2/login.c -o login-attacked
	rm temp-out.c

string-to-char-array: string-to-char-array.c
	$(CC) $(CFLAGS) string-to-char-array.c -o string-to-char-array

test:
	diff cc-attacked cc-attacked2
	echo "ken" | ./login-attacked

clean: 
	rm -f attack.h cc-attacked cc-attacked2 login-attacked string-to-char-array trojan-cc2 
	rm -rf *.dSYM
