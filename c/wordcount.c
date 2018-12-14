#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static size_t hash_function(const char* _First)
{
	size_t _Val = 14695981039346656037ULL;
	for (; *_First; ++_First)
	{
		_Val ^= (size_t)(*_First);
		_Val *= 1099511628211ULL;
	}
	return _Val;
}

typedef struct REC
{
	char* str;
	size_t count;
} hash_rec;

size_t hash_table_size = 4096, actual_hash_size = 0;
hash_rec* hash_table;
double rehash_constant = 0.8;
double expand_constant = 2.0;

void rehash()
{
	hash_rec* what;
	const size_t new_table_size = ceil(expand_constant*hash_table_size);
	size_t new_hash_val, i;
	hash_rec* new_table = calloc(new_table_size, sizeof(hash_rec));
	
	if (new_table == NULL)
	{
		fprintf(stderr, "Unable to allocate %g byte memory for rehash!\n", (double)(new_table_size*sizeof(hash_rec)));
		exit(1);
	}
	for (what = hash_table; what < hash_table + hash_table_size; ++what)
		if (what->str)
		{
			new_hash_val = hash_function(what->str) % new_table_size;
            i = new_hash_val;
			do {
				if (new_table[i].str == NULL)
				{
					new_table[i] = *what;
					break;
				}
                i = (i+1) % new_table_size;
			} while(i != new_hash_val);
		} 
	hash_table_size = new_table_size;
	free(hash_table);
	hash_table = new_table;
}

void hash_insert(const char* str, size_t len)
{
	const size_t supposed_to_be = hash_function(str) % hash_table_size;
	hash_rec* where;
    size_t i = supposed_to_be;
	do
	{
        where = hash_table + i;
		if (where->str == NULL)
		{
			where->str = malloc(len + 1);
            if (where->str == NULL)
            {
                fprintf(stderr, "Unable to allocate %g bytes for string\n", (double)(len+1));
                exit(1);
            }
			memcpy(where->str, str, len + 1);
			where->count = 1;
			++actual_hash_size;
			return;
		}
		else if (strcmp(where->str, str) == 0)
		{
			++(where->count);
			return;
		}
        i = (i+1) % hash_table_size;
	} while(i != supposed_to_be);
}

static int comparator(const void* left, const void* right)
{
	hash_rec* const l = (hash_rec*)left, * const r = (hash_rec*)right;
	return (l->count > r->count) ? -1 : (l->count == r->count ? strcmp(l->str, r->str) : 1);
}

char* word;
char input_format[100];
int MAX_WORD_LENGTH = 1000;

static void partition(hash_rec* _First, hash_rec* _Last)
{
    hash_rec temp;
    for (; ; ++_First)
    {   // find any out-of-order pair
        for (; _First != _Last && _First->count > 0; ++_First)
          ;   // skip in-place elements at beginning
        if (_First == _Last)
          break;  // done

        for (; _First != --_Last && _Last->count == 0; )
          ;   // skip in-place elements at end
        if (_First == _Last)
          break;  // done

        temp = *_First;
        *_First = *_Last;
        *_Last = temp;
    }
}

int main(int argc, char* argv[])
{
	const char* output_format = (sizeof(size_t) == sizeof(long long) ? "%s\t%llu\n" : "%s\t%u\n");

	size_t len;
	hash_rec* hash_ptr;
	char* const program_name = argv[0];

	for (++argv; *argv; ++argv)
	{
		if (strcmp("-h", *argv) == 0 || strcmp("--help", *argv) == 0)
		{
			printf("Simple word counting application, author: borbely@math.bme.hu\nUSAGE: %s [options] < your.favorite.text.txt > words.and.counts.txt\n", program_name);
			printf("\t-m --max\tsets maximum word length, default: %d\n", MAX_WORD_LENGTH);
			printf("\t-h --help\tshow this help and exit\n");
			// printf("\t-d --dummy\tuses a dummy hash function, default is %s\n", hash_function == hash_FNV1a ? "FNV-1a" : "dummy");
			printf("\t-r --rehash\tdetermines the fraction above which a rehash is performed, default: %g\n", rehash_constant);
			printf("\t-e --expand\tthe fraction determining how much more space is allocated during a rehash, default: %g\n", expand_constant);
			printf("\t-n --initial\tinitial hash table size, default: %d\n", (int)hash_table_size);
			exit(0);
		}
		else if ((strcmp("-r", *argv) == 0 || strcmp("--rehash", *argv) == 0) && *(argv+1))
		{
			rehash_constant = atof(*++argv);
			rehash_constant = rehash_constant < 1.0 && rehash_constant > 0.0 ? rehash_constant : 0.8;
		}
		else if ((strcmp("-e", *argv) == 0 || strcmp("--expand", *argv) == 0) && *(argv+1))
		{
			expand_constant = atof(*++argv);
			expand_constant = expand_constant > 1.0 ? expand_constant : 1.5;
		}
		else if ((strcmp("-n", *argv) == 0 || strcmp("--initial", *argv) == 0) && *(argv+1))
		{
			hash_table_size = atoi(*++argv) < 1 ? 1 : (size_t)atoi(*argv);
		}
		else if ((strcmp("-m", *argv) == 0 || strcmp("--max", *argv) == 0) && *(argv+1))
		{
			MAX_WORD_LENGTH = atoi(*++argv) < 1 ? 1 : (size_t)atoi(*argv);
		}
		// else if (strcmp("-d", *argv) == 0 || strcmp("--dummy", *argv) == 0)
		// {
			// hash_function = dummy_hash;
		// }
	}

	word = malloc(sizeof(char)*(MAX_WORD_LENGTH + sizeof(size_t)));
	if (word == NULL)
	{
		fprintf(stderr, "Unable to allocate %g bytes\n", (double)(sizeof(char)*(MAX_WORD_LENGTH + sizeof(size_t))));
		return 1;
	}

	hash_table = calloc(hash_table_size, sizeof(hash_rec));

	sprintf(input_format, "%%%ds", MAX_WORD_LENGTH);

	while (scanf(input_format, word) == 1)
	{
		len = strlen(word);
		hash_insert(word, len);
		if (actual_hash_size > rehash_constant*hash_table_size)
			rehash();
	}
	if (!feof(stdin))
	{
		fprintf(stderr, "scanf failed before eof\n");
		return 1;
	}

    partition(hash_table, hash_table + hash_table_size);
	qsort(hash_table, actual_hash_size, sizeof(hash_rec), comparator);
    hash_ptr = hash_table + actual_hash_size;
    --hash_table;
	while (++hash_table < hash_ptr)
		printf(output_format, hash_table->str, hash_table->count);

	return 0;
}
