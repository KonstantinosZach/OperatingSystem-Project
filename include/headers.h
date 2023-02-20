#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/shm.h>
#include <time.h>
#include <sys/wait.h>

#define MEMORY_LOCK "/memorylocking\0"
#define MEMORY_CHANGED "/memorystate\0"
#define CHILD_LOCK "/childlocking\0"
#define SEM_PERMS (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)
#define TEXT_SZ (100*sizeof(char))

struct shared_use_st {
   char single_line[TEXT_SZ]; //εδώ γράφεται από τον parent η γραμμή που ζητά το child
   int line;                  //εδώ γράφει το παιδί την γραμμή που θέλει
   int number_of_lines;       //πόσες γραμμές έχει το αρχείο
   int transactions;          //αριθμός δοσοληψιών κάθε child
};