#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <signal.h>
#include <string.h>

typedef struct _userData
{
	int UID;
	char name[25];
	int score;
}userData;

char* errorMessage;
int isMatching = -1;
int isLogined = -1;
int userInput;
int* shm[3];
userData uData;

void GetServerSHM(char* path);
int Login();
void PrintFile(char* fileName);
void SetUID();
int GetUserInput(char* printText, int allowMin, int allowMax);
void TryMatching();
//----signal and exit handler function
void MatchingHandler(int signo);
void AtExitHandler();
void ExitHandler(int signo);
//----

int main(void)
{	
	//매칭 완료시 호출
	signal(10, MatchingHandler);
	//ctrl+c 입력시 호출
	signal(2, ExitHandler);
	//프로세스 종료시 호출
	atexit(AtExitHandler);
	//지정된 key값에서 shm을 가져옴
	GetServerSHM("/home/g_201711156/project");
	
	while(1)
	{
		if(Login() == -1)
		{
			errorMessage = "8글자를 초과한 닉네임을 입력하셨습니다.";
			continue;
		}
		SetUID();
		isLogined = 1;
		PrintFile("lobby.txt");

		while(1)
		{
			if(isLogined == -1)
			{
				isLogined = 1;
				break;
			}

			if(isMatching == -1)
				userInput = GetUserInput("| 1 : 매칭 시작 | 2 : 룰 설명 | 3 : 랭킹 확인 | 4 : 로그아웃 |\n: ", 1, 4);
			else
				userInput = GetUserInput("| 1 : 매칭 취소 | 2 : 룰 설명 | 3 : 랭킹 확인 | 4 : 로그아웃 |\n: ", 1, 4);
			
			switch(userInput)
			{
				case -1:
					errorMessage = "잘못된 값을 입력하셨습니다.";
					PrintFile("lobby.txt");
				continue;
				case 1:
					if(isMatching == -1)
						isMatching = 1;
					else
						isMatching = -1;
						
					TryMatching();
					PrintFile("lobby.txt");
				break;
				case 2:
					PrintFile("rule.txt");
				continue;
				case 3:
					PrintFile("rank.txt");
				continue;
				case 4:
					isLogined = -1;
					if(isMatching == 1)
					{
						isMatching = -1;
						TryMatching();
					}
				continue;
				default:
				continue;
			}
		}
	}
	
	return 0;
}

void GetServerSHM(char* path)
{
	int keys[3] = {100, 101, 102};
	int shmid;

	for(int i = 0; i < sizeof(keys) / sizeof(int); i++)
	{
		key_t key = ftok(path, keys[i]);
		shmid = shmget(key, 0, IPC_CREAT|0666);
		
		if(shmid == -1)
		{	
			perror("can't find server shm, please running server program first.\n");
			exit(1);
		}
		
		shm[i] = (int*)shmat(shmid, NULL, 0);
	}
}

int Login()
{
	char name[25];

	PrintFile("login.txt");
	printf("사용자 이름을 입력해주세요(한글 최대 8글자).\n: ");
	scanf("%s", name);

	//입력값이 한글 8글자를 초과했는지 확인
	for(int i = 0; i < 25; i++)
	{
		if(name[i] == '\0')
		{
			strcpy(uData.name, name);
			return 0;
		}
	}
	return -1;
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
	
	if(errorMessage != NULL)
	{
		printf("입력 에러 : %s\n", errorMessage);
		errorMessage = NULL;
	}
}

void SetUID()
{	
	FILE* fp = fopen("userData.txt", "r+b");
	int count = 0;
	userData tempData;
	//닉네임이 데이터 파일에 존재하는지 확인
	//있으면 UID를 불러온다.
	while(fread(&tempData, sizeof(userData), 1, fp) == 1)
	{		
		if(strcmp(uData.name, tempData.name) == 0)
		{
			uData.UID = tempData.UID;
			return;
		}
		count++;
	}
	
	//없으면 파일의 가장 뒤에 데이터를 저장한다.
	uData.UID = count;
	uData.score = 0;
	fseek(fp, sizeof(userData) * count, SEEK_SET);
	fwrite(&uData, sizeof(userData), 1, fp);
	fclose(fp);
}

int GetUserInput(char* printText, int allowMin, int allowMax)
{
	int userInput;

	printf("%s", printText);
	scanf("%d", &userInput);	
	
	if(userInput < allowMin || userInput > allowMax)
		return -1;

	return userInput;
}

void TryMatching()
{
	*shm[0] = uData.UID;
	*shm[1] = (int)getpid();
	kill((pid_t)*shm[2], 10);
}

void MatchingHandler(int signo)
{
	printf("\n\nMatchingSignalRecieved\n\n");
	isMatching = -1;
	char tempUID[10];
	sprintf(tempUID, "%d", uData.UID);
	//if(fork() == 0)
		execl("/home/g_201711156/project/Client_Main", "/home/g_201711156/project/Client_Main", tempUID);
	//else
	//	wait(NULL);
}

//프로세스 종료시 실행
void AtExitHandler()
{
	printf("\nExitHandlerActive\n");
	if(isMatching == 1)
		TryMatching();
}

//ctrl + c 입력시 실행
void ExitHandler(int signo)
{
	exit(0);
}
