### Operating systems project 1

In this project, I developed a multi-process application that will perform matrix-vector multiplication for large matrices and vectors. More precisely, the application multiplies an nxn matrix M with a vector v of size n. 

The matrix M information is stored in an input text file (ascii). The vector v information is also stored in an input text file (ascii). We assume that the row-column coordinates of each matrix element is also is stored in the file. Hence for each nonzero matrix value we store a triple (i, j,m[i][j]) in a line of the matrixfile

The line format is:
<rownumber> <columnumber> <value>


The program will take following parameters:
```
mv matrixfile vectorfile resultfile K
```
In **mv**
The main process reads the matrixfile (which can be quite large) and partitions it into K splits (each split is a file). The partitioning is very simple. Assume there are L values (lines) in matrixfile. Then the first split will contain the first s = L/K (integer division, i.e., L div K) values from the matrixfile, the next split will contain the next s values, and so on. The last split may contain more than s, 2 which is fine (i.e, it can contain s + r values, where r is the remainder of L/K). The number of values L in the file can be obtained by first reading the file from beginning to end in the main process before generating the splits. After generating the split files, the main process creates K child processes to process (map) the split files. These child processes are mapper processes (mappers). Each mapper process processes another split file. Mapper
processes run and process their splits concurrently. The processing of a split file in a mapper process will be as follows. The mapper process first reads the whole vectorfile and puts the vector values into an array in main memory. If the vector is of size n, the array size will be n. Then the mapper starts reading its split file one value at a time (one line at a time). The value is be multiplied by the corresponding element in vector v and the result is added into the corresponding entry in a (partial) result array. If, for example, the triple read from a line is (i, j, m[i][j]), then m[i][j] is multiplied by v[j] and the multiplication result is added to the value at entry i of the partial result array. Each line of the split file is read and processed as described. In this way, the mapper creates a partial result for the multiplication. After mapper has finished with processing its split file, it writes out the partial result array into an intermediate output file, one result per line in the following format:

<rownumber> <value>

When all mappers finish, a reducer process (another child) is started by the main process. The reducer process opens and reads and processes the intermediate files produced by the mappers. It will read the intermediate files and will
sum up the respective values corresponding to the same vector index. At the end, the result vector will be obtained (after processing all input files). The result vector will be printed out to the resultfile in sorted order with respect to row numbers. The line format for the resultfile is be:
<rownumber> <value> 


In **mvp** I implement the same program this time using pipes instread of use of
intermediate files between mapper processes and reducer process. If there are K mappers, then there will be K pipes created. Each pipe is used for sending data from a mapper process to the reducer process. Each mapper puts data in a different pipe. Reducer process gets data from all pipes. The main process will generate split files. After split files are generated, the
child processes start working on then. No intermediate files will be generated.

In **mvt** I Implement the same program using threads this time. There will be K mapper threads and 1 reducer thread created.
Global variables (structures like arrays or linked lists) will be used to pass information between mapper threads and the reducer thread. Again the main thread will generate split files. After split files generated, mapper threads will start working on them. There will be no intermediate files. For this part, you can assume mappers and reducer will not access shared stuctures concurrently. Reducer thread can be started after mappers finish. 
