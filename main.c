#include "codec.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define ENCRYPTION 1
#define DECRYPTION 0
#define BUFFER_SIZE 4096
#define THREAD_COUNT 4

//holds a chunk of text to encrypt/decrypt and a position to reassemble the text.
typedef struct Task {
	char* text;
	size_t position;
} Task;

//pass multiple args to thread init.
typedef struct Thread_args {
    int key;
    int mode;
} Thread_args;


//Create our queue.
Task task_queue[256];
int task_count = 0;

//Create a lock for critical section.
pthread_mutex_t mutex_queue;
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

	while(1){
		Task task;
		int found = 0;

		pthread_mutex_lock(&mutex_queue);
		//Critial section
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

		//Check working mode.
		if(mode == ENCRYPTION && found == 1){
			encrypt(task.text ,key);
		}

		if(mode == DECRYPTION && found == 1){
			encrypt(task.text ,key);
		}
	}
}

/**
 * @brief Adding a task to the queue(threadsafe).
 * 
 * @param task 
 */
void addTask(Task task){
	pthread_mutex_lock(&mutex_queue);
	task_queue[task_count] = task;
	task_count++;
	pthread_mutex_unlock(&mutex_queue);
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

	//Holding arguments
	int key;
	char* flag_argument;
	int work_mode;
	
	//File variables
	char* file_content;
	size_t file_len;
	size_t file_size = BUFFER_SIZE;
	file_content = (char *)malloc(BUFFER_SIZE * sizeof(char));

	if( file_content == NULL){
        perror("Unable to allocate buffer");
        exit(1);
    }

	//Create space for content.
	char *content = malloc(sizeof(char) * BUFFER_SIZE);

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

	//set thread arguments.
	thread_args.mode = work_mode;
	thread_args.key = key;

	//so far key is valid and flags are valid.
	file_len = getline(&file_content, &file_size, stdin);
	pthread_mutex_init(&mutex_queue, NULL);
	int i;

	//init all threads.
	for(i = 0; i < THREAD_COUNT; i++){
		if(pthread_create(&th[i], NULL, &startThread, &thread_args) != 0){
			perror("Failed to initialize threads");
		}
	}

	//add tasks to the queue.
	for(i = 0; i < file_len; i++){
		Task t = {
			.text = file_content[i],
			.position = i
		};
		
		addTask(t);
	}

	//join all threads.
	for(i = 0; i < THREAD_COUNT; i++){
		if(pthread_join(th[i], NULL) != 0){
			perror("Failed to join the thread");
		}
	}

	char data[] = "my text to encrypt";
	key = 12;

	encrypt(data,key);
	printf("encripted data: %s\n",data);

	decrypt(data,key);
	printf("decripted data: %s\n",data);

	return 0;
}
