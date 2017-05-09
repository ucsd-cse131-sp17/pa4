#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern int  our_code_starts_here() asm("our_code_starts_here");
extern void error()                asm("error");
extern int  print(int val)         asm("print");
extern int  input(int i)           asm("input");

const int TRUE  = 0xFFFFFFFF;
const int FALSE = 0x7FFFFFFF;

int  INPUT_COUNT = 0;
int* INPUTS      = NULL;

const long int INT_MIN = - (1 << 30);
const long int INT_MAX = (1 << 30) - 1;

void print_simple(int val) {
  if((val & 0x00000001) ^ 0x00000001) {
    printf("%d", val >> 1);
  }
  else if(val == 0xFFFFFFFF) {
    printf("true");
  }
  else if(val == 0x7FFFFFFF) {
    printf("false");
  }
  else {
    fprintf(stderr, "TODO: print_simple @ main.c\n");
    exit(1);

    printf("Unknown value: %#010x", val);
  }
}

int print(int val) {
  print_simple(val);
  printf("\n");
  return val;
}

void error(int i) {
  if (i == 1) {
    fprintf(stderr, "Error: expected a number");
  }
  else if (i == 2) {
    fprintf(stderr, "Error: expected a boolean");
  }
  else if (i == 3) {
    fprintf(stderr, "Error: Integer overflow");
  }
  else if (i == 4) {
    fprintf(stderr, "Error: expected a tuple");
  }
  else if (i == 5) {
    fprintf(stderr, "Error: index too small");
  }
  else if (i == 6) {
    fprintf(stderr, "Error: index too large");
  }
  else {
    fprintf(stderr, "Error: Unknown error code: %d\n", i);
    exit(1);
  }

  exit(i);
}

int input(int i) {
  i = i >> 1;

  if (i < 0 || i >= INPUT_COUNT) {
    fprintf(stderr, "input index out of bounds (given:%d #args:%d) \n", i, INPUT_COUNT);
    exit(1);
  }
  
  return INPUTS[i];
}

int parse_input(const char* in) {
  if (strcmp(in, "true") == 0) {
    return TRUE;

  } else if (strcmp(in, "false") == 0) {
    return FALSE;

  } else {
    size_t l = strlen(in);
    if (l == 0) {
      fprintf(stderr, "input is empty\n");
      exit(1);
    }
      
    char* endptr = (char*) &in[l];
    long int r = strtol(in, &endptr, 10);
    
    if (*endptr != '\0') {
      fprintf(stderr, "input '%s' is not a number or a boolean\n", in);
      exit(1);
    }

    if (r < INT_MIN || r > INT_MAX) {
      fprintf(stderr, "input '%s' is not a representable number\n", in);
      exit(1);
    }
      
    return (int) r * 2;
  }
}

int main(int argc, char** argv) {
  INPUT_COUNT = argc - 1;
  INPUTS = calloc(INPUT_COUNT, sizeof(int));

  int i = 0;
  for (; i < argc - 1; i++) {
    INPUTS[i] = parse_input(argv[i+1]);
  }
  
  int* HEAP = calloc(100000, sizeof (int));

  int result = our_code_starts_here(HEAP);
  print(result);
  
  return 0;
}

