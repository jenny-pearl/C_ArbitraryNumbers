AP: AP.c
	clang AP.c -o AP -Wall -Wextra -Wno-sign-compare -fsanitize=address -g -std=c11 -lm
