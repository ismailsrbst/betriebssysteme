/**
  @file intmul.c
  @author Ismail Serbest 1129764
  @date 12.12.2020
  @brief This Programm Implements an algorithm for the efficient multiplication of large integers.
**/
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>

/** name of the executable**/
static char *myprog;

/**
  @brief exits incase of an error.
  @details exits and prints error message if any error accured.
  @param msg char pointer that contains error message.
**/
static void error_exit(char* msg);

/**
  @brief checks if number is valid.
  @details checks if given hexadecimal number is valid.
  @param number char pointer that contains hexadecimal number.
  @return in case of error returns -1, otherwise returns 1.
**/
static int is_number_valid(char *number);

/**
  @brief creates child process
  @details creates child process and creates pipes to communicate with child
  @param pipe_in write end of pipe.
  @param pipe_out read end of pipe.
  @param A first part of divided number.
  @param B second part of divided number.
  @param AB multipication of A and B.
**/
static void child(int pipe_in[2], int pipe_out[2], char *A, char *B, char *AB);

/**
  @brief reads from child.
  @details reads from read end of pipe.
  @param pipefd file directory of used pipe.
  @param number char pointer to contain read number.
**/
static void read_from_child(int pipefd, char *number);

/**
  @brief writes to child.
  @details writes to write end of pipe.
  @param pipefd file directory of used pipe.
  @param A char pointer to contain written number.
  @param B char pointer to contain written number.
**/
static void write_to_pipe(int pipefd, char *A, char *B);

/**
  @brief handles digits.
  @details adds necessary number of zero end of digits.
  @param number contains digits that need to be modified.
  @param len contains how many zero must be added.
**/
static void add_end_zero(char *number, int len);

/**
  @brief sums numbers
  @details adds four hexadecimal numbers and reverses the solution for correct result.
  @param AHBH contains number to be added.
  @param AHBL contains number to be added.
  @param ALBH contains number to be added.
  @param ALBL contains number to be added.
**/
static void add_hex(char *AHBH, char *AHBL, char *ALBH, char *ALBL);

/**
  @brief main method of programm
  @details reads input from stdin and split them to two parts and send them to child for further processing
  @param argc count of arguments.
  @param argv array contains argument values.
  @return returns error if programm fails, otherwise returns 0.
**/
int main(int argc, char *argv[]){
    if(argc != 1)
        error_exit("invalid argument");
    myprog = argv[0];

    char *first_number = NULL;
    char *second_number = NULL;

    size_t len = 0;
    //read input
    if(getline(&first_number, &len, stdin) == -1){
        free(first_number);
        free(second_number);
        error_exit("getline -> firstline failed.");
    }
    len = 0;
    if(getline(&second_number, &len, stdin) == -1){
        free(first_number);
        free(second_number);
        error_exit("getline -> secondline failed.");
    }

    first_number[strlen(first_number) - 1] = '\0';
	second_number[strlen(second_number) - 1] = '\0';

    int digits = strlen(first_number);
    int half_digits = digits/2;

    if(digits != strlen(second_number)){
        free(first_number);
        free(second_number);
        error_exit("Numbers A and B not equal number of digits!!!");
    }

    if((digits != 1) && ((strlen(first_number) % 2) != 0)){
        free(first_number);
        free(second_number);
        error_exit("The number of digits is not even!!!");
    }

    if(is_number_valid(first_number) != 1 || is_number_valid(second_number) != 1){
        free(first_number);
        free(second_number);
        error_exit("An invalid input is encountered!!!");
    }

    if(digits == 1){
        int result = (int)strtol(first_number, NULL, 16) * (int)strtol(second_number, NULL, 16);
        fprintf(stdout, "%x\n", result);
        free(first_number);
        free(second_number);
        fflush(stdout);
        exit(EXIT_SUCCESS);
    }

    char AH[half_digits+1];
    char AL[half_digits+1];
    char BH[half_digits+1];
    char BL[half_digits+1];
    
    for(int i = 0; i < half_digits; i++){
        AH[i] = first_number[i];
        BH[i] = second_number[i];
    }
    AH[half_digits] = '\0';
    BH[half_digits] = '\0';

    int k = 0;
    for(int i = half_digits; i < strlen(first_number); i++){
        AL[k] = first_number[i];
        BL[k] = second_number[i];
        k++;
    }
    AL[half_digits] = '\0';
    BL[half_digits] = '\0';

    int pipe_AH_BH[2][2];
    int pipe_AH_BL[2][2];
    int pipe_AL_BH[2][2];
    int pipe_AL_BL[2][2];

    char AH_BH[digits+1];
    char AH_BL[digits+1];
    char AL_BH[digits+1];
    char AL_BL[digits+1];

    child(pipe_AH_BH[0], pipe_AH_BH[1], AH, BH, AH_BH);
    child(pipe_AH_BL[0], pipe_AH_BL[1], AH, BL, AH_BL);
    child(pipe_AL_BL[0], pipe_AL_BL[1], AL, BH, AL_BH);
    child(pipe_AL_BH[0], pipe_AL_BH[1], AL, BL, AL_BL);

    add_end_zero(AH_BH, digits);
    add_end_zero(AH_BL, digits/2);
    add_end_zero(AL_BH, digits/2);
    AL_BL[strlen(AL_BL)-1] = '\0';

    add_hex(AH_BH, AH_BL, AL_BH, AL_BL);

    free(first_number);
    free(second_number);
    
    return EXIT_SUCCESS;
}

static void add_hex(char *AHBH, char *AHBL, char *ALBH, char *ALBL){
    static char hex_mult[1024];
    memset(hex_mult, '\0', sizeof(hex_mult));
    int c1, c2, c3, c4, c5, x;
    int len_number = 0;
    char tmp[2] = { '0', '\0'};
    int len = strlen(AHBH);
    char carry[2] = { '0', '\0'};
    char result[] ={'0','0','\0'};
    int carry_int = 0; 
    for(int i = 1; i <= len; i++){
        tmp[0] = AHBH[strlen(AHBH) - i];
        len_number = strlen(AHBH)- i;
        c1 = (len_number >= 0 ? (int)strtol(tmp, NULL, 16) : 0);
        tmp[0] = AHBL[strlen(AHBL) - i];
        len_number = strlen(AHBL)- i;
        c2 = (len_number >= 0 ? (int)strtol(tmp, NULL, 16) : 0);
        tmp[0] = ALBH[strlen(ALBH) - i];
        len_number = strlen(ALBH) - i;
        c3 = (len_number >= 0 ? (int)strtol(tmp, NULL, 16) : 0);
        tmp[0] = ALBL[strlen(ALBL) - i];
        len_number = strlen(ALBL) - i;
        c4 = (len_number >= 0 ? (int)strtol(tmp, NULL, 16) : 0);
        
        carry_int = (int)strtol(carry, NULL, 16);
        c5 = (c1 + c2 + c3 + c4 + carry_int);
        x = sprintf(result, "%x", c5);
        //  carry
        if (x != 1){
            carry[0]  = result[0];
            hex_mult[strlen(hex_mult)] =  result[1];
            hex_mult[strlen(hex_mult)] =  '\0';
        } else{
           hex_mult[strlen(hex_mult)] =  result[0];
           hex_mult[strlen(hex_mult)] =  '\0';
           carry[0] = '0';
           carry_int = 0;
        }
        result[0]='0';
        result[1]='0';   
    }
    if (carry[0] != '0'){
        hex_mult[strlen(hex_mult)] =  carry[0];
        hex_mult[strlen(hex_mult)] =  '\0';
    }

    for(int i = strlen(hex_mult)-1; i >= 0; i--){
        printf("%c", hex_mult[i]);
    }
    printf("\n");
   
}

static void add_end_zero(char *number, int len){
    int size = strlen(number)-1;
    while(0 < len){
        number[size++] = '0';
        len--;
    }
    number[size] = '\0';
}

static void write_to_pipe(int pipefd, char *A, char *B){
    FILE *fp = fdopen(pipefd, "w");
    if(fp == NULL)
        error_exit("File can not open. -> Write");
    fprintf(fp,"%s\n%s\n", A, B);
    if(fclose(fp) != 0){
        error_exit("fclose failed.");
    }
}

static void read_from_child(int pipefd, char *number){
    FILE *fp_read = fdopen(pipefd, "r");
    if(fp_read == NULL)
            error_exit("File can not open. -> Read");
    size_t len = 0;
    char *tmp = NULL;
    if(getline(&tmp, &len, fp_read) == -1)
        error_exit("getline -> firstline failed.");
    strcpy(number, tmp);
    free(tmp);
    if(fclose(fp_read) != 0)
        error_exit("fclose failed.");    
}

static void child(int pipe_in[2], int pipe_out[2], char *A, char *B, char *AB){
    if(pipe(pipe_in) == -1 || pipe(pipe_out) == -1)
        error_exit("Pipe not created!!");

    int pid = fork();

    if(pid == 0){ // in child proces
        if(close(pipe_in[1]) == -1)
            error_exit("Pipe_in[1] not closed");
        if(close(STDIN_FILENO) == -1)
            error_exit("STDIN_FILENO not closed");
        if(dup2(pipe_in[0], STDIN_FILENO) == -1)
            error_exit("Dup2 -> pipe_in failed");
        if(close(pipe_in[0]) == -1)
            error_exit("Pipe_in[0] not closed");
        
        if(close(pipe_out[0]) == -1)
            error_exit("Pipe_out[0] not closed");
        if(close(STDOUT_FILENO) == -1)
            error_exit("STDOUT_FILENO not closed");
        if(dup2(pipe_out[1], STDOUT_FILENO) == -1)
            error_exit("Dup2 -> pipe_out failed");
        if(close(pipe_out[1]) == -1)
            error_exit("Pipe_out[1] not closed");
            
        if(execlp(myprog, myprog, NULL) == -1)
            error_exit("Execlp failed.");

    } else if(pid < 0){
        if(close(pipe_in[1]) == -1)
            error_exit("Pipe_in[1] not closed");
        if(close(pipe_in[0]) == -1)
            error_exit("Pipe_in[0] not closed");
        
        if(close(pipe_out[0]) == -1)
            error_exit("Pipe_out[0] not closed");
        if(close(pipe_out[1]) == -1)
            error_exit("Pipe_out[1] not closed");

        error_exit("fork failed!!");
    } else{ // parent
        if(close(pipe_in[0]) == -1)
            error_exit("Pipe_in[0] not closed");
        if(close(pipe_out[1]) == -1)
            error_exit("Pipe_out[1] not closed");

        write_to_pipe(pipe_in[1], A, B);
        //close(pipe_in[1]);
        int status;
        waitpid(pid, &status, 0);
        if(WEXITSTATUS(status) != EXIT_SUCCESS){
            error_exit("WEXITSTATUS -> FAILURE");
        }
        read_from_child(pipe_out[0], AB);
        //close(pipe_out[0]);
         
    }
}

static int is_number_valid(char *number){
    while (*number != '\0'){
        if((*number >= '0' && *number <= '9') || (*number >= 'A' && *number <= 'F') || (*number >= 'a' && *number <= 'f')){
            number++;
        }else{
            return -1;
        }
    }
    return 1;
}

static void error_exit(char* msg){
    fprintf(stderr, "[%s] ERROR: %s\n", myprog, msg);
	exit(EXIT_FAILURE);
}