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
    ++len; // size in memory, not string size
	do
	{
        where = hash_table + i;
		if (where->str == NULL)
		{
			where->str = malloc(len);
            if (where->str == NULL)
            {
                fprintf(stderr, "Unable to allocate %g bytes for string\n", (double)len);
                exit(1);
            }
			memcpy(where->str, str, len);
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

char* buffer;
size_t buffer_size = 1 << 16; // 64k

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

typedef enum
{
    IN_WORD = 0,
    IN_SPACE = 1,
} State;

typedef enum
{
    WORD_CHR = 0,
    SPACE_CHR = 2,
} CharType;

const char* const output_format = (sizeof(size_t) == sizeof(long long) ? "%s\t%llu\n" : "%s\t%u\n");

int main(int argc, char* argv[])
{
    CharType chartype;
    State state = IN_SPACE;
    
	size_t len, allocated_buffer_size;
	hash_rec* hash_ptr;
	char* const program_name = argv[0];
    char *buffer_pos, *buffer_end, *word_pos, *new_buffer;
	for (++argv; *argv; ++argv)
	{
		if (strcmp("-h", *argv) == 0 || strcmp("--help", *argv) == 0)
		{
			printf("Simple word counting application, author: borbely@math.bme.hu\nUSAGE: %s [options] < your.favorite.text.txt > words.and.counts.txt\n", program_name);
			printf("	-b --buffer	sets buffer size, default: %llu\n", (unsigned long long)buffer_size);
			printf("	-h --help	show this help and exit\n");
			printf("	-r --rehash	determines the fraction above which a rehash is performed, default: %g\n", rehash_constant);
			printf("	-e --expand	the fraction determining how much more space is allocated during a rehash, default: %g\n", expand_constant);
			printf("	-n --initial	initial hash table size, default: %d\n", (int)hash_table_size);
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
		else if ((strcmp("-b", *argv) == 0 || strcmp("--buffer", *argv) == 0) && *(argv+1))
		{
			buffer_size = atoll(*++argv) < 1 ? 1 : (size_t)atoll(*argv);
		}
	}

	buffer = malloc(buffer_size);
	if (buffer == NULL)
	{
		fprintf(stderr, "Unable to allocate %g bytes for buffer\n", (double)(buffer_size));
		return 1;
	}
    allocated_buffer_size = buffer_size;
    buffer_pos = buffer;

    hash_table = calloc(hash_table_size, sizeof(hash_rec));

    while (0 < (len = fread(buffer_pos, 1, buffer_size, stdin)))
    {   // process buffer
        buffer_end = buffer_pos + len;
        
        for (word_pos = buffer; buffer_pos < buffer_end; ++buffer_pos)
        {   // process word
            switch(*buffer_pos)
            {
                case ' ':
                case '\t':
                case '\n':
                case '\r':
                case '\v':
                case '\f':
                    chartype = SPACE_CHR;
                break;
                default:
                    chartype = WORD_CHR;
            }
            switch(chartype + state)
            {
                case (IN_WORD + SPACE_CHR):
                {
                    *buffer_pos = '\0';
                    len = buffer_pos - word_pos;
                    hash_insert(word_pos, len);
                    if (actual_hash_size > rehash_constant*hash_table_size)
                        rehash();
                    state = IN_SPACE;
                }break;
                case (IN_SPACE + WORD_CHR):
                {
                    word_pos = buffer_pos;
                    state = IN_WORD;
                }break;
                default: break;
            }
        }
        // take care of the end chunk
        switch (state)
        {
            case IN_WORD: // leftovers
                len = buffer_end - word_pos;
                if (len + buffer_size > allocated_buffer_size)
                {
                    if (NULL != (new_buffer = malloc(len + buffer_size)))
                    {
                        allocated_buffer_size = len + buffer_size;
                        memcpy(new_buffer, word_pos, len);
                        free(buffer);
                        buffer = new_buffer;
                    }
                    else
                    {
                        fprintf(stderr, "unable t allocate %g bytes!\n", (double)(len + buffer_size));
                        return 1;
                    }
                }else
                {
                    memcpy(buffer, word_pos, len);
                }
                buffer_pos = buffer + len;
            break;
            case IN_SPACE: // clean finish
                buffer_pos = buffer;
            break;
        }
    }
    if (!feof(stdin))
    {
        fprintf(stderr, "fread failed before eof\n");
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
