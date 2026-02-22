AP: AP.c
	clang AP.c -o AP -Wall -Wextra -g -fsanitize=address -std=c11
