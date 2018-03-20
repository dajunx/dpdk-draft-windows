#include <sys/_termios.h>

int tcgetattr(int fd, struct termios *t)
{
	return 0;
}

int tcsetattr(int fd, int opt, struct termios *t)
{
	return 0;
}