#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include <math.h>
#include <sys/types.h>
#include <stdbool.h>
#include <time.h>
#include <wait.h>
#include <semaphore.h>
#include <pthread.h>


typedef struct thread_attr thread_attr;
typedef struct pair pair;
typedef struct circular_buffer circular_buffer;

#define N 19

sem_t mutex;
int n, L, K, B, childProcessNumber, partialSize[19];
int splitSizes[19];

struct pair_int{
  int row, col, val;
} thread_buffer[19][10009];

struct circular_buffer{
  int writeID, readID, bufferLength;
} bufferProp[19];

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
  int row, val;
  for(int i = 1; i <= n; i++){
      row = val = 0;
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
    if(visited[i] == 1)
      fprintf(fp, "%d %d\n", i, vector[i]);
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

void processSplit(char *textName, int m, int id){
  FILE *fp = fopen(textName, "r");
  int row, val, col;
  for(int i = 1; i <= m; i++){
    // wait until the buffer has some space;
    while(bufferProp[id].bufferLength == B);

    sem_wait(&mutex);
    row = val = col = 0;
    fscanf(fp, "%d %d %d", &row, &col, &val);

    thread_buffer[id][bufferProp[id].writeID].row = row;
    thread_buffer[id][bufferProp[id].writeID].col = col;
    thread_buffer[id][bufferProp[id].writeID].val = val;

    bufferProp[id].writeID++;
    bufferProp[id].bufferLength++;

    if(bufferProp[id].writeID == B)
      bufferProp[id].writeID = 0;

    // printf("Thread %d wrote to the buffer %dth line to %d-id(%d,%d) val=%d\n", id, bufferProp[id].bufferLength, bufferProp[id].writeID, row, col, val);
    sem_post(&mutex);
  }

  // wait until the reducer is done with this thread's files.
  while(bufferProp[id].bufferLength > 0);

  // this means that the thread id is now done, and it needs not to be checked
  // printf("Reducer is done with Thread %d\n", id);
  bufferProp[id].bufferLength = -1;

  fclose(fp);
}

struct thread_attr{
  int vectorSize, *splitSizes, id, K;
  char *textName1, *textName2;
};

void *threadProcess(void *threadAttributes){
  thread_attr attrs = (*((struct thread_attr *)threadAttributes));
  int id = attrs.id, m = (attrs.splitSizes)[id];
  char splitTextName[19], number[5];
  char basSplit[6] = "split";
  sprintf(number, "%d", id);
  textNameGenerator(splitTextName, number, basSplit);
  processSplit(splitTextName, m, id);
  // printf("Thread %d is done\n", id);
  return NULL;
}

void *reducerProcess(void *reducerAttributes){
  thread_attr attrs = (*((struct thread_attr *)reducerAttributes));
  int n = attrs.vectorSize, K = attrs.K;
  char *resultTextName = attrs.textName1;
  char *vectorTextName = attrs.textName2;
  int result[n + 1], row, col, val, index;
  int vector[n + 1];
  memset(result, 0, sizeof result);
  memset(vector, 0, sizeof vector);
  readerVector(vectorTextName, vector, n);
  int done = 0;
  while(done != 1){
    sem_wait(&mutex);
    done = 1;
    // check whether the are some mappers left that aren't done with writing
    for(int i = 1; i <= K; i++){
      if(bufferProp[i].bufferLength != -1){
        done = 0;
        break;
      }
    }
    for(int i = 1; i <= K; i++){
      if(bufferProp[i].bufferLength > 0){
        index = bufferProp[i].readID;
        row = thread_buffer[i][index].row;
        col = thread_buffer[i][index].col;
        val = thread_buffer[i][index].val;
        result[row] += (val * vector[col]);

        // printf("Reducer read %dth line from %dth thread at (%d, %d) with val = %d\n", index + 1, i, row, col, val);

        bufferProp[i].bufferLength--;
        bufferProp[i].readID++;

        // to make the buffer circular
        if(bufferProp[i].readID == B){
          bufferProp[i].readID = 0;
        }
        // breaking to read only one line from the buffer, if there are other mappers that are waiting to produce
        break;
      }
    }
    sem_post(&mutex);
  }
  writeResult(resultTextName, n, result);
  return NULL;
}

/*
./mvt matrixfile.txt vectorfile.txt result.txt 3
./mvt_s ./test/testMatrix.txt ./test/testVector.txt result.txt 3 2
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
  B = atoi(argv[5]);

  // K = 3;
  // B = 2;
  // char matrixTextName[] = "./test/testMatrix1.txt";
  // char vectorTextName[] = "./test/testVector1.txt";
  // char resultTextName[] = "result.txt";

  for(int j = 1; j <= K; j++){
    bufferProp[j].writeID = bufferProp[j].readID = bufferProp[j].bufferLength = 0;

    for(int i = 0; i <= B; i++){
      thread_buffer[j][i].row = 0;
      thread_buffer[j][i].col = 0;
      thread_buffer[j][i].val = 0;
    }
  }


  L = sizeOfTheText(matrixTextName);
  n = sizeOfTheText(vectorTextName);

  splitMatrix(matrixTextName, L, K);

  // the processes Part starts here
  pthread_t threads[K + 1];
  thread_attr threadAttributes[K + 1];
  pthread_t reducer;
  thread_attr reducerAttributes;

  sem_init(&mutex, 0, 1);

  for(int i = 1; i <= K; i++){
      threadAttributes[i] = (thread_attr){.vectorSize = n, .splitSizes = splitSizes, .id = i, .K = K, .textName1 = vectorTextName, .textName2 = resultTextName};
      pthread_create(&threads[i], NULL, threadProcess, &threadAttributes[i]);
  }
  reducerAttributes = (thread_attr){.vectorSize = n, .splitSizes = splitSizes, .id = 0, .K = K, .textName1 = resultTextName, .textName2 = vectorTextName};
  pthread_create(&reducer, NULL, reducerProcess, &reducerAttributes);

  // wait until threads are done
  for(int i = 1; i <= K; i++){
    pthread_join(threads[i], NULL);
  }
  pthread_join(reducer, NULL);

  t = clock() - t;
  double time_taken = ((double)t)/CLOCKS_PER_SEC;
  sem_destroy(&mutex);

  printf("The program took %f seconds to execute mvt with N = %d\n", time_taken, n);
  printf("mvt ended\n");
}
/*
Issues:
2. Lots of useless code from the first homework that needs to be removed
3. Understand exactly what you do and see if it can be improved.
*/
