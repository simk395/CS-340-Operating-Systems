// Simon Kong
// g++ SKong_prj2_33_src.cpp -o SKong_prj2_33_src.out -pthread

// Sorry its messy, I didn't have time to refactor
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <math.h>

//mutex to set condition for synchronous thread to fix output issues
pthread_mutex_t lock;
int values[10];

//struct to pass arguments through thread_create
struct readParams{
    int id;
    int file;
    int offset[2];

};

// get number of digits in n
int nDigits(int n){
    // if n is 0 then the number becomes -inf
    if(n == 0){
        return 1;
    }

    return log10(n) + 1;
}

// converts int to char array
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

void* computeMin(void *param){

    // use lock for synchronous threading
    pthread_mutex_lock(&lock);

    struct readParams *params = (struct readParams*)param;
    // subtracting the two offsets will give the size of the buffer
    // also used to tell how much to read since params->offset[1] will give segfault
    // maybe change to size_t
    int SIZ = params->offset[1] - params->offset[0];
    char buf[SIZ];
    pread(params->file, buf, SIZ, params->offset[0]);

    int start = 0;
    int end = 0;
    int valStart = 0;
    int valEnd = 0;
    int digits = 0;
    int newDigits = 0;
    int number;

    for(int i = 0; i < SIZ; i++){
        // check for new line to indicate when a new integer is going to be read
        if(buf[i] == '\n'){
            end = i;

            // initialize variables from above
            // index: current starting point
            // digits: number of digits for min val
            // newDigits: number of digits for current val
            // valStart: index to where min val starts
            // valEnd: index to where min val ends
            // number: latest min val
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
    
    // converting ints to char arrays to be written
    values[params->id] = number;
    char out[] = "Worker Output:\t";
    char *threadId = toArray(params->id);
    char *outStart = toArray(params->offset[0]);
    char *outEnd = toArray(params->offset[1]);
    char *min = toArray(number);
    int startDigits = nDigits(params->offset[0]);
    int endDigits = nDigits(params->offset[1]);
    int idDigits = nDigits(params->id);

    // open existing file or create new file
    int fd = open("SKong_prj2_33_out.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
    // check if open failed
    if(fd == -1){
        char ERROR_MESSAGE[] = "Error. Failed to write.";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE));
        exit(2);
    }

    // write, I left it as zero because I think O_APPEND auto corrects for me???
    size_t write_offset = 0;
    pwrite(fd, out, sizeof(char)*15, write_offset);
    pwrite(fd, threadId, sizeof(char)*idDigits, write_offset);
    pwrite(fd, "\t", sizeof(char), write_offset);
    pwrite(fd, outStart, sizeof(char)*startDigits, write_offset);
    pwrite(fd, "\t", sizeof(char), write_offset);
    pwrite(fd, outEnd, sizeof(char)*endDigits, write_offset);
    pwrite(fd, "\t", sizeof(char), write_offset);
    pwrite(fd, min, sizeof(char)*digits, write_offset);
    pwrite(fd, "\n", sizeof(char), write_offset);

    // unlock thread for next thread to use
    pthread_mutex_unlock(&lock);

    close(fd);
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

        int fd2 = open("SKong_prj2_33_out.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
        // check if open failed
        if(fd2 == -1){
            char ERROR_MESSAGE[] = "Error. Failed to write.";
            write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE));
            exit(2);
        }

        // initialize variables
        // File_SIZE: size of the file
        // offset: current position being read
        // chunk: current partition
        // start: starting index of partition
        // lineCount: counts what line the program is on
        int FILE_SIZE = lseek(fd, -1, SEEK_END) + 1;
        int offset = 0;
        int chunk = 0;
        int start = 0;
        int lineCount = 0;
        char buf[1];
        struct readParams params;
        struct readParams workerData[10];
        
        // keep looping until file reaches the end
        while(offset != FILE_SIZE){
            // read at set position
            pread(fd, buf, 1, offset++);
            if(buf[0] == '\n'){
                lineCount++;
                // mod 1000 will detect if program is a line divisible by 1000
                if(lineCount != 0 && lineCount % 1000 == 0){
                    // assign values to struct variables
                    // every struct is one file chunk
                    params.id = chunk;
                    params.file = fd;
                    params.offset[0] = start;
                    params.offset[1] = offset;
                    // data for the file chunk is saved into an array
                    // this way I have 10 chunks in an array [chunk1, chunk2, chunk3]
                    workerData[chunk] = params;
                    
                    // parse integers to char array to be written
                    char out[] = "Main Output:\t";
                    char *outStart = toArray(start);
                    char *outEnd = toArray(offset);
                    char *threadId = toArray(chunk);
                    int startDigits = nDigits(start);
                    int endDigits = nDigits(offset);
                    int chunkDigits = nDigits(chunk);

                    //write to existing or new file
                    size_t write_offset = 0;
                    pwrite(fd2, out, sizeof(char)*13, write_offset);
                    pwrite(fd2, threadId, sizeof(char)*chunkDigits, write_offset);
                    pwrite(fd2, "\t", sizeof(char), write_offset);
                    pwrite(fd2, outStart, sizeof(char)*startDigits, write_offset);
                    pwrite(fd2, "\t", sizeof(char), write_offset);
                    pwrite(fd2, outEnd, sizeof(char)*endDigits, write_offset);
                    pwrite(fd2, "\n", sizeof(char), write_offset);
                    start = offset;
                    chunk++;
                }
            }
        }

        // create a pthread to be used for multithreading
        pthread_t workers[chunk];
        pthread_mutex_init(&lock, NULL);

        // create child threads to run
        for(int i = 0; i < chunk; i++){
            pthread_create(&workers[i], NULL, computeMin, &workerData[i]); 
        }

        // join them back together
        for(int i = 0; i < chunk; i++){
            pthread_join(workers[i], NULL);
        }
        // delete the mutex
        pthread_mutex_destroy(&lock);
        close(fd);
        // initialize min val as the first index then compare them
        int min = values[0];
        for(int i = 0; i < 10; i++){
            if(values[i] < min){
                min = values[i];
            }
        }

        char *globalMin = toArray(min);
        int minDigits = nDigits(min);
        size_t write_offset = 0;
        pwrite(fd2, "Main Output: The global minimum = \t", sizeof(char)*35, write_offset);
        pwrite(fd2, globalMin, sizeof(char)*minDigits, write_offset);
        pwrite(fd2, "\n", sizeof(char), write_offset);
        close(fd2);
        
        return 0;
}
