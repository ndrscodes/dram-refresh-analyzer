#include <emmintrin.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <x86intrin.h>
#include <sched.h>

struct timespec tstart={0,0}, tend={0,0};
typedef struct {
  uint64_t ts;
  uint64_t duration;
} measurement;
const size_t N_MEASUREMENTS = 300000;

void take_measurements(measurement* arr, size_t n, volatile char* row) {
  uint32_t tsc_aux;
  for(int i = 0; i < N_MEASUREMENTS; i++) {
    _mm_clflush((void*)row);
    _mm_lfence();
    
    uint64_t start = __rdtscp(&tsc_aux);
    _mm_lfence();

    *row;

    _mm_lfence();
    uint64_t end = __rdtscp(&tsc_aux);

    arr[i].duration = end - start;
    arr[i].ts = end;
  }
}

int compare_int( const void* a, const void* b )
{
  if( *(uint64_t*)a == *(uint64_t*)b ) return 0;
  return *(uint64_t*)a < *(uint64_t*)b ? -1 : 1;
}

uint64_t median(uint64_t arr[], size_t n) {
  uint64_t* cpy = malloc(n * sizeof(uint64_t));
  memcpy(cpy, arr, n * sizeof(uint64_t));
  qsort(cpy, n, sizeof(uint64_t), compare_int);
  uint64_t med = *(cpy + n / 2);
  free(cpy);
  return med;
}

uint64_t find_threshold(measurement times[], size_t n) {
  uint64_t sum = 0;

  for(int i = 0; i < N_MEASUREMENTS; i++) {
    sum += times[i].duration;
  }

  double avg = (double)sum / N_MEASUREMENTS;
  printf("the average measurement duration was %f ns based on %lu measurements and a sum of %lu, determining refresh interval...\n", avg, N_MEASUREMENTS, sum);

  uint64_t peaks[N_MEASUREMENTS];
  size_t npeaks = 0;
  for(size_t i = 0; i < N_MEASUREMENTS; i++) {
    if(times[i].duration > avg * 1.02) {
      peaks[npeaks++] = times[i].duration;
    }
  }
  printf("found %lu peaks.\n", npeaks);

  uint64_t med = median(peaks, npeaks);
  printf("median peak duration seems to be %lu ns\n", med);
  return avg + ((med - avg) / 2);
}

double_t avg_trefi(measurement times[], size_t n) {
  uint64_t threshold = find_threshold(times, N_MEASUREMENTS);
  uint64_t lpeak = 0;
  uint64_t peak_sum = 0;
  uint64_t npeaks = 0;
  for(int i = 0; i < N_MEASUREMENTS; i++) {
    if(times[i].duration > threshold) {
      npeaks++;
      if(lpeak != 0) {
        peak_sum += times[i].ts - lpeak;
      }
      lpeak = times[i].ts;
    }
  }

  return peak_sum / (double_t)npeaks;
}

int main(int argc, char *argv[])
{
  void *ptr = mmap(NULL, 1, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0);
  if(ptr == MAP_FAILED) {
    printf("map failed");
    return EXIT_FAILURE;
  }

  (*((char*)ptr)) = 0x42;
  FILE* f = fopen("out.txt", "w+");
  if(f == NULL) {
    printf("file creation failed");
    return EXIT_FAILURE;
  }

  measurement times[N_MEASUREMENTS];

  //used as a preparation period for the OS to finish scheduling the program
  take_measurements(times, N_MEASUREMENTS / 10, (volatile char*)ptr);
  sched_yield();
  take_measurements(times, N_MEASUREMENTS, (volatile char*)ptr);

  for(int i = 0; i < N_MEASUREMENTS; i++) {
    fprintf(f, "%lu;%lu\n", times[i].duration, times[i].ts);
  }
  
  double peak_avg = avg_trefi(times, N_MEASUREMENTS);
  printf("calculated refresh interval to be %f cycles", peak_avg);

  return EXIT_SUCCESS;
}
