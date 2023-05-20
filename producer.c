#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <fcntl.h> //
#include <errno.h> //
#include <assert.h>//
#include <sys/file.h>//
#include <time.h>


#define TEXT_SZ 2048

struct shared_use{
    int written_by_you; // boolean o consumer to thetei 0 kai o producer to kanei 1
	char some_text[TEXT_SZ]; // to string pou tupwnetai 
    int kids;
    int requests;
};

int main(){
    srand(time(NULL));

    int k = 3;
    int n = 4 ;

    int status;
    void *shared_memory = (void *)0;
	struct shared_use *shared_stuff;
    int shmid;
    char buffer[BUFSIZ];

    clock_t begin;
    clock_t end;
    double time;
    
    shmid = shmget((key_t)1234, sizeof(struct shared_use), 0666 | IPC_CREAT); // dhmiourgw kleidi me casting 
	if (shmid == -1) {
		fprintf(stderr, "shmget failed\n");
		exit(EXIT_FAILURE);
	}
	shared_memory = shmat(shmid, (void *)0, 0); // to kanoume attach sto process
	if (shared_memory == (void *)-1) {
		fprintf(stderr, "shmat failed\n");
		exit(EXIT_FAILURE);
	}
	printf("Shared memory segment with id %d attached at %p\n", shmid, shared_memory);

    FILE* fp;
    int count =0; 
    char c ;
    char* filename = "mosca.txt";
    char string[256];
    fp = fopen(filename, "r");

    if (fp == NULL){
        printf("Couldnt open file %s \n", filename);
        return 0; 
    }
    for(c = getc(fp); c!=EOF; c=getc(fp)){
        if(c == '\n'){
            count = count + 1;
        }
    }
    printf("\nThe file %s has %d lines \n\n", filename, count);
    fclose(fp);

    // SEMAPHORES
    sem_t *sem1 = sem_open("sem1", O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);

	if (sem1 != SEM_FAILED) {
		printf("created new semaphore!\n");
	} else if (errno== EEXIST ) {
		printf("semaphore appears to exist already!\n");
		sem1 = sem_open("sem1", 0);
	}

	assert(sem1 != SEM_FAILED);

    sem_t *sem2 = sem_open("sem2", O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 1);

	if (sem2 != SEM_FAILED) {
		printf("created new semaphore!\n");
	} else if (errno== EEXIST ) {
		printf("semaphore appears to exist already!\n");
		sem2 = sem_open("sem2", 0);
	}

	assert(sem2 != SEM_FAILED);

    sem_t *sem3 = sem_open("sem3", O_CREAT|O_EXCL, S_IRUSR|S_IWUSR, 0);

	if (sem3 != SEM_FAILED) {
		printf("created new semaphore!\n\n");
	} else if (errno== EEXIST ) {
		printf("semaphore appears to exist already!\n");
		sem3 = sem_open("sem3", 0);
	}

	assert(sem3 != SEM_FAILED);

    
    int pid;
    for(int i=0; i<k; i++){
        pid = fork();
        
        //////////////////////////////////

        //parent process
        if (pid != 0){
            shared_stuff = (struct shared_use *)shared_memory;
            sem_wait(sem1); // wait for parent to be done with prev child
            for(int l=0; l<n; l++){
                sem_wait(sem3); //parent waits for child to write on memory
                printf("Parent process %d \n", getpid()); 
                
                shared_stuff->written_by_you = 0;
                shared_stuff->kids = k;
                shared_stuff->requests = n;

                int line = atoi(shared_stuff->some_text) ;
                printf("Line %d :", line);
                int i=1;
                fp = fopen(filename, "r");
                while(fgets(string, sizeof(string), fp)){
                    if(i==line){
                        printf(" %s ", string);
                        break;
                    }
                    i++ ;
                }
                shared_stuff->written_by_you = 0;

                sem_post(sem2);// parent is ready for next request
            }
            
            wait(&status);
        }
        else{
            shared_stuff = (struct shared_use *)shared_memory;

            /////////////////////////
            printf("In child process %d\n", getpid()); 
            
            //consume
            for(int j=0; j<n; j++){
                sem_wait(sem2); // waits for parent to be done with the previous request  
                int rndm = rand() % count ;
                printf("line number %d \n", rndm);
                sprintf(buffer, "%d", rndm) ;
                strncpy(shared_stuff->some_text, buffer, TEXT_SZ);
                begin = clock();    // end of request
                sem_post(sem3); // child sends message to parent
                end = clock();  // this should continue after parent process prints line 
            }
            time = ((double)end-(double)begin)/CLOCKS_PER_SEC ; 
            
            sem_post(sem1); // this child is done
            printf("child %d is done with count %f\n\n", getpid(), time) ;
            exit(0);
        }
    }    

    fclose(fp);

    sem_close(sem1);
    sem_close(sem2);
    sem_close(sem3);

    sem_unlink("sem1");
    sem_unlink("sem2");
    sem_unlink("sem3");

    if (shmdt(shared_memory) == -1) {
		fprintf(stderr, "shmdt failed\n");
		exit(EXIT_FAILURE);
	}
    if (shmctl(shmid, IPC_RMID, 0) == -1) {
		fprintf(stderr, "shmctl(IPC_RMID) failed\n");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);

    return 0;
}