#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <pthread.h>

using namespace std;

//////////////////////////////////////////////////////// COMMAND CLASS DEFINITION ////////////////////////////////////////////////////////

class commandObj {
	public:
		int cmdCounter;
		string cmdName;
		string input;
		string option;
		string fileDirection;
		string fileName;
		bool firstInput;
		string background;

		commandObj(int c, string n, string i = "", string o = "", string fD = "", string fN = "", bool fI = true, string b = ""){		// DEFAULT CONSTRUCTOR
			this->cmdCounter = c;
			this->cmdName = n;
			this->input = i;
			this->option = o;
			this->fileDirection = fD;
			this->fileName = fN;
			this->background = b;
			this->firstInput = fI;
		}

	void print(){
		cout << cmdCounter << cmdName << input << option << fileDirection << fileName << background << endl;
	}
};

//////////////////////////////////////////////////////// PARSER FUNCTION ///////////////////////////////////////////////////////

vector<commandObj> fillCommands(){
	vector<commandObj> commands;

	ifstream openedFile("commands.txt");
	string currLine;
	string currWord;

	while(getline(openedFile, currLine)){
			stringstream ss(currLine);
			string n = "";
			ss >> n;

			bool fI = true;
			int c = 1;
			string i = "", o = "", fD = "", fN = "", b = "";
			while(ss >> currWord){
				if(n == "wait"){
					break;
				}

				else if(currWord == "&"){
					b = currWord;
				}

				else if(currWord[0] == '-'){
					if(i == ""){
						fI = false;
					}
					o = currWord;
					c++;
				}

				else if(currWord == ">" || currWord == "<"){
					if(currWord == "<"){
						c++;
					}

					fD = currWord;
					ss >> fN;
				}

				else{
					c++;
					i = currWord;
				}
			}

			commandObj currObj(c, n, i, o, fD, fN, fI, b);
			commands.push_back(currObj);
		}

	openedFile.close();

	return commands;
}

//////////////////////////////////////////////////////// FUNCTION TO WRITE PARSE.TXT ////////////////////////////////////////////////////////

void printCommands(vector<commandObj> & commandArr){
	ofstream openedFile("parse.txt");

	for(int i = 0; i < commandArr.size(); i++){
		openedFile << "----------" << endl;
		openedFile << "Command: " << commandArr[i].cmdName << endl;
		openedFile << "Inputs: " << commandArr[i].input << endl;
		openedFile << "Options: " << commandArr[i].option << endl;
		string fileDirection = commandArr[i].fileDirection != "" ? commandArr[i].fileDirection : "-";		// SPECIAL CASE #1
		string background = commandArr[i].background != "" ? "y" : "n"; // SPECIAL CASE #2
		openedFile << "Redirection: " << fileDirection << endl;
		openedFile << "Background Job: " << background << endl;
		openedFile << "----------" << endl;
	}

	openedFile.close();
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;			// MUTEX LOCK

// DID NOT USE BECAUSE POINTER APPROACH REQUIRED
struct pipeObj{
	int readEnd;
};

//////////////////////////////////////////////////////// THREAD FUNCTION TO PRINT OUTPUTS ////////////////////////////////////////////////////////

void* print_threaded(void* arg){
	pthread_mutex_lock(&lock);
	printf("---- %ld\n", pthread_self());

	int fileDesc = *(int*)arg;		// PIPE READ-END

	if(fileDesc < 0){
		fprintf(stderr, "Invalid file descriptor\n");
		return NULL;
	}

	FILE* openedFile = fdopen(fileDesc, "r");
	if(openedFile == NULL){
		fprintf(stderr, "Opening fd[0] failed\n");
		exit(1);
	}

	char buffer[512];

	ssize_t nread;

	while(fgets(buffer, sizeof(buffer), openedFile) != NULL){
		printf("%s", buffer);
	}

	printf("---- %ld\n", pthread_self());

	pthread_mutex_unlock(&lock);

	return NULL;
}

int main(){
	vector<commandObj> commandArr;
	commandArr = fillCommands();
	printCommands(commandArr);

	vector<pid_t> bgProcesses;
	vector<pthread_t> threadArr;

	//commandArr[0].print(); 				FOR DEBUG PURPOSES

	for(int i = 0; i < commandArr.size(); i++){
		if(commandArr[i].cmdName != "wait"){
			if(commandArr[i].fileDirection == ">"){
				int prWithOutput = fork();
				if(prWithOutput < 0){
					fprintf(stderr, "Fork for process with output file failed, Exiting...\n");
					exit(1);
				}

				else if(prWithOutput == 0){		// CHILD PROCESS
					close(STDOUT_FILENO);
					int openedFile = open(strdup(commandArr[i].fileName.c_str()), O_CREAT | O_APPEND | O_WRONLY, 0666);

					char* argumentList[commandArr[i].cmdCounter + 1];

					int cmdCounter = 0;
					argumentList[cmdCounter++] = strdup(commandArr[i].cmdName.c_str());

					if(commandArr[i].firstInput){
						if(commandArr[i].input != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].input.c_str());
						}

						if(commandArr[i].option != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].option.c_str());
						}
					}

					else{
						if(commandArr[i].option != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].option.c_str());
						}

						if(commandArr[i].input != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].input.c_str());
						}
					}

					argumentList[cmdCounter] = NULL;

					execvp(strdup(commandArr[i].cmdName.c_str()), argumentList);
				}

				else{
					if(commandArr[i].background != "&"){
						waitpid(prWithOutput, NULL, 0);
					}

					else{
						bgProcesses.push_back(prWithOutput);
					}
				}
			}

			else{		// DIRECTION IS NOT ">"
				int* fileDesc = (int*) malloc(sizeof(int) * 2);		// PIPE IS CREATED WITH POINTER RATHER THAN ARRAY 
				pipe(fileDesc);
				pthread_t cmdThread;		// THREAD IS CREATED

				int prWithoutOutput = fork();

				if(prWithoutOutput < 0){
					fprintf(stderr, "Fork failed for the forking of process without output input\n");
					exit(1);
				}

				else if(prWithoutOutput == 0){			// CHILD PROCESS
					char* argumentList[commandArr[i].cmdCounter + 1];

					// NOT REQUIRED AND ALSO DID NOT WORK GIVIN FILE AS COMMAND WORKED

					/*
					if(commandArr[i].fileDirection == "<"){
						int openedFile = open(commandArr[i].fileName.c_str(), O_APPEND | O_WRONLY);
						if(openedFile < 0){
							fprintf(stderr, "Invalid file opening\n");
							exit(1);
						}
						dup2(openedFile, STDIN_FILENO);
					}
					*/

					int cmdCounter = 0;
					argumentList[cmdCounter++] = strdup(commandArr[i].cmdName.c_str());

					if(commandArr[i].firstInput){
						if(commandArr[i].input != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].input.c_str());
						}

						if(commandArr[i].option != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].option.c_str());
						}
					}

					else{
						if(commandArr[i].option != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].option.c_str());
						}

						if(commandArr[i].input != ""){
							argumentList[cmdCounter++] = strdup(commandArr[i].input.c_str());
						}
					}

					if(commandArr[i].fileName != ""){
						argumentList[cmdCounter++] = strdup(commandArr[i].fileName.c_str());
					}

					argumentList[cmdCounter] = NULL;

					dup2(fileDesc[1], STDOUT_FILENO);		// WRITE TO WRITE END OF PIPE INSTEAD OF TERMINAL

					execvp(strdup(commandArr[i].cmdName.c_str()), argumentList);
				}

				else{		// SHELL PROCESS
					close(fileDesc[1]);		// TO NOTIFY THREAD FUNCTION WRITING IS DONE IT IS TIME TO READ UNTIL EOF
					pthread_create(&cmdThread, NULL, print_threaded, &fileDesc[0]);		// READ END OF PIPE IS GIVEN AS ARGUMENT
					threadArr.push_back(cmdThread);

					if(commandArr[i].background == "&"){
						bgProcesses.push_back(prWithoutOutput);
					}

					else{
						waitpid(prWithoutOutput, NULL, 0);
					}
				}
			}
		}

		else{				// WAIT COMMAND ENCOUNTERED
			for(int i = 0; i < bgProcesses.size(); i++){
				waitpid(bgProcesses[i], NULL, 0);
			}

			for(int i = 0; i < threadArr.size(); i++){
				pthread_join(threadArr[i], NULL);
			}
		}
	}

	// COMMANDS ARE DONE JUST WAIT BG PROCESSES AND THREADS BEFORE FINISHING PROGRAM

	for(int i = 0; i < bgProcesses.size(); i++){
		waitpid(bgProcesses[i], NULL, 0);
	}

	for(int i = 0; i < threadArr.size(); i++){
		pthread_join(threadArr[i], NULL);
	}

	return 0;
}
