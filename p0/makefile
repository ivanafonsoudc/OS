exec:
	gcc -Wall -g -o p0 p0.c list.c
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-report.txt ./p0
