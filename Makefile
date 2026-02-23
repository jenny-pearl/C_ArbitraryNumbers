AP: AP.c
	clang AP.c -o AP -Wall -Wextra -fsanitize=address -g -std=c11 -lm
