// Simon Kong
// g++ SKong_prj2_33_src_part_2.cpp -o SKong_prj2_33_src_part_2.out -pthread

// a lot of it was copy and pasted from part 1, I suggest seeing part 1 first and its documentation first

#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>
#include <semaphore.h>
#define BUFFER_SIZE 5

// semaphores for data synchronization. I'm not really sure if I was allowed to use this.
// If I'm not then, I would've replaced it with mutex locks and conditional variables (pthread_cond_wait, etc).

sem_t semProduce;
sem_t semConsume;
sem_t semProduce2;
sem_t semConsume2;

// struct for shared memory structure
struct sharedMemory{
    int id;
    int begin;
    int end;
};

//I didn't know what it meant by shared buffer array so, I just thought of where threads can share memory.
// My first thought was the heap so, I made some global variables.
struct sharedMemory buf1[BUFFER_SIZE];
int buf2[BUFFER_SIZE];
bool CONDITION = true;

//gets number of digits from an int
int nDigits(int n){
    // if n is 0 then the number becomes -inf
    if(n == 0){
        return 1;
    }

    return log10(n) + 1;
}

// parse int to a char *array
char* toArray(int number){
    if(number == 0){
        char* numberArray = new char[1];
        numberArray[0] = '0';
        return numberArray;
    }
    int n = nDigits(number);
    // initialize a char array using calloc with n size * sizeof characters for storing the digits
    // in this case it is length of n (number of digits) * char size for machine (mine is 1 byte)
    char *numberArray = (char*) calloc(n, sizeof(char));
    // going right to left of int n, take the first digit
    // place first digit into the last index
    // % 10 will get the digit on the very right
    // / 10 to remove the digit and then repeat
    // + 0 is 0-9 + ascii code 48 which gives '0' - '9'
    for (int i = n-1; i >= 0; --i, number /= 10){
        numberArray[i] = (number % 10) + '0';
    }
    return numberArray;
}

// converts portion of char array to integer
int toInteger(int start, int end, int d, char buf[]){
    int number = 0;
    int dec = 1;
    // multiply 10 by digit amount to get greatest decimal place
    for(int i = 1; i < d; i++){
        dec *= 10;
    }

    // iterate array left to right and multiply value by dec to be summed into number to create an integer representing a single line
    // from the text document
    for(start; start < end; start++){
        int val = buf[start] - '0';
        number += val*dec;
        dec /= 10;
    }
    return number;
}

// gathers all offsets and file chunks
void* thr(void* param){
    int *fd = (int *)param;

    int fd2 = open("SKong_prj2_33_out.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
    // check if open failed
    if(fd2 == -1){
        char ERROR_MESSAGE[] = "Error. Failed to write.";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE));
        exit(2);
    }
    
    // initialize some variables; copy and pasted of part 1
    int FILE_SIZE = lseek(*fd, -1, SEEK_END) + 1;
    int offset = 0;
    int chunkId = 0;
    int start = 0;
    int lineCount = 0;
    int count = 0;
    char localBuf[1];
    struct sharedMemory data;
    
    while(offset != FILE_SIZE){
        pread(*fd, localBuf, 1, offset++);
        if(localBuf[0] == '\n'){
            lineCount++;
            if(lineCount != 0 && lineCount % 1000 == 0){
                // decrement my semaphore's counter to get closer to the limit
                // unlike in part 1 where the code can just go through as long as it has access through the mutex lock
                // the semaphore allows x amount of insertions before the thread gets blocked temporarily
                sem_wait(&semProduce);
                data.id = chunkId;
                data.begin = start;
                data.end = offset;
                buf1[count] = data;
                start = offset;

                // parse integers to char array to be written
                char *outStart = toArray(start);
                char *outEnd = toArray(offset);
                char *threadId = toArray(chunkId);
                int startDigits = nDigits(start);
                int endDigits = nDigits(offset);
                int chunkDigits = nDigits(chunkId);

                //write to existing or new file
                size_t nBytes;
                pwrite(fd2, "Pthread 1 Output:\t", sizeof(char)*18, nBytes);
                pwrite(fd2, threadId, sizeof(char)*chunkDigits, nBytes);
                pwrite(fd2, "\t", sizeof(char), nBytes);
                pwrite(fd2, outStart, sizeof(char)*startDigits, nBytes);
                pwrite(fd2, "\t", sizeof(char), nBytes);
                pwrite(fd2, outEnd, sizeof(char)*endDigits, nBytes);
                pwrite(fd2, "\n", sizeof(char), nBytes);

                // increment the consume sem so that the data can be read by thread 2
                sem_post(&semConsume);

                count++, chunkId++;
                //reset
                if(count >= BUFFER_SIZE){
                    count = 0;
                }
            }
        }
    }
    CONDITION = false;
    close(fd2);
    pthread_exit(0);

}

// gathers all local minimum values
// initialize variables from above
// index: current starting point
// digits: number of digits for min val
// newDigits: number of digits for current val
// valStart: index to where min val starts
// valEnd: index to where min val ends
// number: latest min val
void* thr2(void* param){
    int *fd = (int *)param;
    int count = 0;
    int number;

    // open existing file or create new file
    int fd2 = open("SKong_prj2_33_out.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
    // check if open failed
    if(fd2 == -1){
        char ERROR_MESSAGE[] = "Error. Failed to write.";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE));
        exit(2);
    }

    while(CONDITION){
        // was incremented by thread 1. Now thread 2 can check for data. Without the sem, the data may not have arrived yet.
        sem_wait(&semConsume);

        // read the data; mostly copy and pasted from part 1
        int SIZ =  buf1[count].end - buf1[count].begin;
        char buf[SIZ];
        pread(*fd, buf, SIZ, buf1[count].begin);

        int start = 0;
        int end = 0;
        int valStart = 0;
        int valEnd = 0;
        int digits = 0;
        int newDigits = 0;

        for(int i = 0; i < SIZ; i++){
            // check for new line to indicate when a new integer is going to be read
            if(buf[i] == '\n'){
                end = i;

                if(digits == 0){
                    int index = end-start;
                    digits = index;
                    newDigits = index;
                    valStart = start;
                    valEnd = end;
                    number = toInteger(start, end, digits, buf);
                }
                else{
                    newDigits = end-start;
                }

                //if the number of digits is less then automatically replace
                if(newDigits < digits){
                    digits = newDigits;
                    valStart = start;
                    valEnd = end;
                    number = toInteger(start, end, digits, buf);
                }
                //if number of digits are equal then check the digits of the numbers being compared
                else if(newDigits == digits){
                    
                    for(int j = start; j < end; j++){
                        int val = valStart;

                        // if the min val digit is lower then ignore the new number
                        if(buf[val] < buf[j]){
                            break;
                        }
                        // if the min val is equal then continue
                        else if(buf[j] == buf[val]){
                            val++;
                            continue;
                        }
                        // else replace the min val because it has to be higher
                        valStart = start;
                        valEnd = end;
                        number = toInteger(start, end, digits, buf);
                        
                    }
                }
                start = i+1;
            }
        }

        // parse to char array
        char *numberArr = toArray(number);
        char *idArr = toArray(buf1[count].id);
        char *beginArr = toArray(buf1[count].begin);
        char *endArr = toArray(buf1[count].end);
        int d = nDigits(number);
        int idDigits = nDigits(buf1[count].id);
        int beginDigits = nDigits(buf1[count].begin);
        int endDigits = nDigits(buf1[count].end);

        // write into file
        size_t nBytes;
        pwrite(fd2, "Pthread 2 Output:\t", sizeof(char)*18, nBytes);
        pwrite(fd2, idArr, sizeof(char)*idDigits, nBytes);
        pwrite(fd2, "\t", sizeof(char), nBytes);
        pwrite(fd2, beginArr, sizeof(char)*beginDigits, nBytes);
        pwrite(fd2, "\t", sizeof(char), nBytes);
        pwrite(fd2, endArr, sizeof(char)*endDigits, nBytes);
        pwrite(fd2, "\t", sizeof(char), nBytes);
        pwrite(fd2, numberArr, sizeof(char)*d, nBytes);
        pwrite(fd2, "\n", sizeof(char), nBytes);

        // sem for thread 3 to wait for data in buffer 2
        sem_wait(&semProduce2);
        buf2[count] = number;
        // tell thread 1 it can put more data in
        sem_post(&semProduce);

        count++;
        if(count > 4){
            count = 0;
        }
        //tell thread 3 it can check for data
        sem_post(&semConsume2);
    }
    close(fd2);
    pthread_exit(0);
}

// gathers global minimum value
void* thr3(void* param){
    int *fd = (int *)param;
    int count = 0;
    bool chk = true;
    int min;
    while(CONDITION){
        // tell thread 2 its going to start
        sem_wait(&semConsume2);
        if(chk && count == 0){
            min = buf2[count];
            chk = false;
        }
        
        // check current min to val in buffer if lower then replace
        if(min > buf2[count]){
            min = buf2[count];
        }
    
        count++;
        if(count > 4){
            count = 0;
        }
        // tell thread 2 it finished
        sem_post(&semProduce2);
    }

    // open existing file or create new file
    int fd2 = open("SKong_prj2_33_out.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
    // check if open failed
    if(fd2 == -1){
        char ERROR_MESSAGE[] = "Error. Failed to write.";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE));
        exit(2);
    }

    char *minArr = toArray(min);
    int d = nDigits(min);

    // write to file
    size_t nBytes;
    pwrite(fd2, "Pthread 3 Output: The current global minimum = \t", sizeof(char)*48, nBytes);
    pwrite(fd2, minArr, sizeof(char)*d, nBytes);
    pwrite(fd2, "\n", sizeof(char), nBytes);

    close(fd2);
    pthread_exit(0);
}

int main(int argc, const char *argv[]){
    int fd;
    
    // check for argument
    if (argc != 2){
        const char ERROR_MESSAGE[] = "Wrong number of command line arguments\n";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE)-1);
        return 1;
    }

    // open and check file in argument
    if ((fd = open(argv[1], O_RDONLY, 0777)) == -1){
        const char ERROR_MESSAGE[] = "Can't open file.";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE)-1);
        return 2;
    }

    // create 3 threads
    pthread_t pthr, pthr2, pthr3;

    // initialize semaphores
    sem_init(&semProduce, 0, BUFFER_SIZE);
    sem_init(&semConsume, 0, 0);
    sem_init(&semProduce2, 0, BUFFER_SIZE);
    sem_init(&semConsume2, 0, 0);

    // create 3 threads
    pthread_create(&pthr, NULL, thr, &fd);
    pthread_create(&pthr2, NULL, thr2, &fd);
    pthread_create(&pthr3, NULL, thr3, &fd);

    // join the 3 threads
    pthread_join(pthr, NULL);
    pthread_join(pthr2, NULL);
    pthread_join(pthr3, NULL);

    // destroy semaphores
    sem_destroy(&semProduce);
    sem_destroy(&semProduce2);
    sem_destroy(&semConsume);
    sem_destroy(&semConsume2);

    // close file being read
    close(fd);
    return 0;
}