// Extendible Hashing.cpp : Defines the entry point for the console application.
//
/* Entry of the application.
   Rajesh
   5/1/2012 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include "Buffer.h"
#include "Page.h"
#include "HashFunction.h"

extern int TOTAL_PAGE;
extern int TOTAL_INDEX_PAGE;
extern int GLOBAL_DEPTH;
extern struct page buffer[BUFFER_NUM];
extern bool used[BUFFER_NUM];
extern struct index_page index_buffer[INDEX_BUFFER_NUM];
extern bool ibuff_used[INDEX_BUFFER_NUM];

//void tchar2char (TCHAR* tch_str, char *char_str, int size);
FILE *data_import (FILE *in_stream, FILE *out_stream);
void run_test (FILE *data_stream, FILE *test_in, FILE *test_out);  // no implementation
void index_init (void);
void get_search_key (char *search_key, const char *depot, int pos);
bool find_page_in_pool (unsigned int pid, int *frame_num);
int choose (bool is_data_page_mode = true);
int power (int base, int exponent);
void update_index (unsigned int p_pid, unsigned int s_pid, unsigned short local_depth,
					int discriminator, FILE *out_stream);
void bulk_read (char *depot, char *remainder, int *r_num, FILE *in_stream);
int find_target_frame (int target_pid, FILE *out_stream);
int insert_record (struct page *target_page, const char *depot, int *pos,
					bool *finished, bool *split, FILE *out_stream, bool forced = false);
void split_page_insert_again (struct page *target_page, char *depot, int *pos,
								bool *finished, bool *split, FILE *out_stream);
struct page *add_overflow_page (int parent_pid, FILE *out_stream);
void redistribute (struct page *splited_page, struct page *split_page, FILE *out_stream);
int get_target_iframe (int entry_num, FILE *out_stream);
unsigned int *index (int entry_num, FILE *out_stream);
bool search_by_key (const char *target_key, struct page *result, FILE *data_stream);
void sort_record_in_page (struct page *target_page, int field_num, unsigned short *ordered_rids);
void insert_ordered_double (unsigned short *fellow, int val, int *according_vals, int sort_key_val, int size);
void put_result (struct page *result, unsigned short *ordered_rids, FILE *test_out);
char *fget_num (char *num_str, int size, FILE *input_stream);

int main()
{
	//char path[100];
	//char file_name[100];
	//char command[100];
	//int command_num;
	char path_[100] = "D:\\sysu\\Database Systems\\TPC\\tpch_2_14_3\\dbgen";
	std::string path;
	FILE *in_stream, *out_stream, *data_handle;

	//assert (argc != 2);

	//tchar2char (argv[1], path, 100);

	page_init ();
	buffer_init ();
	index_init ();
	path = path_;

	//while (get_command (command))
	//{
	//	command_num = parse_command (command);
	//	do_command(command_num);
	//}

	in_stream = fopen ((path + "\\lineitem.tbl").c_str(), "rb");
	out_stream = fopen ((path + "\\hashindex.txt").c_str(), "wb+");
	assert (in_stream != NULL && out_stream != NULL);
	data_handle = data_import (in_stream, out_stream);

	//printf ("%d\n", power (2, GLOBAL_DEPTH));
	//system ("pause");

	in_stream = fopen ((path + "\\testinput.txt").c_str(), "rb");
	out_stream = fopen ((path + "\\testoutput.txt").c_str(), "wb");
	run_test (data_handle, in_stream, out_stream);
	//while (!feof (in_stream))
	//	printf ("%d\n", atoi (fgets (path_, 100, in_stream)));

	system ("pause");

	return 0;
}

/* 1. Choose a frame from index_buffer to place the initial index whose
      size is 4.
   2. Load a new index page into the frame, add infomation accordingly. */
void index_init (void)
{
	/*index = (unsigned int *) malloc (sizeof (unsigned int) * TOTAL_PAGE);*/
	int ibuff_fnum;
	int bound = power (2, GLOBAL_DEPTH);
	//index = new (std::nothrow) unsigned int[bound];
	//assert (index != NULL); // debug
	ibuff_fnum = choose (false);	// index buffer page initialization mode.
	ibuff_used[ibuff_fnum] = true;
	init_index_page (&index_buffer[ibuff_fnum]);
	for(unsigned int i = 0; i < bound; i++)
		index_buffer[ibuff_fnum].entries[i] = INDEX_FIELD_SIZE + i;
}

FILE *data_import (FILE *in_stream, FILE *out_stream)
{
	char depot[DEPOT_SIZE];
	char remainder[200];
	char s_key[20];
	int r_num = 0;
	int target_frame;
	int pos;				// The next position in depot to be operated.
	bool finished, split;

	while (!feof (in_stream))
	{
		finished = split = false;
		pos = 0;

		bulk_read (depot, remainder, &r_num, in_stream);

		while (!finished)						// If depot hasn't been used up, loop again.
		{
		//int j = 10;
		//while (j--){
			get_search_key (s_key, depot, pos);
			target_frame = find_target_frame (*index (hash_func (s_key), out_stream), out_stream);
			
			unsigned int pid1 = buffer[target_frame].pid; // debug
			insert_record (&buffer[target_frame], depot, &pos, &finished, &split, out_stream);
			assert (pid1 == buffer[target_frame].pid); // debug
		//}
		//for (j = 0; j < BUFFER_NUM; j++ )
		//	printf ("\n\n%d\n%s\n\n", buffer[j].pid, buffer[j].data);
			if (split)
			{
				split_page_insert_again (&buffer[target_frame], depot, &pos, &finished, &split, out_stream);
				split = false;
			}
		} // while (!finished)
		pos = 0; // debug
	} // while (!feof (in_stream))

	return out_stream;
}

/* 1. Copy last turn's remainders into depot.
   2. Bulk read DEPOT_SIZE - r_num bytes start at &depot[r_num].
   3. Copy new remainders in depot to remainder, fill their positions with '\0''s and update r_num. */
void bulk_read (char *depot, char *remainder, int *r_num, FILE *in_stream)
{
	int i;
	size_t char_count;

	strncpy (depot, remainder, *r_num);
	char_count = fread (&depot[*r_num], sizeof (char), DEPOT_SIZE - *r_num, in_stream);
	if (!feof (in_stream))
		for (i = DEPOT_SIZE - 1; depot[i] != '\n' && i >= 0; i--);
	else
		i = *r_num + char_count;
	assert (i >=0); // debug
	if (i + 1 == DEPOT_SIZE)
		*r_num = 0;
	else
	{
		*r_num = 0;
		if (!feof (in_stream))
			for (i = i + 1; i < DEPOT_SIZE && *r_num < 200; i++)
			{
				remainder[(*r_num)++] = depot[i];
				depot[i] = '\0';
			}
		else
			for (; i < DEPOT_SIZE; i++)
				depot[i] = '\0';
		assert (*r_num < 200); // when *r_num == 200, it's possible that remainder[] is to small.
	}
}

/* 1. Use target page's id to find it.
   2. If it's in the buffer pool buffer[BUFFER_NUM], return the number of the frame that storing it.
	  Otherwise choose a page, write it out if it's used, read in the target into that frame, and
	  return the frames's number. */
int find_target_frame (int target_pid, FILE *out_stream)
{
	int target_fnum;

	if (find_page_in_pool (target_pid, &target_fnum))
		return target_fnum;
	else
	{
		target_fnum = choose ();
		if (used[target_fnum])
		{
			fseek (out_stream, sizeof (struct page) * buffer[target_fnum].pid, SEEK_SET);
			fwrite (&buffer[target_fnum], sizeof (struct page), 1, out_stream);
		}
		used[target_fnum] = true;
		fseek (out_stream, sizeof (struct page) * target_pid, SEEK_SET);
		fread (&buffer[target_fnum], sizeof (struct page), 1, out_stream);
		return target_fnum;
	}
}

/* Each record is terminated with a '\n'.
   1. copy the record start at depot[pos] character by character, until '\n' or
      fsp == DEPOT_SIZE.
   2. If '\n' is encountered, insertion succeeded. If next character is '\0' or
      i == DEPOT_SIZE, finished = true;
   3. If fsp == DEPOT_SIZE, the page is full. If there is an overflow page, get
      it, and repeat insertion for it. If finally no more overflow page exists,
	  then insertion failed. Thus, split = true. Fill remainders with '\0'.
   4. return the pid of the last overflow page for cascading new overflow page. */
int insert_record (struct page *target_page, const char *depot, int *pos,
					bool *finished, bool *split, FILE *out_stream, bool forced)
{
	assert (*pos < DEPOT_SIZE && *pos >= 0);
	//printf ("\n%d ", *pos);

	int i;
	int fnum_for_ovp;
	int last_opid = target_page->pid;		// return -2 if insertion is succeed.
											// return -3 if depot is empty.
											// otherwise, it's the pid of the last overflow
											// page (self inclusive).
	unsigned int fsp = target_page->dirctory[0];
	if (!forced)
		target_page->pin_cnt++;

	for (i = *pos; depot[i] != '\0' && depot[i] != '\n' && fsp < DEPOT_SIZE; i++)
		target_page->data[fsp++] = depot[i];

	if (depot[i] == '\0')
	{
		assert (i == 0); // debug
		*finished = true;
		*split = false;
		return -3;
	}
	else if (depot[i] == '\n' && fsp < DEPOT_SIZE)
	{
		target_page->data[fsp++] = depot[i++];
		if (i == DEPOT_SIZE || depot[i] == '\0')
			*finished = true;
		else
			*finished = false;
		*pos = i;
		target_page->dirctory[++target_page->dirctory[1]] = target_page->dirctory[0];
		target_page->dirctory[0] = fsp;
		*split = false;
		last_opid = -2;
	}
	else
	{
		for (fsp = target_page->dirctory[0]; fsp < DEPOT_SIZE; fsp++)
			target_page->data[fsp] = '\0';
		
		*split = true;
		*finished = false;

		if (target_page->opid != -1)
		{
			fnum_for_ovp = find_target_frame (target_page->opid, out_stream);
			last_opid = insert_record (&buffer[fnum_for_ovp], depot, pos, finished, split, out_stream, true);
			if (last_opid == -2)
				last_opid = target_page->opid;
		}
	}

	//printf ("%d  %d\n", *pos, fsp);
	if (!forced)
	{
		target_page->pin_cnt--;
		target_page->ref_bit = true;
	}
	return last_opid;
}

/* 1. Split: choose a new frame other than the one containing parent page. Write
      it out if it's used and then load it with a new page. Update the index accordingly.
   2. Redistributes records in the parent page. Overflow page may exist. Mind it!
   3. The insert the record again. If insertion fails, add an overflow page and insert
      it into the overflow page. */
void split_page_insert_again (struct page *target_page, char *depot, int *pos,
								bool *finished, bool *split, FILE *out_stream)
{
	int new_page_fnum;
	unsigned int pid_again;
	int ovp_pid;
	struct page *new_page, *overflow_page;
	char s_key[20];

	target_page->pin_cnt++;
	new_page_fnum = choose ();
	new_page = &buffer[new_page_fnum];
	target_page->pin_cnt--;

	if (used[new_page_fnum])
	{
		fseek (out_stream, sizeof (struct page) * new_page->pid, SEEK_SET);
		fwrite (new_page, sizeof (struct page), 1, out_stream);
	}
	used[new_page_fnum] = true;

	//prepare_page (target_page, tmp);
	init_page (new_page, target_page->local_depth + 1);
	update_index (target_page->pid, new_page->pid, target_page->local_depth + 1,
					power (2, target_page->local_depth), out_stream);

	unsigned int pid1 = target_page->pid, pid2 = new_page->pid; // debug
	redistribute (target_page, new_page, out_stream);
	assert (pid1 == target_page->pid && pid2 == new_page->pid); // debug
	*split = false;

	get_search_key (s_key, depot, *pos);
	pid_again = *index (hash_func(s_key), out_stream);
	if (pid_again == target_page->pid)
		ovp_pid = insert_record (target_page, depot, pos, finished, split, out_stream, true);
	else if (pid_again == new_page->pid)
		ovp_pid = insert_record (new_page, depot, pos, finished, split, out_stream, true);
	else
	{
		printf ("Page number panic!\n");
		exit (1);
	}

	if (*split)		// add overflow page.
	{
		assert (ovp_pid != -2 && ovp_pid != -3); // debug
		overflow_page = add_overflow_page (ovp_pid, out_stream);
		insert_record (overflow_page, depot, pos, finished, split, out_stream);
	}
}

/* 1. Redistribute the records in splited_page to itself and its split.
   2. If splited_page has overflow page(s), also do redistribution for
      it(them).
   3. If split_page is full, add overflow page cascading it and continue
      redistribution by using the overflow page. */
void redistribute (struct page *splited_page, struct page *split_page, FILE *out_stream)
{
	struct page *p_ovp = splited_page, *s_ovp = split_page;
	bool finished, overflow;
	unsigned int target_pid;
	int ovp_pid;
	int tmp_pos, ovp_fnum;
	char s_key[20];
	char tmp[DEPOT_SIZE];

	splited_page->pin_cnt++;
	split_page->pin_cnt++;

	do
	{
		prepare_page (p_ovp, tmp);
		tmp_pos = 0;
		finished = false;
		overflow = false;
		
		while (!finished)		// temp hasn't been used up
		{
			get_search_key (s_key, tmp, tmp_pos);
			target_pid = *index (hash_func (s_key), out_stream);

			if (target_pid == splited_page->pid)
			{
				s_ovp->pin_cnt++;
				insert_record (p_ovp, tmp, &tmp_pos, &finished, &overflow, out_stream, true);
				// sth wrong here. p_ovp contains incomplete record!!!
				s_ovp->pin_cnt--;
			}
			else if (target_pid == split_page->pid)
			{
				p_ovp->pin_cnt++;
				insert_record (s_ovp, tmp, &tmp_pos, &finished, &overflow, out_stream);
				p_ovp->pin_cnt--;
			}
			else
			{
				printf ("Page number panic!\n");
				exit (2);
			}

			if (overflow)
			{
				p_ovp->pin_cnt++;
				s_ovp = add_overflow_page (s_ovp->pid, out_stream);
				p_ovp->pin_cnt--;
				overflow = false;
			}
		} // while (!finished)

		if ((ovp_pid = p_ovp->opid) != -1)
		{
			s_ovp->pin_cnt++;
			ovp_fnum = find_target_frame (ovp_pid, out_stream);
			p_ovp = &buffer[ovp_fnum];
			s_ovp->pin_cnt--;
		}
	} while (ovp_pid != -1);

	splited_page->pin_cnt--;
	split_page->pin_cnt--;
}

/* Apply for a new overflow page and return its handle. */
struct page *add_overflow_page (int parent_pid, FILE *out_stream)
{
	int parent_fnum, ovp_fnum;

	parent_fnum = find_target_frame (parent_pid, out_stream);
	buffer[parent_fnum].pin_cnt++;

	ovp_fnum = choose ();
	if (used[ovp_fnum])
	{
		fseek (out_stream, sizeof (struct page) * buffer[ovp_fnum].pid, SEEK_SET);
		fwrite (&buffer[ovp_fnum], sizeof (struct page), 1, out_stream);
	}
	used[ovp_fnum] = true;
	init_page (&buffer[ovp_fnum], buffer[parent_fnum].local_depth);
	buffer[parent_fnum].opid = buffer[ovp_fnum].pid;
	buffer[parent_fnum].pin_cnt--;
	
	return &buffer[ovp_fnum];
}

void get_search_key (char *search_key, const char *depot, int pos)
{
	if (pos >= DEPOT_SIZE || pos < 0)
	{
		printf ("%d\n", pos);
		system ("pause");
	}
	//assert (pos < DEPOT_SIZE && pos >= 0);
	int j = 0;
	for (int i = pos; depot[i] != '|'; j++, i++)
		search_key[j] = depot[i];
	search_key[j] = '\0';
}

bool find_page_in_pool (unsigned int pid, int *frame_num)
{
	int i;

	for (i = 0; i < BUFFER_NUM; i++)
		if (buffer[i].pid == pid && used[i] == true)
			break;
	if (i == BUFFER_NUM)
		return false;
	else
		*frame_num = i;
	return true;
}

int choose (bool is_data_page_mode)
{
	if (is_data_page_mode)
	{
		int i = rand() % BUFFER_NUM;

		for (int j = 0; j < BUFFER_NUM; j++)
			if (!used[j])
				return j;

		while (true)
		{
			if (buffer[i % BUFFER_NUM].pin_cnt == 0 && buffer[i % BUFFER_NUM].ref_bit == true)
				buffer[i % BUFFER_NUM].ref_bit = false;
			if (buffer[i % BUFFER_NUM].pin_cnt == 0 && buffer[i % BUFFER_NUM].ref_bit == false)
				return i % BUFFER_NUM;
			i++;
		}
	}
	else
	{
		int i = rand() % INDEX_BUFFER_NUM;

		for (int j = 0; j < INDEX_BUFFER_NUM; j++)
			if (!ibuff_used[j])
				return j;

		while (true)
		{
			if (index_buffer[i % INDEX_BUFFER_NUM].pin_cnt == 0 && index_buffer[i % INDEX_BUFFER_NUM].ref_bit == true)
				index_buffer[i % INDEX_BUFFER_NUM].ref_bit = false;
			if (index_buffer[i % INDEX_BUFFER_NUM].pin_cnt == 0 && index_buffer[i % INDEX_BUFFER_NUM].ref_bit == false)
				return i % INDEX_BUFFER_NUM;
			i++;
		}
	}
}

int power (int base, int exponent)
{
	int result = 1;
	for (int i = 0; i < exponent; i++)
		result *= base;
	return result;
}

void update_index (unsigned int p_pid, unsigned int s_pid, unsigned short local_depth,
					int discriminator, FILE *out_stream)
{
	int bound, old_bound;
	int offset, ibuff_fnum, ibuff_fnum_new;

	if (local_depth <= GLOBAL_DEPTH)
	{
		bound = power (2, GLOBAL_DEPTH);
		for (int i = 0; i < bound; i++)
		{
			if (i % ENTRIES_SIZE == 0)
			{
				offset = 0;
				ibuff_fnum = get_target_iframe (i, out_stream);
			}
			if (index_buffer[ibuff_fnum].entries[offset++] == p_pid && (i & discriminator) != 0)
				index_buffer[ibuff_fnum].entries[offset - 1] = s_pid;
			//if (index[i] == p_pid && (i & discriminator) != 0)
			//	index[i] = s_pid;
		}
	}
	else
	{
		old_bound = power (2, GLOBAL_DEPTH);
		bound = power (2, ++GLOBAL_DEPTH);
		//temp = (unsigned int *) malloc (sizeof (unsigned int) * bound);
		//temp = new (std::nothrow) unsigned int[bound];
		//assert (temp != NULL); // debug
		for (int i = 0; i < old_bound; i++)
		{
			if (i % ENTRIES_SIZE == 0)
			{
				offset = 0;
				ibuff_fnum = get_target_iframe (i, out_stream);
				index_buffer[ibuff_fnum].pin_cnt++;
			}

			if ((old_bound + i) / ENTRIES_SIZE >= TOTAL_INDEX_PAGE)
			{
				ibuff_fnum_new = choose (false);
				if (ibuff_used[ibuff_fnum_new])
				{
					fseek (out_stream, sizeof (struct index_page) * index_buffer[ibuff_fnum_new].pid, SEEK_SET);
					fwrite (&index_buffer[ibuff_fnum_new], sizeof (struct index_page), 1, out_stream);
				}
				ibuff_used[ibuff_fnum_new] = true;
				init_index_page (&index_buffer[ibuff_fnum_new]);
			}
			
			if (index_buffer[ibuff_fnum].entries[offset++] == p_pid)
			{
				*index (old_bound + i, out_stream) = s_pid;
				//temp[i] = p_pid;
				//temp[old_bound + i] = s_pid;
			}
			else
				*index (old_bound + i, out_stream) = index_buffer[ibuff_fnum].entries[offset - 1];
			//	temp[i] = temp[old_bound + i] = index[i];
			if ((i + 1) % ENTRIES_SIZE == 0)
				index_buffer[ibuff_fnum].pin_cnt--;
		}
		if (index_buffer[ibuff_fnum].pin_cnt > 0)
			index_buffer[ibuff_fnum].pin_cnt--;
		// free (index);
		//delete [] index;
		//index = temp;
	}
}

int get_target_iframe (int entry_num, FILE *out_stream)
{
	int ifnum;
	unsigned int pid = entry_num / ENTRIES_SIZE;
	
	for (ifnum = 0; ifnum < INDEX_BUFFER_NUM; ifnum++)	// find in buffer pool first
		if (ibuff_used[ifnum] && index_buffer[ifnum].pid == pid)
			return ifnum;
	
	ifnum = choose (false);
	if (ibuff_used[ifnum])
	{
		fseek (out_stream, sizeof (struct index_page) * index_buffer[ifnum].pid, SEEK_SET);
		fwrite (&index_buffer[ifnum], sizeof (struct index_page), 1, out_stream);
	}
	ibuff_used[ifnum] = true;
	fseek (out_stream, sizeof (struct index_page) * pid, SEEK_SET);
	fread (&index_buffer[ifnum], sizeof (struct index_page), 1, out_stream);
	return ifnum;
}

/* This function receives a hash value and use it to find the corresponding entry
   in the index. It will use the hash value (the entry #) to calculate the pid of
   the page in which the entry resides. If the page is in the index_buffer pool,
   it will return a pointer to that entry. If the page is in the file, it swap in
   the page and then return the desired pointer. */
unsigned int *index (int entry_num, FILE *out_stream)
{
	int ibuff_fnum = get_target_iframe (entry_num, out_stream);
	return &index_buffer[ibuff_fnum].entries[entry_num % ENTRIES_SIZE];
}

/* 1. Use the target_key to find the corresponding bucket. Read it into buffer if necessary.
   2. Make use of the directory to traverse each record in the bucket. Compare each search
      key value to the value of target_key. If they are equal, copy the record to the result
	  page.
   3. If there is any overflow pages, repeat step 2 for those pages as well.
   P.S. The result page should have been initialized. */
bool search_by_key (const char *target_key, struct page *result, FILE *data_stream)
{
	unsigned int target_pid = *index (hash_func (target_key), data_stream);
	int target_fnum = find_target_frame (target_pid, data_stream);
	int start_offset, ovp_pid;
	char s_key[20];
	bool dummy, overflow = false;

	assert (result->dirctory[0] == 0 && result->dirctory[1] == 1);	// The result page should be empty.

	do
	{
		for (int i = 0; i < buffer[target_fnum].dirctory[1] -1; i++)
		{
			start_offset = buffer[target_fnum].dirctory[i + 2];
			get_search_key (s_key, buffer[target_fnum].data, start_offset);
			if (strcmp (target_key, s_key) == 0)
				insert_record (result, buffer[target_fnum].data, &start_offset, &dummy, &overflow, data_stream);

			assert (overflow == false);	// The result cannot over one page size recently.
		}

		if ((ovp_pid = buffer[target_fnum].opid) != -1)
		{
			target_pid = ovp_pid;
			target_fnum = find_target_frame (target_pid, data_stream);
		}
	} while (ovp_pid != -1);

	if (result->dirctory[1] == 1)
		return false;		// No such record.
	return true;
}

void run_test (FILE *data_stream, FILE *test_in, FILE *test_out)
{
	int num_of_request;
	unsigned short ordered_rids[100];
	char num_of_request_str[10];
	char target_key[20];
	struct page result;

	num_of_request = atoi (fget_num (num_of_request_str, 10, test_in));
	while (num_of_request--)
	{
		init_page (&result, true);
		fget_num (target_key, 20, test_in);	// Get a request.
		if (search_by_key (target_key, &result, data_stream))  // sth worng here!!!
		{
			sort_record_in_page (&result, 2, ordered_rids);
			put_result (&result, ordered_rids, test_out);
		}
		else
			fprintf (test_out, "%d\r\n", -1);
	}
}

void sort_record_in_page (struct page *target_page, int field_num, unsigned short *ordered_rids)
{
	int delimiter_cnt = 0;
	int num_of_record = target_page->dirctory[1] - 1;
	int according_vals[100];
	int sort_key_val;
	int offset;
	char sort_key[20];
	
	for (int i = 0; i < num_of_record; i++)		// Loop for all records.
	{
		delimiter_cnt = 0;

		for (offset = target_page->dirctory[i + 2]; delimiter_cnt != field_num -1; offset++)
			if (target_page->data[offset] == '|')
				delimiter_cnt++;
		get_search_key (sort_key, target_page->data, offset);
		sort_key_val = atoi (sort_key);

		insert_ordered_double (ordered_rids, i, according_vals, sort_key_val, i + 1);
	}
}

/* Insert sort_key_val into according_vals[] orderly (ascending) and
   fellow[] will simulate the acts of according_vals[]. */
void insert_ordered_double (unsigned short *fellow, int val, int *according_vals, int sort_key_val, int size)
{
	int i, j;

	if (size == 1)
	{
		fellow[0] = val;
		according_vals[0] = sort_key_val;
	}
	else
	{
		for (i = 0; i < size - 1; i++)
			if (according_vals[i] > sort_key_val)
				break;

		for (j = size - 1; j > i; j--)
		{
			according_vals[j] = according_vals[j - 1];
			fellow[j] = fellow[j - 1];
		}

		according_vals[i] = sort_key_val;
		fellow[i] = val;
	}
}

void put_result (struct page *result, unsigned short *ordered_rids, FILE *test_out)
{
	int offset;
	for (int i = 0; i < result->dirctory[1] -1; i++)
	{
		offset = result->dirctory[ordered_rids[i] + 2];
		while (result->data[offset] != '\n')
			fputc (result->data[offset++], test_out);
		fputc (result->data[offset], test_out);
	}
	fprintf (test_out, "%d\r\n", -1);
}

/* Do the same work as fgets except that it will not store '\n' and '\r'. */
char *fget_num (char *num_str, int size, FILE *input_stream)
{
	char digit;
	int i = 0;

	assert (input_stream != NULL);

	while ((digit = fgetc (input_stream)) != '\n' && i < size - 1)
		num_str[i++] = digit;
	num_str[i - 1] = '\0';

	return num_str;
}

//void tchar2char (TCHAR* tch_str, char *char_str, int size) 
//{ 
//	int length = 2 * wcslen (tch_str);
//	assert (size < length + 1);
//	wcstombs (char_str, tch_str, length + 1);
//}

