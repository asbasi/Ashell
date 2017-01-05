
ashell: main.o history.o token.o command.o
	gcc -Wall main.o history.o token.o command.o -o ashell

main.o: main.c
	gcc -Wall -c main.c

history.o: history.c history.h
	gcc -Wall -c history.c

token.o: token.c token.h
	gcc -Wall -c token.c

command.o: command.c command.h
	gcc -Wall -c command.c

clean:
	rm -rf *.o ashell
