#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <setjmp.h>

#define MAX_SET 2
#define MAX_MATCH 3
#define MAX_ROUND 5
#define ALARM_TIME 5

typedef struct _userData
{
	int UID;
	char name[25];
	int score;
}userData;

int* shm;
userData uData;
int stdin_copy;
int score;
int isWait = 1;

void PlaySignalHandler(int signo);
void AlarmHandler(int signo);
void ExitHandler(int signo);
void GetSHM(char* uid);
void PrintFile(char* fileName);
void WaitServerSignal();
int GetUserInput(char* printText, int allowMin, int allowMax);

int main(int argc, char* argv[])
{
	signal(12, PlaySignalHandler);
	signal(SIGALRM, AlarmHandler);
	signal(2, ExitHandler);
	stdin_copy = dup(0);
	score = 0;

	//서버가 Shared Memory를 만들때까지 기다린다
	sleep(1);
	GetSHM(argv[1]);
	*shm = getpid();
	printf("pid in shm : %d\n", *shm);
	WaitServerSignal();

	for(int i = 0; i < MAX_SET; i++)
	{
		for(int j = 0; j < MAX_MATCH; j++)
		{
			int enemyIndex = *shm;
			PrintFile("round.txt");

			for(int k = 0; k < MAX_ROUND; k++)
			{			
				int userInput = 1;
				int enemyInput = 1;
	
				alarm(ALARM_TIME);
				printf("상대 : 플레이어 %d, %d/%d세트 %d/%d매치  %d/%d라운드 | 점수 : %d\n", enemyIndex, i + 1, MAX_SET, j + 1, MAX_MATCH, k + 1, MAX_ROUND, score);
				userInput = GetUserInput("| 1 : 협력 | 2 : 배신 |(제한시간 5초, 미입력시 자동 협력)\n", 1, 2);
				*shm = userInput;
				WaitServerSignal();
				enemyInput = *shm;
	
				//유저 협력시 1(협력, C)
				//유저 배신시 2(배신, B)
				if(userInput == 1)
				{
					if(enemyInput == 1)
					{
						PrintFile("cc.txt");
						score += 2;
					}
					else
						PrintFile("cb.txt");
				}	
				else
				{
					if(enemyInput == 1)
					{
						PrintFile("bc.txt");
						score += 3;
					}
					else
					{
						PrintFile("bb.txt");
						score -= 1;
					}			
				}	
			}	
			WaitServerSignal();
		}
	}
	*shm = score;
	WaitServerSignal();
	system("clear");
	printf("이번 게임의 우승자는 Player%d입니다!\n", *shm);
	return 0;
}

void PlaySignalHandler(int signo)
{
	isWait = -1;
}

void AlarmHandler(int signo)
{
	close(0);
}

void ExitHandler(int signo)
{
	printf("exit called\n");
	exit(1);
}

void GetSHM(char* uid)
{
	key_t key = ftok("/home/g_201711156/project", 400 + atoi(uid));
	int shmid = shmget(key, 0, IPC_CREAT|0666);
	shm = (int*)shmat(shmid, NULL, 0);
}

void PrintFile(char* fileName)
{
	system("clear");
	char buf[256];
	
	FILE* fp = fopen(fileName, "r");
	
	while(fgets(buf, 256, fp) != NULL)
	{
		printf("%s", buf);
	}
}

void WaitServerSignal()
{
	isWait = 1;
	while(1)
	{
		sleep(1);
		if(isWait == -1)
			break;
	}
}

int GetUserInput(char* printText, int allowMin, int allowMax)
{
	int userInput = 1;
	dup2(stdin_copy, 0);
	printf("%s", printText);
	scanf("%d", &userInput);
	
	while(1)
	{
		if(userInput >= allowMin && userInput <= allowMax)
			break;

		printf("잘못된 값을 입력하셨습니다.\n");
		scanf("%d", &userInput);		
	}

	return userInput;
}
