/**
  @file ispalindrom.c
  @author Ismail Serbest 1129764
  @date 11.11.2020
  @brief This program checks if the given text is a palindrom.
**/


#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/*
    @brief global variable: myprog.
*/
char *myprog;

/**
 * This function writes helpful usage information about the program to stderr.
 * @details global variable: myprog.
**/
static void usage(void){
    (void)fprintf(stderr, "USAGE: %s\n", myprog);
    exit(EXIT_FAILURE);
}

/**
 * This function removes all occurences of the space character from string.
 * @details The string must be terminated by a '\0'
 * @param string to remove all space characters.
 * 
 * side effects: The string source is overwritten. 
**/
static void removeSpace(char *string){
    char *tmp = string;
    while ((*tmp = *string) != 0){
        if (*tmp != ' '){
            tmp++;
        }
        string++;
    }
    
}

/**
 * This function chances all lowercase letter to uppercase letter in the string.
 * @details The string must be terminated by a '\0'
 * @param string to chances all lowercase letter to uppercase letter.
**/
static void stringToUpperCase(char *string){
    while (*string != 0){
        if(*string >= 'a' && *string <= 'z'){
            *string = toupper(*string);
        }
        string++;
    }

}

/**
 * This function checks if the given text is a palindrom.
 * @details The string must be terminated by a '\0'.
 * @param string to check if it is a palindrom.
 * @return 1 - text is a palindrom.
 * @return 0 - text is not a palindrom.
**/
static int isPalindrom(char *string){
    char *end = string + strlen(string) - 1;
    while(string < end){
        if(*string != *end){
            return 0;
        }
        string++;
        end--;
    }
    return 1;
}

/**
 * Reads the input and tells whether the value read is a palindrome or not
 * @param input readed input
 * @param s_flag is 's' marked.
 * @param i_flag is 'i' marked.
 * @param output save result.
**/
static void inputReader(FILE *input, int s_flag, int i_flag, FILE *output){

    size_t size = 0;
    char *inputRead = NULL;
    char *tmp = NULL;
    while (getline(&inputRead, &size, input) != -1){

        tmp = strdup(inputRead);
        if(tmp[strlen(tmp)-1] == '\n'){
            inputRead[strlen(inputRead)-1] = 0; 
            tmp[strlen(tmp)-1] = '\0';
        }
        if(s_flag == 1){
            (void)removeSpace(tmp);
        }
        if(i_flag == 1){
            (void)stringToUpperCase(tmp);
        }

        if (output == NULL){
            if(isPalindrom(tmp) == 1){
                (void)printf("%s is a palindrom\n", inputRead);
            }else{
                (void)printf("%s is not a palindrom\n", inputRead);
            }
        }else{
            if(isPalindrom(tmp) == 1){
                fprintf(output, "%s is a palindrom\n", inputRead);
            }else{
                fprintf(output, "%s is not a palindrom\n", inputRead);
            }
        }  
    }
    free(inputRead);
    free(tmp);
}

/**
 * Reads options and arguments and directs the program according to the result.
 * @param argc the number of arguments entered from the command line.
 * @param argv arguments entered from the command line.
 * @return EXIT_SUCCESS if the program runs without errors.
 * @return EXIT_FAILURE if there is an error in the program.
**/
int main(int argc, char *argv[]){

    myprog = argv[0];

    int opt;
    int s_flag = 0;
    int i_flag = 0;
    int o_flag = 0;

    FILE *output = NULL;

    while ((opt = getopt(argc, argv, "sio:")) != -1){
        
        switch (opt){
        case 's':
            s_flag++;
            break;
        case 'i':
            i_flag++;
            break;
        case 'o':
            o_flag++;
            output = fopen(optarg, "w");
            break;
        default:
            break;
        }
    }

    if(s_flag > 1 || i_flag > 1 || o_flag > 1){
        (void)usage();
    }

    if(o_flag == 1){
        if(output == NULL){
            (void)usage();
        }
        int tmpIndex = optind;
        FILE *input;
        while (tmpIndex < argc){
            input = fopen(argv[tmpIndex], "r");
            if(input==NULL){
                (void)usage();
            }
            inputReader(input, s_flag, i_flag, output);
            tmpIndex++;
            fclose(input);
        }
    }else{
        inputReader(stdin, s_flag, i_flag, NULL);
    }
    
    return EXIT_SUCCESS;
}
