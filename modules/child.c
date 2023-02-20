#include "headers.h"

//Ο κώδικας του child process
int worker(struct shared_use_st* args)
{
   int pid = (int)getpid();
   printf("* child with pid(%d) was forked *\n",pid);

   //Ανοίγουμε τους σημαφόρους που έχουμε δημιοργήσει στον parent
   sem_t *memory_locking = sem_open(MEMORY_LOCK, O_RDWR);
   if (memory_locking == SEM_FAILED) {
      perror("sem_open(3) failed");
      exit(EXIT_FAILURE);
   }
   sem_t *memory_state= sem_open(MEMORY_CHANGED, O_RDWR);
   if (memory_state == SEM_FAILED) {
      perror("sem_open(3) failed");
      exit(EXIT_FAILURE);
   }
   sem_t *child_locking = sem_open(CHILD_LOCK, O_RDWR);
   if (child_locking == SEM_FAILED) {
      perror("sem_open(3) failed");
      exit(EXIT_FAILURE);
   }

   srand(getpid());

   //Για να υπολογήσουμε τους χρόνους των δοσοληψιών
   struct timespec tstart={0,0}, tend={0,0};
   clock_gettime(CLOCK_MONOTONIC, &tstart);
   clock_gettime(CLOCK_MONOTONIC, &tend);
   double avarage_time = 0;

   /****************** entry section *******************************/

   for(int transaction = 0; transaction < args->transactions; transaction++){

      //Επιλέγεται ποιο child process θα προχωρήσει και θα πραγματοποιήσει δοσοληψία με τον parent
      while (sem_wait(child_locking) == -1)
         if(errno != EINTR) {
            fprintf(stderr, "Process failed to lock semaphore\n");
            return 0;
         }
      
      //Μια δοσοληψία αποτελείται από 2 μέρη 
      //(1ο -> ζητάει την γραμμή και περιμένει) και (2ο -> διαβάζει την γραμμή που έδωσε ο parent)
      for(int ask_take = 0; ask_take < 2; ask_take++){

         //Περιμένουμε μέχρι το child να έχει αποκλειστική πρόσβαση στη μνήμη
         while (sem_wait(memory_locking) == -1)
            if(errno != EINTR) {
               fprintf(stderr, "Process failed to lock semaphore\n");
               return 0;
            }

   /****************** start of critical section *******************/

         int linepos = rand() % args->number_of_lines;   //τυχαίος αριθμός γραμμής

         //Ζητάμε την γραμμή
         if(ask_take == 0){
            printf("child pid(%d) wants the line with number: %d\n",pid, linepos);
            args->line = linepos;         //γράφουμε τον αριθμό στο shared memory
            clock_gettime(CLOCK_MONOTONIC, &tstart);
            
            //Ενημερώνουμε ότι έχει πραγματοποιηθεί αλλαγή στην μνήμη και είναι σειρά του parent να συνεχίσει
            if (sem_post(memory_state) == -1) {
               fprintf(stderr, "Process failed to unlock semaphore\n");
            }
         }
         //Διαβάζουμε την γραμμή
         else{
            clock_gettime(CLOCK_MONOTONIC, &tend);
            printf("child pid(%d) got %s\n",pid, args->single_line);
         }

   /****************** exit section ********************************/

      }
      avarage_time += (((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec));

      //Ενημερώνουμε ότι και τα υπόλοιπα child processes μπορούν να διεκδικήσουν
      //την πραγματοποίηση μιας δοσοληψίας
      if (sem_post(child_locking) == -1) {
         fprintf(stderr, "Process failed to unlock semaphore\n");
      }

      //Ενημερώνουμε ότι είναι δυνατή η είσοδος στο shared memory     
      if (sem_post(memory_locking) == -1) {
         fprintf(stderr, "Process failed to unlock semaphore\n");
      }

      //usleep(1); uncomment άμα θέλουμε να "δούμε" τα childs να κάνουν αιτήματα "μπλεγμένα" 
   }
   printf("child pid(%d) -> avarage time taken for each demand:%lf\n",pid, avarage_time/args->transactions);

   //Τέλος κλείνουμε τους σημαφόρους
   if (sem_close(memory_locking) < 0) {
      perror("sem_close(3) failed");
      sem_unlink(MEMORY_LOCK);
      exit(EXIT_FAILURE);
   }
   if (sem_close(child_locking) < 0) {
      perror("sem_close(3) failed");
      sem_unlink(CHILD_LOCK);
      exit(EXIT_FAILURE);
   }
   if (sem_close(memory_state) < 0) {
      perror("sem_close(3) failed");
      sem_unlink(MEMORY_CHANGED);
      exit(EXIT_FAILURE);
   }
   exit(EXIT_SUCCESS);
}