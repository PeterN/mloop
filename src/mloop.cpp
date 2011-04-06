/* $Id$ */

#include <unistd.h>
#include "jack.h"
#include "ui.h"

int main(int argc, char **argv)
{
	Jack j;

	if (!j.Connect()) return 1;

	UI u;

	while (true) {
		if (j.Run()) break;
		if (u.Run(j)) break;
		usleep(10000);
	}

	j.Disconnect();

	return 0;
}
