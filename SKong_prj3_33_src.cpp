// Simon Kong
// g++ SKong_prj3_33_src.cpp -o SKong_prj3_33_src.out -pthread
// or
// g++ SKong_prj3_33_src.cpp -o SKong_prj3_33_src.exe -pthread

#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <stack>
#include <math.h>

pthread_mutex_t lock;
pthread_cond_t cond;
std::stack<int> intStack;
int minArr[10];

int nDigits(int n){
    // if n is 0 then the number becomes -inf
    if(n == 0){
        return 1;
    }

    return log10(n) + 1;
}

// parse array to integer
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

// parse char to integer
int toInteger(int d, char num[]){
    int number = 0;
    // this gives me decimal place to start
    int dec = pow(10, d-1);

    // put char into proper decimal place
    for(int i = 0; i < d; i++){
        int val = num[i] - '0';
        number += val*dec;
        dec /= 10;
    }

    return number;
}

// function for thread 1 that pushes integers into the stack
void* producer(void* param){
// initialize variables
    int *fd = (int *)param;
    int FILE_SIZE = lseek(*fd, -1, SEEK_END) + 1;
    int offset = 0;
    int start = 0;
    int end = 0;
    char buf[1];

    // read from 0 to EOF
    while(offset != FILE_SIZE){
        // setting end to most recently read char
        end = offset;
        pread(*fd, buf, 1, offset++);
        
        if(buf[0] == '\n'){

            // if the size of the stack is greater than 100 it breaks requirements so,
            // let other threads read to lower the stack size. This way it never exceeds 100;
            if(intStack.size() >= 100){
                pthread_yield();
            }

            // subtract the offsets will give the amount of decimal places because
            // it is reading 1 char at a time
            int digits = end - start;
            if(digits == 0){
                continue;
            }

            // block all other threads using same lock
            pthread_mutex_lock(&lock);
            char numArr[digits];

            // reiterate through the line to pass the char into a char array
            for (int i = 0; i < digits; i++){
                pread(*fd, buf, 1, start++);
                numArr[i] = buf[0];
                
            }

            // parse the char array into an integer to be pushed into the stack
            int val = toInteger(digits, numArr);
            intStack.push(val);
            // reset the starting point for the next integer
            start = offset;
            pthread_mutex_unlock(&lock);
            pthread_cond_signal(&cond);
        }
    }

    pthread_exit(0);
}

// thread 2 function that reads the stack to push current global min after every 1000 integers
void* consumer(void* param){
    // initialize
    int count = 1;
    int min = 0;
    int index = 0;
    bool flag = true;

    pthread_mutex_lock(&lock);
    // after the global array is full it needs to stop
    while(index != 10){
        // if the stack is empty it will cause a segfault when checking the top.
        // the program needs to check if the it is empty.
        // if it is empty then it can unlock the mutex and wait for when there is data in the stack.
        // Once it is signaled that there is data in the stack it can progress.
        if(intStack.empty()){
            pthread_cond_wait(&cond, &lock);
        }

        // if the stack isnt empty, then check the top of the stack.
        if(!intStack.empty()){
            int val = intStack.top();

            // initialize min to the first integer being read 
            if(flag){
                min = val;
                flag = false;
            }

            // if the value from the stack is lower than the min
            // then it is safe to replace min with the current val
            if(val < min){                
                min = val;
            }

            // count is a variable that check how many integers the program has gone through
            // if the count is modded by 1000 is 0 then that means 1000 integers have gone by
            // it is time to add the current min to the global array then
            if( (count % 1000) == 0 ){
                minArr[index] = min;
                index++;
            }
            
            intStack.pop();
            count++;
        }
        
    }
    
    pthread_mutex_unlock(&lock);
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

        // initialize pthreads, locks, and conditions
        pthread_t thr, thr2; 
        pthread_mutex_init(&lock, NULL);
        pthread_cond_init(&cond, NULL);
        
        // create the thread
        pthread_create(&thr, NULL, producer, &fd);
        pthread_create(&thr2, NULL, consumer, NULL);

        // collect them back
        pthread_join(thr, NULL);
        pthread_join(thr2, NULL);

        // destroy the mutex and cond
        pthread_mutex_destroy(&lock);
        pthread_cond_destroy(&cond);
        
        // close the file being read
        close(fd);

        // open existing file or create new file
        fd = open("SKong_prj3_33_out.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
        // check if open failed
        if(fd == -1){
        char ERROR_MESSAGE[] = "Error. Failed to write.";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE));
        exit(2);
    }

        // the loops iterates over the global min array to be written into a text file
        for(int i = 0; i < 10; i++){
            
            // since every index represents 1000 integers, some arithmetic can fix the number
            int line = (i + 1)*1000;
            char* integer = toArray(minArr[i]);
            char* iterator = toArray(line);
            int dm = nDigits(minArr[i]);
            int di = nDigits(line);
            write(fd, "Minimum integer after ", sizeof(char)*22);
            write(fd, iterator, sizeof(char)*di); 
            write(fd, " integers = \t", sizeof(char)*13);
            write(fd, integer, sizeof(char)*dm);
            write(fd, "\n", sizeof(char)); 

        }

        return 0;
}