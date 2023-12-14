#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

key_t waitUserUIDKey;
int waitUserUIDID;
int *waitUserUID;

key_t waitUserPIDKey;
int waitUserPIDID;
int *waitUserPID;

key_t serverPIDKey;
int serverPIDID;
int *serverPID;

int userCount;
int UID[4];
int PID[4];

int exitCall;
void init()
{
	waitUserUIDKey = ftok("/home/g_201711156/project", 100);
	waitUserUIDID = shmget(waitUserUIDKey, sizeof(int), IPC_CREAT|0666);
	
	if(waitUserUIDID == -1)
	{		
		printf("user UID SHM error");
		exit(1);
	}
	waitUserUID = (int *)shmat(waitUserUIDID, NULL, 0);
	*waitUserUID = -1;

	waitUserPIDKey = ftok("/home/g_201711156/project", 101);
	waitUserPIDID = shmget(waitUserPIDKey, sizeof(int), IPC_CREAT|0666);
	
	if(waitUserPIDID == -1)
	{
		printf("user PID SHM error");
		exit(1);
	}

	waitUserPID = (int *)shmat(waitUserPIDID, NULL, 0);
	*waitUserPID = -1;


	serverPIDKey = ftok("/home/g_201711156/project", 102);
    serverPIDID = shmget(serverPIDKey, sizeof(int), IPC_CREAT|0666);
    
    if(serverPIDID == -1)
    {   
        printf("server PID SHM error");
        exit(1);
    }
    
    serverPID = (int *)shmat(serverPIDID, NULL, 0);
    *serverPID = getpid();

	userCount = 0;
	for(int i = 0; i < 4; i++)
	{
		UID[i] = -1;
		PID[i] = -1;
	}
	
	exitCall = 0;
}		

void makeGameRoom()
{
    if(userCount != 4)
    {
        return;
    }

    if(fork() == 0)
    {
        char tempUID[4][10];
        for(int i = 0; i < 4; i++)
        {
            sprintf(tempUID[i], "%d", UID[i]);
        }
        execl("/home/g_201711156/project/inGameServer", "/home/g_201711156/project/inGameServer", tempUID[0], tempUID[1], tempUID[2], tempUID[3], NULL);
        printf("Exec fail");
        exit(1);
    }

	sleep(1);

    userCount = 0;
    for(int i = 0; i < 4; i++)
    {
		pid_t temp = (pid_t)PID[i];
        kill(temp, 10);
	printf("user %d : %d\n", i, PID[i]);
        UID[i] = -1;
        PID[i] = -1;
    }
}


void userCheck()
{
	int uid = *waitUserUID;
	int pid = *waitUserPID;

	printf("user %d input\n", uid);

	*waitUserUID = -2;
	*waitUserPID = -2;

	int isPlayerClose = 0;
	for(int i = 0; i < 4; i++)
	{
		if(uid == UID[i])
		{
			userCount -= 1;
			UID[i] = -1;
			PID[i] = -1;
			isPlayerClose = 1;
			printf("Player %d Close\n", uid);
		}	
	}
		
	if(isPlayerClose == 0)
	{
		userCount += 1;
		for(int i = 0; i < 4; i++)
		{
			if(UID[i] == -1)
			{
				UID[i] = uid;
				PID[i] = pid;
				printf("Player %d Matching\n", uid);
				break;
			}
		}
	}
	
	makeGameRoom();	
		
	*waitUserUID = -1;
	*waitUserPID = -1;
}	

void processExit()
{
    shmctl(waitUserUIDID, IPC_RMID, NULL);
    shmctl(waitUserPIDID, IPC_RMID, NULL);
    shmctl(serverPIDID, IPC_RMID, NULL);
    printf("matching server exit");
}   


void exitHandler()
{
	if(exitCall == 0 )
	{
		processExit();
		printf("process error exit\n");
		if(fork() == 0)
		{
			execl("/home/g_201711156/project/matching", "/home/g_201711156/project/matching", NULL);
		}
	}
}
	
void controlCHandler(int signo)
{
	processExit();
	exitCall = 1;
	exit(1);
}

void matchingHandler(int signo)
{
	userCheck();
}

int main()
{
	init();	
	signal(SIGINT, controlCHandler);
	signal(10, matchingHandler);
	atexit(exitHandler);	

	while(1)
	{
		//printf("user count : %d\n", userCount);
		sleep(10);
	}
}
