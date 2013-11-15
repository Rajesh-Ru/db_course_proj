/*Rajesh
  5/1/2012 */

#include "Buffer.h"


extern int GLOBAL_DEPTH;
struct page buffer[BUFFER_NUM];
struct index_page index_buffer[INDEX_BUFFER_NUM];
bool used[BUFFER_NUM];
bool ibuff_used[INDEX_BUFFER_NUM];

extern int power (int base, int exponent);

void buffer_init (void)
{
	int bound = power (2, GLOBAL_DEPTH);
	for (int i = 0; i < BUFFER_NUM; i++)
	{
		if (i < bound)
		{
			init_page (&buffer[i], GLOBAL_DEPTH);
			used[i] = true;
		}
		else
			used[i] = false;
	}
	for (int i = 0; i < INDEX_BUFFER_NUM; i++)
		ibuff_used[i] = false;
}