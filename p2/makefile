exec:
	gcc -Wall -g -o p2 p2.c list.c
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --log-file=valgrind-report.txt ./p2
