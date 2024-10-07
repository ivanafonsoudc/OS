exec:
	gcc -Wall -g -o p1 p1.c list.c
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-report.txt ./p1
