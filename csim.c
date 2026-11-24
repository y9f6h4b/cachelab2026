// student id: xxx
// please change the above line to your student id

#include <assert.h>
#include <errno.h>
#include <getopt.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

void printSummary(int hits, int misses, int evictions)
{
  printf("hits:%d misses:%d evictions:%d\n", hits, misses, evictions);
  FILE *output_fp = fopen(".csim_results", "w");
  assert(output_fp);
  fprintf(output_fp, "%d %d %d\n", hits, misses, evictions);
  fclose(output_fp);
}

void printHelp(const char *name)
{
  printf(
      "Usage: %s [-hv] -s <num> -E <num> -b <num> -t <file>\n"
      "Options:\n"
      "  -h         Print this help message.\n"
      "  -v         Optional verbose flag.\n"
      "  -s <num>   Number of set index bits.\n"
      "  -E <num>   Number of lines per set.\n"
      "  -b <num>   Number of block offset bits.\n"
      "  -t <file>  Trace file.\n\n"
      "Examples:\n"
      "  linux>  %s -s 4 -E 1 -b 4 -t traces/yi.trace\n"
      "  linux>  %s -v -s 8 -E 2 -b 4 -t traces/yi.trace\n",
      name, name, name);
}

typedef struct
{
  uint64_t tag;
  uint64_t last_used;
} CacheLine;

typedef enum
{
  HIT,
  MISS,
  MISS_AND_EVICT
} AccessResult;

static int parse_number(const char *text, size_t *number)
{
  char *end;
  if (*text < '0' || *text > '9')
    return 0;
  errno = 0;
  uintmax_t value = strtoumax(text, &end, 10);
  if (errno == ERANGE || *end != '\0' || value > SIZE_MAX)
    return 0;
  *number = (size_t)value;
  return 1;
}

static AccessResult access_set(CacheLine *set, size_t ways, uint64_t need_tag, uint64_t timestamp)
{
  CacheLine *victim = set;
  for (size_t way = 0; way < ways; ++way)
  {
    CacheLine *line = &set[way];
    if (line->last_used && line->tag == need_tag)
    {
      line->last_used = timestamp;
      return HIT;
    }
    if (line->last_used < victim->last_used)
      victim = line;
  }

  AccessResult result = victim->last_used ? MISS_AND_EVICT : MISS;
  *victim = (CacheLine){need_tag, timestamp};
  return result;
}

int main(int argc, char *argv[])
{
  size_t set_bits = SIZE_MAX, ways = 0, block_bits = SIZE_MAX;
  const char *trace_path = NULL;
  int verbose = 0;
  int option;
  while ((option = getopt(argc, argv, "hvs:E:b:t:")) != -1)
  {
    size_t *parameter;
    switch (option)
    {
      case 'h': printHelp(argv[0]); return EXIT_SUCCESS;
      case 'v': verbose = 1; continue;
      case 's': parameter = &set_bits; break;
      case 'E': parameter = &ways; break;
      case 'b': parameter = &block_bits; break;
      case 't': trace_path = optarg; continue;
      default: goto ultima;
    }
    if (!parse_number(optarg, parameter))
    {
      fprintf(stderr, "Invalid value for -%c: %s\n", option, optarg);
      goto ultima;
    }
  }

  if (set_bits >= 64 || block_bits >= 64 || set_bits + block_bits > 64 ||
      !ways || !trace_path || optind != argc || set_bits >= sizeof(size_t) * CHAR_BIT)
  {
    fprintf(stderr, "Invalid or missing cache configuration; use -h for help.\n");
    goto ultima;
  }

  size_t set_count = (size_t)1 << set_bits;
  if (ways > SIZE_MAX / sizeof(CacheLine) / set_count)
  {
    fprintf(stderr, "Cache configuration is too large.\n");
    goto ultima;
  }

  FILE *trace = fopen(trace_path, "r");
  if (!trace)
  {
    perror(trace_path);
    goto ultima;
  }

  //!!! File on !!!

  CacheLine *cache = calloc(set_count * ways, sizeof(*cache));
  if (!cache)
  {
    perror("Cache allocation");
    goto file_ultima;
  }

  //!!! mem allocated !!!

  int hits = 0, misses = 0, evictions = 0;
  uint64_t timestamp = 0;
  char line[256];
  while (fgets(line, sizeof(line), trace))
  {
    char operation;
    uint64_t address;
    int size;
    if (sscanf(line, " %c %" SCNx64 ",%d", &operation, &address, &size) != 3 ||
        (operation != 'L' && operation != 'S'))
      continue;

    uint64_t block = address >> block_bits;
    size_t set_index = (size_t)(block & (set_count - 1));
    AccessResult result = access_set(&cache[set_index * ways], ways, block >> set_bits, ++timestamp);
    hits += result == HIT;
    misses += result != HIT;
    evictions += result == MISS_AND_EVICT;
    if (verbose)
      printf("%c %" PRIx64 ",%d %s\n", operation, address, size,
             result == HIT ? "hit" : result == MISS ? "miss" : "miss eviction");
  }

  int read_failed = ferror(trace);
  free(cache);
  fclose(trace);
  if (read_failed)
  {
    fprintf(stderr, "Failed to read trace: %s\n", trace_path);
    goto ultima;
  }
  printSummary(hits, misses, evictions);
  return EXIT_SUCCESS;

  file_ultima:
  fclose(trace);
  ultima:
  return EXIT_FAILURE;
}
