# PMC

This code used the POSIX Threads(pthreads) , semaphore to control access to shared resources. There are three different functions such as producer, consumer, middleman. The role of producer is that it generate the random number to the buffer A until A is full. The role of middleman is to deliver the number from buffer A to buffer B. The role of consumer is to consume the number in the buffer B.

### Keys
The size of buffer is 5 * sizeof(int).
```c
size_t size = 5 * sizeof(int);
int *A = (int*) malloc(size);
int *B = (int*) malloc(size);
```

Threads has attributes as below.
Producers insert function to buffer A.
Middlemans read the buffer A, insert number in A to buffer B, remove number in A.
Consumers remove the number in buffer B.


### outputs
A[13][18][0][0][0], in = 2, out = 0 queue = Producer
A[0][18][0][0][0], in = 2, out = 1 queue = Middleman
B[13][0][0][0][0], in = 1, out = 0 queue = Middleman
B[0][0][0][0][0], in = 1, out = 1 queue = Consumer
where in, out are the pointer for the index of buffer.
