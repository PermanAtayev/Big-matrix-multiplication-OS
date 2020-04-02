#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <math.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <wait.h>


typedef struct thread_attr thread_attr;

#define N 19

int n, L, K, childProcessNumber, partialSize[19];
int splitSizes[19];

void textNameGenerator(char *splitTextName, char *number, char *bas){
  char extension[5] = ".txt";
  splitTextName[0] = '\0';

  strcat(splitTextName, bas);
  strcat(splitTextName, number);
  strcat(splitTextName, extension);
}


void splitMatrix(char *textName, int L, int K){
  FILE *matrixFile = fopen(textName, "r");

  int splitSize = L / K;
  char number[5], splitTextName[19];
  char basSplit[6] = "split";

  if(matrixFile){
    int j = 1;
    for(int i = 1; i <= K; i++){
      sprintf(number, "%d", i);
      textNameGenerator(splitTextName, number, basSplit);
      if(i == K && (L % K != 0)){
        FILE *fp = fopen(splitTextName, "w");
        // Place the biggest chunk in the first FILE
        //, in case the are not equal
        splitSizes[i] = L - j + 1;
        for(; j <= L; j++){
          int row = 0, val = 0, col = 0;
          fscanf(matrixFile, "%d %d %d", &row, &col, &val);
          fprintf(fp, "%d %d %d\n", row, col, val);
        }
        fclose(fp);
      }
      else{
        FILE *fp = fopen(splitTextName, "w");
        int currentLim = j + splitSize - 1;
        splitSizes[i] = currentLim - j + 1;
        for(; j <= currentLim; j++){
          int row = 0, val = 0, col = 0;
          fscanf(matrixFile, "%d %d %d", &row, &col, &val);
          fprintf(fp, "%d %d %d\n", row, col, val);
        }
        fclose(fp);
      }
      // clearing for the next name
      splitTextName[0] = '\0';
    }
  }
  else{
    printf("%s - File wasn't found", textName);
  }

  fclose(matrixFile);
}

void readerVector(char *textName, int *vector, int n){
  FILE *fp = fopen(textName, "r");
  for(int i = 1; i <= n; i++){
    int row = 0, val = 0;
    fscanf(fp, "%d %d", &row, &val);
    vector[row] = val;
  }
  fclose(fp);
}

void readerMatrix(char *textName, int n, int matrix[n][n]){
  FILE *fp = fopen(textName, "r");
  for(int i = 1; i <= n * n; i++){
    int row = 0, val = 0, col = 0;
    fscanf(fp, "%d %d %d", &row, &col, &val);
    matrix[row][col] = val;
  }
  fclose(fp);
}

void printMatrix(int n, int matrix[n][n]){
  for(int i = 1; i <= n; i++){
    for(int j = 1; j <= n; j++)
      printf("%d ", matrix[i][j]);
    printf("\n");
  }
}

void printVector(int n, int *vector){
  for(int i = 1; i <= n; i++)
    printf("%d ", vector[i]);
  printf("\n");
}

void writeVector(char *textName, int n, int *vector, int *visited){
  FILE *fp = fopen(textName, "w");
  for(int i = 1; i <= n; i++){
    if(visited[i] == 1){
      fprintf(fp, "%d %d\n", i, vector[i]);
    }
  }
  fclose(fp);
}

void writeResult(char *textName, int n, int *result){
  FILE *fp = fopen(textName, "w");
  for(int i = 1; i <= n; i++)
      fprintf(fp, "%d %d\n", i, result[i]);
  fclose(fp);
}


int sizeOfTheText(char *textName){
  int size = 0;
  FILE *fp = fopen(textName, "r");
  char c = getc(fp);
  while(!feof(fp)){
    c = fgetc(fp);
    if(c == '\n')
      size++;
  }
  fclose(fp);
  return size;
}

void processSplit(char *textName, int *vector, int * partial,
                  int *visited, int m){
  FILE *fp = fopen(textName, "r");
  for(int i = 1; i <= m; i++){
    int row = 0, val = 0, col = 0;
    fscanf(fp, "%d %d %d", &row, &col, &val);
    visited[row] = 1;
    partial[row] += (val * vector[col]);
  }
  fclose(fp);
}

void processPartial(char *textName, int *result, int n){
  FILE *fp = fopen(textName, "r");
  for(int i = 1; i <= n; i++){
    int row, val;
    fscanf(fp, "%d %d", &row, &val);
    result[row] += val;
  }
  fclose(fp);
}

struct thread_attr{
  int vectorSize, *splitSizes, id, K;
  char *textName;
};

void *threadProcess(void *threadAttributes){
  thread_attr attrs = (*((struct thread_attr *)threadAttributes));
  int n = attrs.vectorSize, id = attrs.id, m = (attrs.splitSizes)[id];
  char *vectorTextName = attrs.textName;

  char splitTextName[19], number[5], partialTextName[19];
  char basSplit[6] = "split", basPartial[9] = "partial";
  sprintf(number, "%d", id);
  textNameGenerator(splitTextName, number, basSplit);

  int vector[n + 1];
  memset(vector, 0, sizeof vector);
  readerVector(vectorTextName, vector, n);

  // We would need all locations, because we don't know
  // about the order of the input
  int partial[n + 1], visited[n + 1];
  memset(partial, 0, sizeof partial);
  memset(visited, 0, sizeof visited);

  // printf("In the thread number %d and split size %d\n", id, m);
  processSplit(splitTextName, vector, partial, visited, m);
  textNameGenerator(partialTextName, number, basPartial);
  writeVector(partialTextName, n, partial, visited);

  return NULL;
}

void *reducerProcess(void *reducerAttributes){
  thread_attr attrs = (*((struct thread_attr *)reducerAttributes));
  int n = attrs.vectorSize, K = attrs.K;
  char *resultTextName = attrs.textName;

  int result[n + 1];
  memset(result, 0, sizeof result);
  char number[5], partialTextName[19], basPartial[9] = "partial";

  for(int i = 1; i <= K; i++){
    sprintf(number, "%d", i);
    textNameGenerator(partialTextName, number, basPartial);
    int m = sizeOfTheText(partialTextName);
    processPartial(partialTextName, result, m);
  }

  writeResult(resultTextName, n, result);

  return NULL;
}

/*
./mvt matrixfile.txt vectorfile.txt result.txt 3
*/

int main(int argc, char *argv[]){
  printf("mvt started\n");
  clock_t t = clock();

  char matrixTextName[109] = "\0";
  char vectorTextName[109] = "\0";
  char resultTextName[109] = "\0";

  strcpy(matrixTextName, argv[1]);
  strcpy(vectorTextName, argv[2]);
  strcpy(resultTextName, argv[3]);
  K = atoi(argv[4]);

  // K = 2;
  // char matrixTextName[] = "matrixfile.txt";
  // char vectorTextName[] = "vectorfile.txt";
  // char resultTextName[] = "result.txt";

  L = sizeOfTheText(matrixTextName);
  n = sizeOfTheText(vectorTextName);

  splitMatrix(matrixTextName, L, K);

  // the processes Part starts here
  pthread_t threads[K + 1];
  thread_attr threadAttributes[K + 1];

  for(int i = 1; i <= K; i++){
      // I can't directly use the variable i's address here, because it might increment before I get to the runner function
      threadAttributes[i] = (thread_attr){.vectorSize = n, .splitSizes = splitSizes, .id = i, .K = K, .textName = vectorTextName};
      pthread_create(&threads[i], NULL, threadProcess, &threadAttributes[i]);
  }
  // wait until threads are done
  for(int i = 1; i <= K; i++){
    pthread_join(threads[i], NULL);
  }


  // Reducer is going to join all parts to the one file
  pthread_t reducer;
  thread_attr reducerAttributes;

  reducerAttributes = (thread_attr){.vectorSize = n, .splitSizes = splitSizes, .id = 0, .K = K, .textName = resultTextName};
  pthread_create(&reducer, NULL, reducerProcess, &reducerAttributes);
  pthread_join(reducer, NULL);


  t = clock() - t;
  double time_taken = ((double)t)/CLOCKS_PER_SEC;
  printf("The program took %f seconds to execute mvt with N = %d\n", time_taken, n);
  printf("mvt ended\n");
}
