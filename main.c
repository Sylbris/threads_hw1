#include "codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <time.h>

#define ENCRYPTION 1
#define DECRYPTION 0
#define ENCRYPTION_MAX_SIZE 1024
#define THREAD_COUNT 6

//holds a chunk of text to encrypt/decrypt and a position to reassemble the text.
typedef struct Task {
	char text[ENCRYPTION_MAX_SIZE + 1];
	size_t position;
} Task;

//pass multiple args to thread init.
typedef struct Thread_args {
    int key;
    int mode;
} Thread_args;


//Create our queue & array to hold tasks.
Task task_queue[256];
Task task_array[256];
int task_count = 0;
int array_current_size = 0;

//Holding arguments
int key;
char* flag_argument;
int work_mode;

//Create a lock for critical section.
pthread_mutex_t mutex_queue;
pthread_cond_t cond_queue;
/**
 * @brief Consumer
 * consumes a task from the queue(thread safe).
 * 
 * @param args 
 * @param mode 
 * @param key 
 * @return void* 
 */
void* startThread(void* args){

	//Unpack args
	Thread_args* thread_args = (Thread_args *) args;
	int key = thread_args->key;
	int mode = thread_args->mode;

	//Run in loop and check for tasks.
	while(1){

		Task task;
		int found = 0;

		pthread_mutex_lock(&mutex_queue);
		if(task_count > 0){
			found = 1;
			task = task_queue[0];
			int i;
			for(i = 0; i < task_count - 1; i++){
				task_queue[i] = task_queue[i + 1];
			}
			task_count--;
		}
		pthread_mutex_unlock(&mutex_queue);
		
		//No tasks were found, finish the job.
		if(found == 0){
			return NULL;
		}

		//Check working mode.
		if(mode == ENCRYPTION && found == 1){
			encrypt(task.text ,key);
			
			//Add task to array.
			pthread_mutex_lock(&mutex_queue);
			task_array[array_current_size++] = task;
			pthread_mutex_unlock(&mutex_queue);
		}

		if(mode == DECRYPTION && found == 1){
			decrypt(task.text ,key);

			//Add task to array.
			pthread_mutex_lock(&mutex_queue);
			task_array[array_current_size++] = task;
			pthread_mutex_unlock(&mutex_queue);
		}
	}
}

/**
 * @brief Adding a task to the queue(threadsafe).
 * 
 * @param task 
 */
void addTask(Task* task){

	pthread_mutex_lock(&mutex_queue);
	task_queue[task_count] = *task;
	task_count++;
	pthread_mutex_unlock(&mutex_queue);

}

/**
 * @brief Check cmd args
 * 
 */
void get_set_args(int argc, char * const argv[]){

	//Check valid number of arguments.
	if( argc != 3){
		fprintf( stderr, "usage: %s source destination\n", *argv);
		exit(1);
	}

	flag_argument = argv[2];

	//Check key is valid.
	if( (key = atoi(argv[1])) == 0){
		fprintf( stderr, "Error: %s \n", argv[1]);
		exit(1);
	}

	//Check flag argument.
	if( flag_argument[0] != '-'){
		fprintf( stderr, "Error: %s \n", flag_argument);
		exit(1);
	}
	else {
		if(flag_argument[1] == 'e'){

			work_mode = ENCRYPTION;

		}else if(flag_argument[1] == 'd'){

			work_mode = DECRYPTION;
			
		}else {
			fprintf( stderr, "Error: %c \n", flag_argument[1]);
			exit(1);
		}
	}
}

//task comparator for sorting
int cmp( const void *a, const void *b )
{
    const Task *left  = (Task *)a;
    const Task *right = (Task *)b;

    return ( left->position < right->position ) - ( right->position < left->position );  
}
/**
 * @brief The function recieves 2 cmd arguments, 
 * first argument is the key an integer.
 * 
 * second one is the flag:
 * -e for encryption
 * -d for decryption
 * 
 * example:
 * coder key -e < my_original_file > encripted_file
 * coder key -d < my_decripter_file > my_original_file
 * 
 * @param argc 
 * @param argv 
 * @return int 
 */
int main(int argc, char *argv[])
{

	pthread_t th[THREAD_COUNT];
	Thread_args thread_args;

	get_set_args(argc, argv);

	//set thread arguments.
	thread_args.mode = work_mode;
	thread_args.key = key;

	//init mutex.
	pthread_mutex_init(&mutex_queue, NULL);

	int i;
	char buffer[ENCRYPTION_MAX_SIZE+1];
	size_t pos = 0;

	while( fgets(buffer,1024, stdin) != NULL){

		Task t = {
			.text = "",
			.position = pos++
		};

		//Partition file content
		strncpy( t.text, buffer, strlen(buffer) );
		addTask(&t);
	}

	//init & start all threads.
	for(i = 0; i < THREAD_COUNT; i++){
		if(pthread_create(&th[i], NULL, &startThread, &thread_args) != 0){
			perror("Failed to initialize threads");
		}
	}
	
	//join all threads.
	for(i = 0; i < THREAD_COUNT; i++){
		if(pthread_join(th[i], NULL) != 0){
			perror("Failed to join the thread");
		}
	}
	pthread_mutex_destroy(&mutex_queue);

	//Sort the array.
	qsort( task_array, array_current_size, sizeof( Task ), cmp );

	//concat all array
	for(i=0; i < array_current_size; i++){
		printf("%s", task_array[i].text);
	}

	return 0;
}
