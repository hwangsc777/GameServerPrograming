#include <stdio.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct _userData
{
	int UID;
	char name[25];
	int score;
}userData;

typedef struct _roundInput
{
	int usernum;
	int input;
}roundInput;

int gameScore[4];
userData users[4];

key_t userInputKey[4];
int userInputID[4];
int *userInput[4];
int userPID[4];

int matchingRand[3];
int matchingIndex[4];
void matchingResult(int input[4], int round);

int isWait = 1;

void initFile(char *argc[])
{
	FILE *userDataFile = fopen("userData.txt", "rb");
    for(int i = 1; i <= 4; i++)
    {
        int uid = atoi(argc[i]);
        fseek(userDataFile, sizeof(userData) * uid, SEEK_SET);
        fread(&users[i-1], sizeof(userData), 1, userDataFile);
		printf("%d --- %d user score : %d\n",uid, users[i-1].UID, users[i-1].score);
	}

    fclose(userDataFile);
}

void init()
{
	for(int i = 0; i < 4; i++)
    {
        userInputKey[i] = ftok("/home/g_201711156/project", 400 + users[i].UID);

        userInputID[i] = shmget(userInputKey[i], sizeof(int), IPC_CREAT|0666);
        if(userInputID[i] == -1)
        {
            printf("user ID error : %d",i);
            exit(1);
        }
        userInput[i] = (int *)shmat(userInputID[i], NULL, 0);

        gameScore[i] = 0;
    }

	sleep(4);
	for(int i = 0; i < 4; i++)
		userPID[i] = *userInput[i];
	
	printf("\n\nuserPID = [%d, %d, %d, %d]", userPID[0], userPID[1], userPID[2], userPID[3]);
}

void SetMatchingRand()
{
	for(int i = 0; i < 3; i++)
		matchingRand[i] = 0;

	int count = 0;

	while(1)
	{
		int randInt = rand() % 3 + 2;
		
		for(int i = 0; i < 3; i++)
		{
			if(matchingRand[i] == 0)
			{	
				matchingRand[i] = randInt;		
				count++;
				break;
			}
			
			if(matchingRand[i] == randInt)
				break;

			
		}

		if(count >= 3)
			break;
	}

	printf("\nMatchingRand : [%d, %d, %d]\n", matchingRand[0], matchingRand[1], matchingRand[2]);
}

void SendAllUserSignal()
{
	for(int i = 0; i < 4; i++)
	{
		kill((pid_t)userPID[i], 12);
	}
}

void inGame()
{
	for(int set = 0; set < 2; set++)
	{
		SetMatchingRand();
		
		for(int match = 0; match < 3; match++)
		{
			//matchingRand 변수를 통해 Player1이 상대할 대상을 먼저 선택한 후
			//나머지를 매칭
			matchingIndex[0] = 1;
			matchingIndex[1] = matchingRand[match];
			matchingIndex[2] = matchingRand[(match + 1) % 3];
			matchingIndex[3] = matchingRand[(match + 2) % 3];
			
			printf("Matching Player %d vs %d\n", matchingIndex[0], matchingIndex[1]);
			printf("Matching Player %d vs %d\n", matchingIndex[2], matchingIndex[3]);
				
			//각 유저에게 이번 매치 상대 플레이어 번호를 전달해줌
			*userInput[matchingIndex[0] - 1] = matchingIndex[1];
			*userInput[matchingIndex[1] - 1] = matchingIndex[0];
			*userInput[matchingIndex[2] - 1] = matchingIndex[3];
			*userInput[matchingIndex[3] - 1] = matchingIndex[2];

			SendAllUserSignal();

			for(int round = 0; round < 5; round++)
			{
				sleep(6);
				matchingResult(*userInput, round);				

				printf("Player Input [%d, %d, %d, %d]\n", *userInput[0], *userInput[1], *userInput[2], *userInput[3]);

				int temp = *userInput[matchingIndex[0] - 1];
				*userInput[matchingIndex[0] - 1] = *userInput[matchingIndex[1] - 1];
				*userInput[matchingIndex[1] - 1] = temp;
				temp = *userInput[matchingIndex[2] - 1];
				*userInput[matchingIndex[2] - 1] = *userInput[matchingIndex[3] - 1];
				*userInput[matchingIndex[3] - 1] = temp;

				SendAllUserSignal();
			}
			sleep(3);
		}
	}
	printf("gameScore [%d, %d, %d, %d]\n", gameScore[0], gameScore[1], gameScore[2], gameScore[3]);
	SendAllUserSignal();
	sleep(3);
	printf("final user score [%d, %d, %d, %d]\n", *userInput[0], *userInput[1], *userInput[2], *userInput[3]);	

	for(int i = 0; i < 4; i++)
		users[i].score = *userInput[i];

	int max = 0;
	int index = 0;
	for(int i = 0; i < 4; i++)
	{
		if(max < *userInput[i]);
		{
			max = *userInput[i];
			index = i;
		}
	}
	
	for(int i = 0; i < 4; i++)
		*userInput[i] = index + 1;
	
	printf("max : %d, winner : player%d\n", max, index + 1);
	SendAllUserSignal();
}

void matchingResult(int input[4], int round)
{
	roundInput p1[2];
	roundInput p2[2];
	int cur1 = 0;
	int cur2 = 0;

	for(int i = 0; i < 4; i++)
	{
		if(matchingIndex[i] == 0)
		{
			if(cur1 == 0)
			{
				cur1 = 1;
				p1[0].usernum = i;
				p1[0].input = input[i];
			}
			else
			{
				p2[0].usernum = i;
				p2[0].input = input[i];
			}
		}
		else
		{
			if(cur2 == 0)
			{
				cur2 = 1;
				p1[1].usernum = i;
				p1[1].input = input[i];
			}
			else
			{
				p2[1].usernum = i;
				p2[1].input = input[i];
			}
		}
	}

	int thisRoundScoreChange[4] = {0,0,0,0};
	for(int i = 0; i < 2; i++)
	{
		if(p1[i].input == 1 && p2[i].input == 1)
		{
			thisRoundScoreChange[p1[i].usernum] += 2;
			thisRoundScoreChange[p2[i].usernum] += 2;
		}
		else if(p1[i].input == 1 && p2[i].input == 2)
		{
			thisRoundScoreChange[p2[i].usernum] += 3;
		}
		else if(p1[i].input == 2 && p2[i].input == 1)
		{
			thisRoundScoreChange[p1[i].usernum] += 3;
		}
		else if(p1[i].input == 2 && p2[i].input == 2)
		{
			thisRoundScoreChange[p1[i].usernum] -= 1;
			thisRoundScoreChange[p2[i].usernum] -= 1;
		}
	}
	for(int i = 0; i < 4; i++)
	{
		if(input[i] == 1 && round == 0)
		{
			gameScore[i] += 2;
		}
		else
		{
			gameScore[i] += thisRoundScoreChange[i];
		}
	}	
}
void scoreCount()
{
	int scoreTable[4] = {+3, +1, -1, -3};
    for(int i = 0; i < 4; i++)
    {
        int up = 0;
        for(int j = 0; j < 4; j++)
        {
            if(gameScore[i] < gameScore[j])
            {
                up++;
            }
        }

        users[i].score += scoreTable[up];
        printf("uesr %d Score : %d\n", users[i].UID, users[i].score);
    }
}

void scorePrintFile()
{
	FILE *userDataFile = fopen("userData.txt", "rb+");
    for(int i = 0; i < 4; i++)
    {
        int uid = users[i].UID;
        fseek(userDataFile, sizeof(userData) * uid, SEEK_SET);
        fwrite(&users[i], sizeof(userData), 1, userDataFile);
    }
	fclose(userDataFile);
}

void exitGame()
{
	for(int i = 0; i < 4; i++)
    {
        shmctl(userInputID[i], IPC_RMID, NULL);
    }
	if(fork() == 0)
	{
		execl("/home/g_201711156/project/rankSort", "/home/g_201711156/project/rankSort", NULL);
		printf("rank sort fail\n");
		exit(1);
	}
}

//게임이 시작될 때 매칭서버에서 유저 UID를 입력값으로 가져옴
int main(int argv, char *argc[])
{
	printf("game Start\n");
	initFile(argc);
  	init();

	printf("init End\n");

	inGame();
	
	//scoreCount();
	scorePrintFile();
	exitGame();

/*
FILE *userDataFile = fopen("userData.txt", "wb");
for(int i = 0; i < 40; i++)
{
	userData temp;
	temp.UID = i;
	strcpy(temp.name, "test name is not");
	temp.score = 0;
	
	fseek(userDataFile, sizeof(userData)*i, SEEK_SET);

	fwrite(&temp, sizeof(userData), 1, userDataFile);
}
fclose(userDataFile);
*/
		 
		
}
	
