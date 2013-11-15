/* Rajesh
   5/1/2012 */

#include "HashFunction.h"
#include "stdlib.h"
#include "Page.h"


extern int GLOBAL_DEPTH;

int hash_func (const char *key)
{
	int denom = 1;
	for (int i = 0; i < GLOBAL_DEPTH; i++)
		denom *= 2;
	return atoi (key) % denom;
}