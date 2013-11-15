/* Rajesh
   5/1/2012*/

#include "Page.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>


int TOTAL_PAGE;		/* The number of all pages. */
int TOTAL_INDEX_PAGE;	/* The number of all index pages. */
int GLOBAL_DEPTH;	/* Global depth for an index. */

static unsigned int get_pid (void);
static unsigned int get_ipid (void);

void page_init (void)
{
	TOTAL_PAGE = 0;
	TOTAL_INDEX_PAGE = 0;
	GLOBAL_DEPTH = 2;
}

void init_page (struct page *cur, unsigned short depth, bool for_result_page)
{
	if (!for_result_page)		/* Result page is not included in the number of pages. */
		cur->pid = get_pid ();
	cur->pin_cnt = 0;
	cur->ref_bit = false;
	cur->local_depth = depth;	/* Depth of parent */
	cur->dirctory[0] = 0;
	cur->dirctory[1] = 1;		/* No record yet. */
	cur->opid = -1;				/* No overflow page yet.*/
	for (int i = 0; i < DEPOT_SIZE; i++)
		cur->data[i] = '\0';
}

void init_index_page (struct index_page *cur)
{
	cur->pid = get_ipid ();
	cur->pin_cnt = 0;
	cur->ref_bit = false;
}

unsigned int get_ipid (void)
{
	assert (TOTAL_INDEX_PAGE <= INDEX_FIELD_SIZE);
	return TOTAL_INDEX_PAGE++;	// strart from 0.
}

void prepare_page (struct page *cur, char *temp)
{
	assert (cur->data[DEPOT_SIZE - 1] == '\n' || cur->data[DEPOT_SIZE - 1] == '\0');
	++cur->local_depth;
	for (int i = 0; i < DEPOT_SIZE; i++)
	{
		temp[i] = cur->data[i];
		cur->data[i] = '\0';
	}
	cur->dirctory[0] = 0;
	cur->dirctory[1] = 1;
}

static unsigned int get_pid (void)
{
	return INDEX_FIELD_SIZE + TOTAL_PAGE++; // start from INDEX_FIELD_SIZE.
}

int get_fs_ptr (const struct page *cur)
{
	return cur->dirctory[0];
}

int record_count (const struct page *cur)
{
	return cur->dirctory[1] - 1;
}

void get_record (const struct page *cur, char *record, const unsigned short record_num, int size)
{
	unsigned short i = cur->dirctory[record_num + 2];	// record # start from 0.
	int j;
	
	for (j = 0; j < size - 1 && cur->data[i] != '\n'; j++, i++)
	{
		record[j] = cur->data[i];
	}

	if (j == size - 1)
	{
		printf ("Record buffer copy overflow.\n");
		exit(0);
	}
	else
	{
		record[j++] = cur->data[i++];
		record[j] = '\0';
	}
}

bool is_pinned (const struct page *cur)
{
	return cur->pin_cnt != 0;
}

bool is_ref (const struct page *cur)
{
	return cur->ref_bit;
}