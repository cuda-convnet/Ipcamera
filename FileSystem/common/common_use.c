#include "../include/cm_common.h"

#define DEBUG

void trace(char * error)
{
	#ifdef DEBUG
	printf(error);
	#endif
}
