/* $Id$ */

#include "jack.h"

int main(int argc, char **argv)
{
	Jack *j = new Jack();

	j->Connect();
	j->Run();

	delete j;

	return 0;
}
