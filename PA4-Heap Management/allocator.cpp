#include <pthread.h>
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <semaphore.h>

using namespace std;

class HeapManager{
	private:
		struct node{
			int id;
			int size;
			int index;
			node* next;

			node(int idP, int sizeP, int indexP) : id(idP), size(sizeP), index(indexP), next(NULL) {}
		};

		node* head;
		sem_t semHeap;

	public:
		HeapManager(){
			head = NULL;
			sem_init(&semHeap, 0, 1);
		}

		int initHeap(int size);
		int myMalloc(int id, int size);
		int myFree(int id, int index);
		node* findPrev(node* curr);

		void print();
};

void HeapManager::print(){
	node* curr = head;

	cout << "[" << curr->id << "]" << "[" << curr->size << "]" << "[" << curr->index << "]";
	curr = curr->next;

	while(curr != NULL){
		cout << "---" << "[" << curr->id << "]" << "[" << curr->size << "]" << "[" << curr->index << "]";
		curr = curr->next;
	}

	cout << endl;
}

int HeapManager::initHeap(int size){
	node* initNode = new node(-1, size, 0);
	head = initNode;

	print();

	return 1;
}

int HeapManager::myMalloc(int id, int size){
	sem_wait(&semHeap);

	node* curr = head;

	while(curr != NULL){
		if(curr->id == -1 && curr->size >= size){
			curr->id = id;

			int remaining = curr->size - size;

			curr->size = size;

			if(remaining > 0){
				node* remainingNode = new node(-1, remaining, curr->index + size);
				//if(curr->next != NULL){
					//remainingNode->next = curr->next->next;
				remainingNode->next = curr->next;
				//}
				curr->next = remainingNode;
			}

			cout << "Allocated for thread " << id << endl;

			print();

			sem_post(&semHeap);

			return curr->index;
		}

		curr = curr->next;
	}

	cout << "Can not allocate, requested size " << size << " for thread " << id << " is bigger than remaining size." << endl;
	print();

	sem_post(&semHeap);

	return -1;
}

HeapManager::node* HeapManager::findPrev(node* curr){
	if(curr == head){
		return NULL;
	}

	node* prev = head;
	while(prev != NULL && prev->next != curr){
		prev = prev->next;
	}

	return prev;
}

int HeapManager::myFree(int id, int index){
	sem_wait(&semHeap);

	node* curr = head;
	while(curr != NULL){
		if(curr->id == id && curr->index == index){
			node* prev = findPrev(curr);
			node* next = curr->next;

			if(prev != NULL && prev->id == -1){
				prev->size += curr->size;
				prev->next = next;

				free(curr);
				curr = prev;
			}

			if(next != NULL && next->id == -1){
				curr->size += next->size;
				curr->next = next->next;

				free(next);
			}

			curr->id = -1;
			cout << "Freed for thread " << id << endl;

			print();

			sem_post(&semHeap);

			return 1;
		}

		curr = curr->next;
	}

	sem_post(&semHeap);

	return -1;
}
