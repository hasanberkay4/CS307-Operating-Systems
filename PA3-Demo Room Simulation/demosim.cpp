#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>

using namespace std;

pthread_mutex_t lockThread = PTHREAD_MUTEX_INITIALIZER;

pthread_barrier_t barrier;

sem_t semSPass, *semSPassPtr, semALeave, *semALeavePtr;

sem_t semA, *semAPtr, semS, *semSPtr;

int totalS, waitingS = 0;

int enteredA = 0, *eaPtr, enteredS = 0, *esPtr, aIn = 0, sIn = 0;

void* aHandler(void* arg){
	pthread_mutex_lock(&lockThread);

	printf("Thread ID:%lu, Role:Assistant, I entered the classroom.\n", pthread_self());

	semSPassPtr = &semSPass;

	aIn++;

	if(waitingS == 1 && (sIn + 1) <= (3 * aIn)){
		sem_post(semSPassPtr);
		waitingS--;
	}

	else if(waitingS > 1 && (sIn + 2) <= (3 * aIn)){
		sem_post(semSPassPtr);
		sem_post(semSPassPtr);
		waitingS -= 2;
	}

	else if(waitingS > 1 && (sIn + 1) <= (3 * aIn)){
		sem_post(semSPassPtr);
		waitingS--;
	}

	eaPtr = &enteredA;	esPtr = &enteredS;

	semAPtr = &semA;	semSPtr = &semS;

	int lastGuy = 0;

	if( (*esPtr) > 1){
		sem_post(semSPtr);	sem_post(semSPtr);

		(*esPtr) -= 2;

		lastGuy = 1;
	}

	else{
		(*eaPtr)++;

		pthread_mutex_unlock(&lockThread);

		sem_wait(semAPtr);
	}

	printf("Thread ID:%lu, Role:Assistant, I am now participating.\n", pthread_self());

	pthread_barrier_wait(&barrier);

	printf("Thread ID:%lu, Role:Assistant, demo is over.\n", pthread_self());

	if(lastGuy == 1){
		pthread_barrier_destroy(&barrier);
		pthread_barrier_init(&barrier, NULL, 3);

		pthread_mutex_unlock(&lockThread);
	}

	pthread_mutex_lock(&lockThread);

	semALeavePtr = &semALeave;

	if(sIn > (3 * (aIn - 1))){
		pthread_mutex_unlock(&lockThread);

		sem_wait(semALeavePtr);

		pthread_mutex_lock(&lockThread);
	}


	printf("Thread ID:%lu, Role:Assistant, I left the classroom.\n", pthread_self());

	aIn--;

	pthread_mutex_unlock(&lockThread);

	return NULL;
}

void* sHandler(void* arg){
	pthread_mutex_lock(&lockThread);

	printf("Thread ID:%lu, Role:Student, I want to enter the classroom.\n", pthread_self());

	semSPassPtr = &semSPass;

	int waitedFlag = 0;

	if( (sIn + 1) > (3 * aIn)){
		waitedFlag = 1;
		waitingS++;

		pthread_mutex_unlock(&lockThread);

		sem_wait(semSPassPtr);


		sIn++;


		pthread_mutex_lock(&lockThread);
	}

	if(waitedFlag == 0){
		sIn++;
	}

	printf("Thread ID:%lu, Role:Student, I entered the classroom.\n", pthread_self());

	eaPtr = &enteredA;	esPtr = &enteredS;

	semAPtr = &semA;	semSPtr = &semS;

	int lastGuy = 0;

	if(*eaPtr > 0 && *esPtr > 0){
		sem_post(semAPtr);

		sem_post(semSPtr);

		(*esPtr)--;
		(*eaPtr)--;

		lastGuy = 1;
	}

	else{
		(*esPtr)++;

		pthread_mutex_unlock(&lockThread);

		sem_wait(semSPtr);
	}

	printf("Thread ID:%lu, Role:Student, I am now participating.\n", pthread_self());

	pthread_barrier_wait(&barrier);

	if(lastGuy == 1){
		pthread_barrier_destroy(&barrier);
		pthread_barrier_init(&barrier, NULL, 3);

		pthread_mutex_unlock(&lockThread);
	}

	pthread_mutex_lock(&lockThread);

	printf("Thread ID:%lu, Role:Student, I left the classroom.\n", pthread_self());
	sIn--;
	totalS--;

	if(waitingS >= 1 && (sIn + 1) <= (3 * aIn) ){
		sem_post(semSPassPtr);
		waitingS--;
	}

	semALeavePtr = &semALeave;

	if(aIn >= 1 && totalS == 0){
		for(int i = 0; i < aIn; i++){
			sem_post(semALeavePtr);
		}
	}

	else if( (sIn + 1) > (aIn * 3) && sIn <= (aIn * 3) ){
		sem_post(semALeavePtr);
	}

	pthread_mutex_unlock(&lockThread);

	return NULL;
}

int main(int argc, char* argv[]){
	int assistants = atoi(argv[1]);
	int students = atoi(argv[2]);

	totalS = students;

	int people = assistants + students;

	if(assistants <= 0 || students != assistants * 2){
		printf("The main terminates.\n");

		return 0;
	}

	printf("My program compiles with all the conditions.\n");

	sem_init(&semA, 0, 0);		sem_init(&semS, 0, 0);

	pthread_t threadArr[people];

	pthread_barrier_init(&barrier, NULL, 3);

	int idx = 0;
	while(idx < people){
		if(idx < assistants){
			if(pthread_create(&threadArr[idx], NULL, aHandler, NULL) != 0){
				printf("Error occurred while creating thread\n");
				return 0;
			}
		}

		else{
			if(pthread_create(&threadArr[idx], NULL, sHandler, NULL) != 0){
				printf("Error occurred while creating thread\n");
				return 0;
			}
		}

		idx++;
	}

	for(int i = 0; i < people; i++){
		if(pthread_join(threadArr[i], NULL) != 0){
			printf("Error occured while joining thread\n");
			return 0;
		}
	}

	pthread_barrier_destroy(&barrier);

	printf("The main terminates.\n");

	return 0;
}
