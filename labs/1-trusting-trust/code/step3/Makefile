CC =gcc
CFLAGS = -Og -g -Wall -Wno-unused-variable

all: clone test

clone:
	../step1/string-to-char-array < attack.c > attack.h
	cat attack.h >> replicate.c
	rm attack.h
	cat trojan-cc2.c >> replicate.c
	$(CC) $(CFLAGS) replicate.c -o trojan-cc2
	rm replicate.c
	./trojan-cc2 ../step2/identity-cc.c -o cc-attacked
	rm temp-out.c
	./cc-attacked ../step2/identity-cc.c -o cc-attacked2
	rm temp-out.c 
	./cc-attacked2 ../step2/login.c -o login-attacked
	rm temp-out.c

test:
	diff cc-attacked cc-attacked2
	echo "ken" | ./login-attacked

clean: 
	rm -f trojan-cc2 cc-attacked cc-attacked2 login-attacked
	rm -rf *.dSYM
