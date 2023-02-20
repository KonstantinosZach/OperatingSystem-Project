#include "headers.h"
#include "functions.h"

int main(int argc, char *argv[]){
    // Ελεγχος για τα argumets
    if(argc != 4){
        perror("Wrong arguments\n");
        exit(EXIT_FAILURE);
    }

    int number_of_lines = count_lines(argv[1]);
    int number_of_childs = atoi(argv[2]);
    if(number_of_childs < 1){
        perror("Wrong number of children\n");
        exit(EXIT_FAILURE);
    }

    int number_of_transactions = atoi(argv[3]);
    if(number_of_transactions < 1){
        perror("Wrong number of transactions\n");
        exit(EXIT_FAILURE);
    }

    //Δημιουργούμε το shared memory
    int shm_id;
    void* shm_ptr = (void *)0;
    struct shared_use_st *shared_info;
    shm_id = shmget(IPC_PRIVATE, sizeof(struct shared_use_st), (S_IRUSR|S_IWUSR));
    if (shm_id == -1) {
        perror("shmget failed\n");
        exit(EXIT_FAILURE);
    }
    printf("\nΟbtained a shared memory of %ld characters\n",TEXT_SZ);

    shm_ptr = shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat failed\n");
        exit(EXIT_FAILURE);
    }
    printf("Shared memory segment with id %d attached at %p\n", shm_id, shm_ptr);

    //Αρχικοποιούμε το struct
    shared_info = (struct shared_use_st *)shm_ptr;
    shared_info->number_of_lines = number_of_lines;
    shared_info->transactions = number_of_transactions;

    //Δημιοργούμε τους 3 σημαφόρους που θα χρειαστούμε
    //Ο memory_locking έχει τον ρόλο στο να επιτρέπει ή όχι την πρόσβαση στο shared memory
    sem_t *memory_locking;
    memory_locking = sem_open(MEMORY_LOCK, O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (memory_locking == SEM_FAILED) {
        perror("sem_open(3) error");
        exit(EXIT_FAILURE);
    }

    //Ο memory_state ενημερώνει άμα έχει υπάρξει αλλαγή στα δεδομένα της shared memory
    //Tην αρχικοποιούμε με 0 καθώς η μνήμη στην αρχή είναι άθικτη.
    sem_t *memory_state;
    memory_state = sem_open(MEMORY_CHANGED, O_CREAT | O_EXCL, SEM_PERMS, 0);
    if (memory_state == SEM_FAILED) {
        perror("sem_open(3) error");
        exit(EXIT_FAILURE);
    }

    //Ο child_locking επιτρέπει κάθε φορά σε ένα child process να εκτελέσει μια δοσοληψία με το parent process
    sem_t *child_locking;
    child_locking = sem_open(CHILD_LOCK, O_CREAT | O_EXCL, SEM_PERMS, 1);
    if (child_locking == SEM_FAILED) {
        perror("sem_open(3) error");
        exit(EXIT_FAILURE);
    }

    //Κλείνουμε τον child_locking καθώς δεν χρειάζεται στo parent
    if (sem_close(child_locking) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(CHILD_LOCK);
        exit(EXIT_FAILURE);
    }

    printf("* Parent starts forking... *\n");
    for (int child = 0; child < number_of_childs; child++) {
        int child_pid;
        if ((child_pid = fork()) < 0) {
            perror("Failed to create process");
            exit(EXIT_FAILURE);
        }
        if (child_pid == 0) {
            worker(shared_info);    //παίρνει ως όρισμα το shared memory
            exit(EXIT_SUCCESS);
        }
    }

    /****************** entry section *******************************/

    for(int child = 0; child < number_of_transactions*number_of_childs; child++){
        //To parent process περιμένει να γίνει κάποια αλλαγή στη μνήμη
        while (sem_wait(memory_state) == -1){
            if(errno != EINTR) {
                perror("Process failed to lock semaphore\n");
                exit(EXIT_FAILURE);
            }
        }

    /****************** start of critical section *******************/

        //Διαβάζει από την μνήμη τον αριθμό της γραμμής που θέλει το child 
        //και την αντιγράφει την γραμμή στο shared memory
        int pos = shared_info->line;
        char* line = get_line(argv[1],pos);
        strcpy(shared_info->single_line,line);
        free(line);
    /****************** exit section ********************************/

        //Ενημερώνουμε ότι το child μπορεί να ξαναμπεί και να πειράξει το shared memory
        if (sem_post(memory_locking) == -1) {
            perror("Process failed to unlock semaphore\n");
            exit(EXIT_FAILURE);
        }

    }

    //Μαζεύουμε τα child processes
    for(int child=0; child<number_of_childs; child++)
        printf("* child returned pid(%d) *\n",wait(0));
    printf("* End with the transactions *\n");

    //Αποδεσμεύουμε τον χώρο του shared memory
    if (shmdt(shm_ptr) == -1) {
        fprintf(stderr, "shmdt failed\n");
        exit(EXIT_FAILURE);
    }
    if (shmctl(shm_id, IPC_RMID, 0) == -1) {
        fprintf(stderr, "shmctl(IPC_RMID) failed\n");
        exit(EXIT_FAILURE);
    }

    //Και κάνουμε close και unlink τους σημαφόρους που έχουμε δημιουργήσει

    if (sem_close(memory_locking) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(CHILD_LOCK);
        exit(EXIT_FAILURE);
    }

    if (sem_close(memory_state) < 0) {
        perror("sem_close(3) failed");
        sem_unlink(CHILD_LOCK);
        exit(EXIT_FAILURE);
    }

    if (sem_unlink(MEMORY_CHANGED) < 0){
        perror("sem_unlink(3) failed");
        exit(EXIT_FAILURE);
    }
    if (sem_unlink(MEMORY_LOCK) < 0){
        perror("sem_unlink(3) failed");
        exit(EXIT_FAILURE);
    }       
    if (sem_unlink(CHILD_LOCK) < 0){
        perror("sem_unlink(3) failed");
        exit(EXIT_FAILURE);
    }       
    exit(EXIT_SUCCESS);

}