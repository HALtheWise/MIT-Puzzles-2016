// Copyright (C) 2009-2010 Adam Rosenfield
//
// This file is part of Kakuro, the Kakuro puzzle solver.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXW 30
#define MAXCLUES (MAXW*MAXW)

int width, height, numsetsize;
int innumset[32] = {0};

int grid[MAXW*MAXW] = {0};
int startposs[MAXW*MAXW] = {0};

int clues[MAXCLUES][32];
int sums[MAXCLUES];
int cluelens[MAXCLUES];
int nclues = 0;
int invclues[MAXW*MAXW][2];

int nsolutions = 0;

int topleftparity = 0;

int maxthreads = 1;
volatile int idlethreads = 0;
pthread_mutex_t output_mutex;

typedef struct workqueue_datum
{
  int *poss;
  struct workqueue_datum *next;
} workqueue_datum;

workqueue_datum *workqueue;
int workqueue_size;
pthread_mutex_t workqueue_mutex;
pthread_cond_t workqueue_cond;

void usage(const char *argv0);
void read_puzzle(const char *filename);
int reduce_all(int *poss);
int reduce(int *poss, int cl);
int bitcount(int x);
void search(int *poss);
void print_solution(int *poss);
void *thread_proc(void *arg);

int main(int argc, char **argv)
{
  int i;
  for(i = 1; i < argc; i++)
  {
    if(argv[i][0] == '-')
    {
      if(sscanf(argv[i], "-threads=%d", &maxthreads) < 1 || maxthreads < 1)
        usage(argv[0]);
    }
    else
      break;
  }

  if(i != argc - 1)
    usage(argv[0]);

  read_puzzle(argv[argc - 1]);

#ifndef CHECKERBOARD_KAKURO
  if(reduce_all(startposs) == -1)
  {
    printf("Found 0 solutions\n");
    return 0;
  }
#else
  // Checkerboard kakuro: adjacent cells alternate in parity (even/odd), just
  // like a checkerboard.  So we do two searches, one with the top-left cell
  // even and one with it odd.
  int j;
  int startposs2[MAXW*MAXW];
  memcpy(startposs2, startposs, width * height * sizeof(int));
  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
    {
      startposs [width*i + j] &= 0x55555555 <<  ((i + j) & 1);
      startposs2[width*i + j] &= 0x55555555 << !((i + j) & 1);
    }
  }

  reduce_all(startposs);
  reduce_all(startposs2);
#endif

  if(maxthreads == 1)
  {
    search(startposs);
#ifdef CHECKERBOARD_KAKURO
    search(startposs2);
#endif
  }
  else
  {
    pthread_mutex_init(&output_mutex, NULL);
    pthread_mutex_init(&workqueue_mutex, NULL);
    pthread_cond_init(&workqueue_cond, NULL);

    int *poss = (int *)malloc(width * height * sizeof(int));
    memcpy(poss, startposs, width * height * sizeof(int));

    workqueue = (workqueue_datum *)malloc(sizeof(workqueue_datum));
    workqueue->poss = poss;
    workqueue->next = NULL;
    workqueue_size = 1;

#ifdef CHECKERBOARD_KAKURO
    poss = (int *)malloc(width * height * sizeof(int));
    memcpy(poss, startposs2, width * height * sizeof(int));
    workqueue_datum *datum2 = (workqueue_datum *)malloc(sizeof(workqueue_datum));
    datum2->poss = poss;
    datum2->next = NULL;
    workqueue->next = datum2;
    workqueue_size++;
#endif

    idlethreads = maxthreads;

    pthread_t *tids = (pthread_t *)malloc(maxthreads * sizeof(pthread_t));

    for(i = 0; i < maxthreads; i++)
      pthread_create(&tids[i], NULL, thread_proc, NULL);
    for(i = 0; i < maxthreads; i++)
      pthread_join(tids[i], NULL);

    free(tids);
    assert(workqueue == NULL && workqueue_size == 0 && idlethreads == maxthreads);

    pthread_mutex_destroy(&output_mutex);
    pthread_mutex_destroy(&workqueue_mutex);
    pthread_cond_destroy(&workqueue_cond);
  }

  printf("Found %d solutions\n", nsolutions);

  /*int i, j;
  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
      printf(" %03x", poss[i * width + j]);
    printf("\n");
    }//*/

  /*
  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
      printf("%d ", grid[i * width + j]);
    printf("\n");
  }

  printf("\n\n");

  for(i = 0; i < nclues; i++)
  {
    printf("%d", sums[i]);
    for(j = 0; j < cluelens[i]; j++)
      printf(" %s %d/%d", j == 0 ? "=" : "+", clues[i][j] / width, clues[i][j] % width);
    printf("\n");
  }//*/

  return 0;
}

void usage(const char *argv0)
{
  fprintf(stderr, "Usage: %s [-threads=NUMTHREADS] puzzle-file\n", argv0);
  exit(1);
}

void read_puzzle(const char *filename)
{
  FILE *puzfile = fopen(filename, "r");
  if(puzfile == NULL)
  {
    fprintf(stderr, "%s: %s\n", filename, strerror(errno));
    exit(1);
  }

  if(fscanf(puzfile, "%d %d %d", &width, &height, &numsetsize) != 3 ||
     width < 2 || width > MAXW ||
     height < 2 || height > MAXW ||
     numsetsize < 1 || numsetsize > 32)
  {
    fprintf(stderr, "Error reading width/height/numsetsize\n");
    goto error;
  }

  int downclues[MAXW][MAXW];
  int acrossclues[MAXW][MAXW];

  int i, j, k, n;
  for(i = 0; i < numsetsize; i++)
  {
    if(fscanf(puzfile, "%d", &n) != 1 || n < 0 || n >= 32)
    {
      fprintf(stderr, "Error reading numset %d\n", i);
      goto error;
    }

    innumset[n] = 1;
  }

  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
    {
      char token[64];
      if(fscanf(puzfile, "%64s", token) != 1)
      {
        fprintf(stderr, "Unexpected EOF\n");
        goto error;
      }

      if(strcmp(token, "#") == 0)
        grid[i * width + j] = 1;
      else if(strcmp(token, ".") == 0)
        continue;
      else
      {
        char *backslash = strchr(token, '\\');
        if(backslash == NULL || (backslash == token && token[1] == 0))
        {
          fprintf(stderr, "Invalid token at (%d, %d): %s\n", i, j, token);
          goto error;
        }

        grid[i * width + j] = 2;

        if(backslash > token)
          sscanf(token, "%d", &downclues[i][j]);
        else
          downclues[i][j] = -1;

        if(backslash[1] != 0)
          sscanf(backslash + 1, "%d", &acrossclues[i][j]);
        else
          acrossclues[i][j] = -1;
      }
    }
  }

  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
    {
      int index = i * width + j;

      if(grid[index] != 0)
      {
        invclues[index][0] = -1;
        invclues[index][1] = -1;
      }

      if(grid[index] == 2)
      {
        if(acrossclues[i][j] != -1)
        {
          for(k = j + 1; k < width && grid[i * width + k] == 0; k++)
          {
            clues[nclues][cluelens[nclues]++] = i * width + k;
            invclues[i * width + k][0] = nclues;
          }

          assert(cluelens[nclues] >= 2);

          sums[nclues] = acrossclues[i][j];
          nclues++;
        }

        if(downclues[i][j] != -1)
        {
          for(k = i + 1; k < height && grid[k * width + j] == 0; k++)
          {
            clues[nclues][cluelens[nclues]++] = k * width + j;
            invclues[k * width + j][1] = nclues;
          }

          assert(cluelens[nclues] >= 2);

          sums[nclues] = downclues[i][j];
          nclues++;
        }
      }
    }
  }

  int allnums = 0;
  for(i = 0; i < 32; i++)
  {
    if(innumset[i])
      allnums |= (1 << i);
  }

  for(i = 0; i < width * height; i++)
  {
    if(grid[i] == 0)
      startposs[i] = allnums;
  }

  return;

 error:
  fclose(puzfile);
  exit(1);
}

// Not the most efficient implementation, but good enough...
int isprime(int num)
{
  if(!(num & 1))
    return 0;
  int i;
  for(i = 3; i * i <= num; i += 2)
  {
    if(num % i == 0)
      return 0;
  }

  return 1;
}

// Bit-scan forward: returns index of lowest bit set
inline int bsf(int p)
{
  int ret;
  asm("bsf %1, %0" : "=a"(ret) : "r"(p));
  return ret;
}

// Bit-scan reverse: returns index of highest bit set
inline int bsr(int p)
{
  int ret;
  asm("bsr %1, %0" : "=a"(ret) : "r"(p));
  return ret;
}

int reduce_all(int *poss)
{
  int i, ret;
  int changed;
  do
  {
    changed = 0;
    for(i = 0; i < nclues; i++)
    {
      ret = reduce(poss, i);
      if(ret == 1)
        changed = 1;
      else if(ret == -1)
        return -1;
    }
  } while(changed);

  return 0;
}

int reduce(int *poss, int cl)
{
  int len = cluelens[cl];
  int sum = sums[cl];
  int minsum = 0, maxsum = 0;
  int mins[32], maxs[32];
  int i, j;
  int ret = 0;
#ifdef BLACKJACK_KAKURO
  int canhave1 = 0;
#endif
#ifdef PRODUCT_KAKURO
  int reduced_clue = sum;
  minsum = 1;
  maxsum = 1;
#endif

  for(i = 0; i < len; i++)
  {
    int p = poss[clues[cl][i]];
    mins[i] = bsf(p);
    maxs[i] = bsr(p);
#ifndef PRODUCT_KAKURO
    minsum += mins[i];
    maxsum += maxs[i];
#else
    if(maxs[i] == 0)
      return -1;
    // Product kakuro: multiplication instead of addition
    minsum *= mins[i];
    maxsum *= maxs[i];

    if(mins[i] == maxs[i])
      reduced_clue /= mins[i];
#endif
#ifdef BLACKJACK_KAKURO
    // Blackjack Kakuro: each 1 can be counted as 1 or 11
    canhave1 |= (p & (1 << 1));
#endif
  }

#ifdef BLACKJACK_KAKURO
  if(canhave1 && minsum + 10 <= sum)
    maxsum += 10;
#endif

#ifdef COMPOSITE_KAKURO
  // Composite kakuro: the number formed by each entry must be a composite
  // (non-prime) number
  if(maxsum == minsum)
  {
    static const int lastdigitprime[10] = {0, 1, 0, 1, 0, 0, 0, 1, 0, 1};
    if(lastdigitprime[mins[len - 1]])
    {
      int num = 0;
      for(i = 0; i < len; i++)
        num = num * 10 + mins[i];
      if(isprime(num))
        return -1;
    }
  }
#endif

#ifdef MISTAKEN_KAKURO
  // Mistaken kakuro: every clue is either one higher or one lower
  // than the correct value for that sum
  int adjust = 1;
  if(minsum == maxsum && minsum != sum - 1 && minsum != sum + 1)
    return -1;
  if(maxsum <= sum)
  {
    sum--;
    adjust = 0;
  }
  else if(minsum >= sum)
  {
    sum++;
    adjust = 0;
  }
#endif

  for(i = 0; i < len; i++)
  {
    int min, max;
#ifndef PRODUCT_KAKURO
    min = sum - (maxsum - maxs[i]);
    max = sum - (minsum - mins[i]);
#else
    int divisor = maxsum / maxs[i];
    //if(mins[i] == maxs[i])
    {
      min = (sum + divisor - 1) / divisor;
      max = sum / (minsum / mins[i]);
    }
    //else
    // {
    //  min = (reduced_clue + divisor - 1) / divisor;
    //  max = reduced_clue / (minsum / mins[i]);
    //}
#endif

#ifdef MISTAKEN_KAKURO
    if(adjust)
    {
      min--;
      max++;
    }
#endif

    if(min < 0)
      min = 0;
    if(max > 31)
      max = 31;

    int idx = clues[cl][i];
    int p = poss[idx];
    int mask = ~((1 << min) - 1) & ((2 << max) - 1);

#ifdef PRODUCT_KAKURO
    if(mins[i] != maxs[i])
    {
      for(j = 2; j < 31; j++)
      {
        int m = (1 << j);
        if((p & m) && reduced_clue % j != 0)
          mask &= ~m;
      }
    }
#endif

#ifdef BLACKJACK_KAKURO
    if(min > 1 && max >= 11)
      mask |= (1 << 1);
#endif

    int p2 = p & mask;

    if(p2 == 0)
      return -1;

    if(p2 != p)
    {
      //printf("reduce(%d): (%d, %d): minsum=%d maxsum=%d min=%d max=%d %03x -> %03x\n", cl, clues[cl][i]/width, clues[cl][i]%width, minsum, maxsum, min, max, p, p2);
      poss[idx] = p2;
      ret = 1;
    }

#ifndef DUPLICATE_KAKURO
    if((p2 & (p2 - 1)) == 0)  // is only 1 bit set?
    {
#ifdef NONCONSECUTIVE_KAKURO
      // Nonconsecutive kakuro: consecutive digits are never in
      // adjacent cells
      if(idx % width > 0 && grid[idx - 1] == 0)
      {
        int p3 = poss[idx - 1];
        if(!(p3 & (p3 - 1)) && (p3 == (p2 << 1) || p3 == (p2 >> 1)))
          return -1;
      }

      if(idx >= width && grid[idx - width] == 0)
      {
        int p3 = poss[idx - width];
        if(!(p3 & (p3 - 1)) && (p3 == (p2 << 1) || p3 == (p2 >> 1)))
          return -1;
      }
#endif

      for(j = 0; j < len; j++)
      {
        if(j != i)
          poss[clues[cl][j]] &= ~p2;
      }
    }
#endif  // #ifndef DUPLICATE_KAKURO
  }

#ifdef DUPLICATE_KAKURO
  // Duplicate kakuro: each entry must have exactly one duplicated digit
  int dupcount = 0;
  int numsused = 0;
  int lastdupe = -1;
  for(i = 0; i < len; i++)
  {
    if(mins[i] == maxs[i])
    {
      if(numsused & (1 << mins[i]))
      {
        dupcount++;
        lastdupe = mins[i];
      }

      numsused |= (1 << mins[i]);
    }
  }

  if((minsum == maxsum && dupcount != 1) || dupcount > 1)
    return -1;

  if(dupcount == 1)
  {
    for(i = 0; i < len; i++)
    {
      if(mins[i] == maxs[i])
      {
        int mask = ~(1 << mins[i]);
        for(j = 0; j < len; j++)
        {
          if(j != i && (mins[i] != lastdupe || poss[clues[cl][j]] != (1 << lastdupe)))
            poss[clues[cl][j]] &= mask;
        }
      }
    }
  }
#endif

  return ret;
}

const int bitcount_table[256] =
{
  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
  4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

int bitcount(int x)
{
  return bitcount_table[x & 0xFF] +
    bitcount_table[(x >> 8) & 0xFF] +
    bitcount_table[(x >> 16) & 0xFF] +
    bitcount_table[(x >> 24) & 0xFF];
}

void *thread_proc(void *arg)
{
  while(1)
  {
    pthread_mutex_lock(&workqueue_mutex);
    if(idlethreads == maxthreads && workqueue_size == 0)
    {
      pthread_cond_broadcast(&workqueue_cond);
      pthread_mutex_unlock(&workqueue_mutex);
      return NULL;
    }

    if(workqueue != NULL)
    {
      int *poss = workqueue->poss;
      workqueue_datum *wq = workqueue;
      workqueue = workqueue->next;
      workqueue_size--;

      free(wq);

      idlethreads--;
      pthread_mutex_unlock(&workqueue_mutex);

      search(poss);
      free(poss);

      pthread_mutex_lock(&workqueue_mutex);
      idlethreads++;
      pthread_mutex_unlock(&workqueue_mutex);
    }
    else
    {
      pthread_cond_wait(&workqueue_cond, &workqueue_mutex);
      pthread_mutex_unlock(&workqueue_mutex);
    }
  }
}

void search(int *poss)
{
  int cl = -1;
  unsigned int minn = 0xFFFFFFFF;
  int maxn = 1;
  int i, j;

  for(i = 0; i < nclues; i++)
  {
    int n = 1;
    for(j = 0; j < cluelens[i]; j++)
      n *= bitcount(poss[clues[i][j]]);

    if(n < minn && n != 1)
    {
      cl = i;
      minn = n;
    }

    if(n > maxn)
      maxn = n;
  }

  if(maxn == 1)
  {
    print_solution(poss);
    return;
  }

  int poss2[MAXW*MAXW];
  memcpy(poss2, poss, width * height * 4);

  for(i = 0; i < cluelens[cl]; i++)
  {
    int idx = clues[cl][i];
    int p = poss[idx];

    if(p & (p - 1))  // at least 2 bits set?
    {
      for(j = 0; j < 32; j++)
      {
        if(p & (1 << j))
        {
          poss2[idx] = 1 << j;
          // this is a pessimization, don't uncomment
          //if(reduce(poss2, invclues[idx][0]) >= 0 && reduce(poss2, invclues[idx][1]) >= 0)
          if(reduce_all(poss2) == 0)
          {
            if(idlethreads > workqueue_size && (p & ~((2 << j) - 1)))
            {
              pthread_mutex_lock(&workqueue_mutex);
              if(idlethreads > workqueue_size)
              {
                int *poss3 = (int *)malloc(width * height * sizeof(int));
                memcpy(poss3, poss2, width * height * sizeof(int));
                workqueue_datum *wqitem = (workqueue_datum *)malloc(sizeof(workqueue_datum));
                wqitem->poss = poss3;
                wqitem->next = workqueue;
                workqueue = wqitem;
                workqueue_size++;
                pthread_cond_signal(&workqueue_cond);
                pthread_mutex_unlock(&workqueue_mutex);
              }
              else
              {
                pthread_mutex_unlock(&workqueue_mutex);
                search(poss2);
              }
            }
            else
              search(poss2);
          }

          memcpy(poss2, poss, width * height * 4);
        }
      }

      break;
    }
  }
}

void print_solution(int *poss)
{
  if(maxthreads > 1)
    pthread_mutex_lock(&output_mutex);

  nsolutions++;

  int i, j;
  printf("Found solution:\n");
  for(i = 0; i < height; i++)
  {
    for(j = 0; j < width; j++)
    {
      if(grid[i * width + j] == 0)
        printf(" %2d", bsf(poss[i * width + j]));
      else
        printf("   ");
    }

    printf("\n");
  }

  printf("\n");

  if(maxthreads > 1)
    pthread_mutex_unlock(&output_mutex);
}
