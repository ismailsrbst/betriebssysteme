/**
 * @file supervisor.c
 * @author Ismail Serbest 1129764
 * @date 13.11.2020
 * @brief Defining the required structs and constants for generator and supervisor 
**/

#include "common.h"

/**
 * @brief handle signal
 * @details makes variable quit 1 and myshm-> state is 0.
 * @param signal 
**/
void handle_signal(int signal);

/**
 * @brief This method turns off the open semaphores and shared memories.
 * @details If the program has open semaphore, shared memory or buffers, it turns them off.
**/
void cleanup_server(void);

char *myprog; //program name
volatile sig_atomic_t quit = 0; // for signal

sem_t *free_space;
sem_t *used_space;
sem_t *mutex_sem;
int shmfd = -1;
myshm_t *myshm;

/**
 * @brief This method stores the shortest solution.
 * @details This method prints the shortest of the sent solutions on the screen.
 * @param argc argument count
 * @param argv arguments entered from the terminal
**/
int main(int argc, char *argv[]){

    myprog = argv[0];

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL); 

    create_shm();
    create_semaphores();

    int index_solution = 0;
    int upper = 8;
    myshm->state = 1; 
    while (!quit){
        
        sem_wait(used_space);
      
        if (upper > myshm->new_solution_size[index_solution]){
            int start;
            int end;
            
            if(myshm->new_solution_size[index_solution] == 0){
                printf("[%s] The graph is acyclic!\n", myprog);
                break;
            }
            printf("[%s] Solution with %d edges:", myprog, myshm->new_solution_size[index_solution]);
            for (size_t i = 0; i < myshm->new_solution_size[index_solution] ; i++){
                start = myshm->data[index_solution][i].start;
                end = myshm->data[index_solution][i].end;
                printf(" %d-%d", start, end);
            }
            printf("\n");

            upper =  myshm->new_solution_size[index_solution];
        }
    
        index_solution++;
        index_solution %= MAX_SHM_SIZE;
        sem_post(free_space);
    }
    myshm->state = 0;
    cleanup_server();
    return EXIT_SUCCESS;
}

static void create_shm(void){
   shm_unlink(SHM_NAME);
    // create and/or open the shared memory object:
    shmfd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0600);
    if (shmfd == -1){
        usage(myprog, "shm_open failed.");
    }
    // set the size of the shared memory:
    if (ftruncate(shmfd, sizeof(struct myshm)) < 0){
        usage(myprog, "ftruncate failed.");
    }
    myshm = mmap(NULL, sizeof(*myshm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    if (myshm == MAP_FAILED){
        usage(myprog, "mmap failed.");
    }
}

static void create_semaphores(void){
    sem_unlink(FREE_SPACE);
    sem_unlink(USED_SPACE);
    sem_unlink(MUTEX);
    free_space = sem_open(FREE_SPACE, O_CREAT | O_EXCL, 0600, MAX_SHM_SIZE);
    if(free_space == SEM_FAILED ){
        usage(myprog, "free_space -> sem_open failed.");
    }
    used_space = sem_open(USED_SPACE, O_CREAT | O_EXCL, 0600, 0);
    if(used_space == SEM_FAILED){
        usage(myprog, "used_space -> sem_open failed.");
    }
    mutex_sem = sem_open(MUTEX, O_CREAT | O_EXCL, 0600, 1);
    if(mutex_sem == SEM_FAILED){
        usage(myprog, "mutex -> sem_open failed.");
    }
}

void cleanup_server(void){
    if(munmap(myshm, sizeof(*myshm)) == -1){
        usage(myprog, "myshm -> munmap");
    }
    if(close(shmfd) == -1){
        usage(myprog, "shmfd -> close");
    }
    if(shm_unlink(SHM_NAME) == -1){
        usage(myprog, "SHM_NAME -> unlink");
    }
    if(sem_close(free_space) == -1){
        usage(myprog, "free_space -> close");
    }
    if(sem_unlink(FREE_SPACE) == -1){
        usage(myprog, "free_space -> unlink");
    }
    if(sem_close(used_space) == -1){
        usage(myprog, "used_space -> close");
    }
    if(sem_unlink(USED_SPACE) == -1){
        usage(myprog, "used_space -> unlink");
    }
    if(sem_close(mutex_sem) == -1){
        usage(myprog, "mutex_sem -> close");
    }
    if(sem_unlink(MUTEX) == -1){
        usage(myprog, "mutex_sem -> unlink");
    }
}

void handle_signal(int signal){ 
    quit = 1;
    myshm->state = 0; 
}

static void usage(char *progarg, char *err){
    (void)fprintf(stderr, "ERROR: [%s] %s\n", progarg, err);
    exit(EXIT_FAILURE);
}