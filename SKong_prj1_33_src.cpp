// Simon Kong
// g++ -o Skong_prj1_33_src.exe SKong_prj1_33_src.cpp
// or
// g++ -o Skong_prj1_33_src.out SKong_prj1_33_src.cpp

#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <algorithm>

// macro get number of digits from int. log10 ranges from (digits-1) to digits then + 1.
#define nDigits(n) log10(n) + 1;

// converts int to char array
char* toArray(int number){
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

// function that takes in the following as arguments: file description, buffer, position, and position array.
// position is passed as reference so it can be remembered when reusing the function
// Logic: The buffer array can only hold 32 chars so, I will reassign elements multiple times.
// Grab as many chars as it can hold while also comparing chars
// Using an int that is passed by reference, the position can be saved since function can only return one value
// int pointer array is used to modify elements inside
// elements are saved on the first unused index and the last unused index ex: [a,b, _, c,d] for easier access.
void readfile(int fd, char buf[], int &pos, int posArr[]){
    int n;
    // i = 2 because I use first two indexes for input chars
    for(int i = 2; i < 16; pos++){
        // if nothing to read or error then return
        // sizeof(char) because char can be different sizes
        if(n = (read(fd, buf, sizeof(char))) <= 0){
            return;
        }
        // save the value into the 2nd index
        buf[1] = *buf;
        // go to the end of file
        lseek(fd, -pos, SEEK_END);
        // read char at the end
        read(fd, buf, sizeof(char));

        // check for chars that cant be typed, unsure if needed to do this
        bool chkBuf = (buf[0] > 31 && buf[0] != 95) && (buf[1] > 31 && buf[1] != 95);
        if( (buf[1] != *buf) && chkBuf ){
            // save one front of the array and one at the back
            buf[i] = buf[1];
            buf[-i] = *buf;
            // save the position of the char
            // here I am saving every position but its possible to cut it by half by doing
            // midpoint*2 I believe. I dont rememebr anymore
            posArr[pos-1] = pos;
            i++;
            // print 1 to screen
            // I don't know if I'm suppose to write this to output file
            write(1, "1", sizeof(char));
        }else{
            // print 0 to screen
            write(1, "0", sizeof(char));
        }

        // set position to the next char at the front of the file
        lseek(fd, pos, SEEK_SET);     
    }
}

// write the results to both text file and terminal
void writeResult(int fd, int count){
        char total[] = "Total:";
        char result[] = " character positions differ.\n";
        char* n = toArray(count);  
        int d = nDigits(count);

        // print to terminal
        write(1, total, sizeof(total));
        write(1, n, sizeof(char)*d);
        write(1, result, sizeof(result));

        // write to file
        write(fd, total, sizeof(total));
        write(fd, n, sizeof(char)*d);
        write(fd, result, sizeof(result));
}

// Logic: take the strings and positions and concat them together using write
void writefile(char out[][6], int pos[], int size, int count){
    // base case if empty file
     if(count == 0){
        char msg[] = "Total: 0 character positions differ.";
        write(1, msg, sizeof(msg));
        exit(1);
    }

    // open txt file or create if it doesnt exist. Append to the last input.
    int fd = open("SKong_prj1_33_out.txt", O_RDWR | O_CREAT | O_APPEND, 0777);
    // check if open failed
    if(fd == -1){
        char ERROR_MESSAGE[] = "Error. Failed to write.";
        write(1, ERROR_MESSAGE, sizeof(ERROR_MESSAGE));
        exit(2);
    }

    int i, j;
    for(i = 0, j = 0; i < size; i++){
        // check if there is not match on that index
        // check if j passed the number of matches so it doesnt go out of bounds
        if(pos[i] == 0){
            continue;
        } else if(j > count){
            break;
        }
        // number of digits will be used to tell how many bytes to write
        int d = nDigits(pos[i]);
        // since int is not writable with write() I will convert to char by
        char* n = toArray(pos[i]);
        // write position number to file and terminal
        write(1, n, sizeof(char)*d);
        write(fd, n, sizeof(char)*d);
        // write string to file and terminal
        write(1, out[j], sizeof(char)*5);
        write(fd, out[j], sizeof(char)*5);
        j++;
    }

    // call writeresult to tell how many differences 
    writeResult(fd, count);
    close(fd);

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
    
    // count: number of mismatches, File_size: size of file being read, pos: position of index, 
    // posArr: holds positions, output: 2d array that holds strings of mismatched char, chk: flag
    int count = 0;
    int FILE_SIZE = lseek(fd, -1, SEEK_END) + 1;
    int SIZ = FILE_SIZE*5;
    int pos = 1;
    int* posArr = new int[FILE_SIZE]();
    char* buf = new char[32]();
    char output[SIZ][6];
    bool chk = false;
    lseek(fd, 0, SEEK_SET);

    // Logic:
    // 1) If current position is not the end of the file then loop
    // 2) Call readfile to readfile and get array of chars from the file
    // 3) loop half the size of the buffer (can be made dynamic). Half because im iterating in pairs.
    // 4) create the strings for the output and save them in an index of the 2d array
    // 5) repeat until EOF
    // It is possible to do FILE_SIZE/2 and stop at the point of the file afterwards save in forward and reverse orders.
    // At the front and back of the 2d array. See drawing in document.
    while(lseek(fd, 0, SEEK_CUR) != FILE_SIZE){
        // call readfile
        readfile(fd, buf, pos, posArr);
        // loop to create string
        for(int i = 2; i < 16; i++, count++){
            // base case to check if there is a char
            if(buf[i] == '\0'){
                chk = true;
                break;
            }

            // create the string with the char stored
            output[count][0] = ' ';
            output[count][1] = buf[i];
            output[count][2] = ' ';
            output[count][3] = buf[-i];
            output[count][4] = '\n';
            output[count][5] = '\0';
        }
        // Terminal wouldnt end so I made a break flag.
        if(chk){
            break;
        }
        // reset buffer
        std::fill_n(buf, 32, '\0');
    }
    
    close(fd);
    // new line
    write(1, "\n", sizeof(char));
    // call writefile
    // writefile takes in the 2d array, positions array, size of the file, and amount of matches
    // output: is needed to know what letters to output
    // 2d array: is needed for the position of the letters
    // size of file is to iterate through the positions array
    // amount of matches is needed so I don't go out of bounds because it is possible size > matches
    writefile(output, posArr, FILE_SIZE, count);

    return 0;
}

