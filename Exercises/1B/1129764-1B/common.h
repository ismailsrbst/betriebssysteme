#ifndef COMMON_H__
#define COMMON_H__

/**
 * @file common.h
 * @author Ismail Serbest 1129764
 * @date 13.11.2020
 * @brief Defining the required structs and constants for generator and supervisor 
**/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <signal.h>

#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#define SHM_NAME    "/1129764_shared_memory"
#define FREE_SPACE  "/1129764_free_semaphore"
#define USED_SPACE  "/1129764_used_semaphore"
#define MUTEX   "/1129764_write_semaphore"
#define MAX_EDGE_LIMIT  (8)
#define MAX_SHM_SIZE    (128)

typedef struct edge{
    int start;
    int end;
} edge_t;

typedef struct myshm{
    int state;
    edge_t data[MAX_SHM_SIZE][MAX_EDGE_LIMIT];
    int new_solution_size[MAX_SHM_SIZE];
} myshm_t;

/**
 * @brief This method prints errors in the program and terminates the program.
 * @param prog program name.
 * @param err error message.
**/
static void usage(char *prog, char *err);

/**
 * @brief This method opens shared memories.
**/
static void create_shm(void);

/**
 * @brief This method opens semaphores.
**/
static void create_semaphores(void);
#endif