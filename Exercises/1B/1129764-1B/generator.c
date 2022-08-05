/**
 * @file generator.c
 * @author Ismail Serbest 1129764
 * @date 13.11.2020
 * @brief Defining the required structs and constants for generator and supervisor 
**/

#include "common.h"

/**
 * @brief This method parses the arguments.
 * @details This method saves the given arguments to the edges 
 * and returns the value of the largest vertex.
 * @param argc argument count
 * @param argv arguments entered from the terminal
 * @param edges edges to be filled.
 * @return vertex size.
**/
int parse_arguments(int argc, char *argv[], edge_t *edges);

/**
 * @brief Shuffles the given array.
 * @details Shuffles the given array according to the fisher yates shuffle algorithm.
 * @param arr all vertexes.
 * @param max_vertices vertex size.
**/
static void fisher_yates_shuffle(int *arr, int max_vertice);


/**
 * @brief Generates a solution to feedback_arc_set.
 * @param fisher_array all shuffeled vertex.
 * @param edges all edges.
 * @param edge_count edge count.
 * @param vertice_count vertex size.
**/
static void solution(int *fisher_array, edge_t *edges, int edge_count, int vertice_count);

/**
 * @brief This method turns off the open semaphores and shared memories.
 * @details If the program has open semaphore, shared memory or buffers, it turns them off.
**/
void cleanup_client(void);

sem_t *free_space;
sem_t *used_space;
sem_t *mutex_sem;
int shmfd;
myshm_t *myshm;
int send_cnt;
edge_t solution_arr[MAX_EDGE_LIMIT];

char *myprog;

/**
 * @brief This method generates a solution randomly.
 * @details This method generates a solution randomly and adds it to the circular buffer.
 * @param argc argument count.
 * @param argv arguments entered from the terminal.
 * @return EXIT_SUCCESS if the program runs without errors.
 * @return EXIT_FAILURE if there is an error in the program.
**/
int main(int argc, char *argv[]){
    myprog = argv[0];

    create_shm();
    create_semaphores();

    edge_t edges[argc-1];
    int vertices = parse_arguments(argc, argv, edges);
    int fisher_arr[vertices];

    for (size_t i = 0; i < vertices; i++){
        fisher_arr[i] = i;   
    }
    
    srand(time(NULL));
    int cnt = 0;
    while (myshm->state){
            fisher_yates_shuffle(fisher_arr, vertices);    
            solution(fisher_arr, edges, (argc-1), vertices);
            
            sem_wait(mutex_sem);
            sem_wait(free_space);
            
            /*if(send_cnt == 0){
                myshm->new_solution_size[cnt] = send_cnt;   
                break;
            }*/
            //printf("SOLUTION: ");
            for (size_t i = 0; i < send_cnt; i++){
                myshm->data[cnt][i].start = solution_arr[i].start;
                myshm->data[cnt][i].end = solution_arr[i].end;
                myshm->new_solution_size[cnt] = send_cnt;
                //printf(" %d%d",myshm->data[cnt][i].start,myshm->data[0][i].end);
            }
            //printf("\n");
            cnt++;
            cnt  %= MAX_SHM_SIZE;
        
            sem_post(used_space);
            sem_post(mutex_sem);
    }
    cleanup_client();
    return EXIT_SUCCESS;
}

int parse_arguments(int argc, char *argv[], edge_t *edges){
    int maxIndex = 0;

    for(int i = 1; i < argc; i++){ // 2-3
        edge_t edge;
        int succesfullyMatched = sscanf(argv[i], "%d-%d", &edge.start, &edge.end);

        if (succesfullyMatched != 2 || edge.start < 0 || edge.end < 0){
            exit(EXIT_FAILURE);
        }
        
        if(edge.start > maxIndex){
            maxIndex = edge.start;
        }
        if(edge.end > maxIndex){
            maxIndex = edge.end;
        }
        edges[i-1] = edge;
    }
    return ++maxIndex;
}

static void fisher_yates_shuffle(int *arr, int max_vertice){

    int j = 0;
    int tmp = 0;
    for(int i = max_vertice-1 ; i > 0; i--){
        j = rand() % (i+1);
        tmp = arr[i];
        arr[i] = arr[j];
        arr[j] = tmp;
    }
}

static void solution(int *fisher_array, edge_t *edges, int edge_count, int vertice_count){

    int end;
    int start;
    int start_index = -1;
    int end_index = -1;
    
    send_cnt = 0;
    for (size_t i = 0; i < edge_count; i++){
        end = edges[i].end;
        start = edges[i].start;
        for (size_t j = 0; j < vertice_count; j++){
            if(start == fisher_array[j]){
                start_index = j;
            }
            if(end == fisher_array[j]){
                end_index = j;
            } 
        }

        if(start_index > end_index){
            solution_arr[send_cnt].start = start;
            solution_arr[send_cnt].end = end;
            send_cnt++;
        }   
    }
}

static void create_shm(void){
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

    free_space = sem_open(FREE_SPACE, 0);
    used_space = sem_open(USED_SPACE, 0);
    mutex_sem = sem_open(MUTEX, 0);

    if(free_space == SEM_FAILED || used_space == SEM_FAILED || mutex_sem == SEM_FAILED){
        exit(EXIT_FAILURE);
    }
}

void cleanup_client(void){
    if(munmap(myshm, sizeof(*myshm)) == -1){
        usage(myprog, "myshm -> munmap");
    }
    if(close(shmfd) == -1){
        usage(myprog, "shmfd -> close");
    }
    if(sem_close(free_space) == -1){
        usage(myprog, "free_space -> close");
    }
    if(sem_close(used_space) == -1){
        usage(myprog, "used_space -> close");
    }
    if(sem_close(mutex_sem) == -1){
        usage(myprog, "mutex_sem -> close");
    }
}

static void usage(char *progarg, char *err){
    (void)fprintf(stderr, "ERROR: [%s] %s\n", progarg, err);
    exit(EXIT_FAILURE);
}