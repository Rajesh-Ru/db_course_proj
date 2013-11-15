/* Definition of Page.
   Rajesh
   5/1/2012 */

#ifndef PAGE_H
#define PAGE_H

#define INDEX_FIELD_SIZE 550	/* The number pages reserved for the index. */
#define DEPOT_SIZE 7903
#define ENTRIES_SIZE 2046

struct page
{
	unsigned int pid;				/* Page number. */
	bool ref_bit;					/* Refernece bit used in clock replacement algorithm. */
	unsigned short pin_cnt;			/* Pin count. */
	unsigned short local_depth;     /* Local depth when it's viewed as a bucket. */
	unsigned short dirctory[136];	/* In format: ptr to free space, # of records + 1, record #s. */
	char data[DEPOT_SIZE];			/* Records. */
	int opid;							/* Overflow page pointer. */
};

struct index_page
{
	unsigned int pid;
	unsigned short pin_cnt;
	bool ref_bit;
	unsigned int entries[ENTRIES_SIZE];		/* pids. */
};

void page_init (void);
void init_page (struct page *cur, unsigned short depth, bool for_result_page = false);
void init_index_page (struct index_page *cur);
void prepare_page (struct page *cur, char *temp);
int get_fs_ptr (const struct page *cur);
int record_count (const struct page *cur);
void get_record (const struct page *cur, char *record, const unsigned short record_num, int size);
bool is_pinned (const struct page *cur);
bool is_ref (const struct page *cur);


#endif /* PAGE_H */