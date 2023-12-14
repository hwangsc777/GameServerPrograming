#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <string.h>
#include <unistd.h>

typedef struct _userData
{
	int UID;
	char name[25];
	int score;
}userData;

userData heap[500];
int size;

void heapSet(userData uData)
{
	size++;
	int cur = size;

	while(cur != 1)
	{
		if(heap[cur/2].score < uData.score)
		{
			heap[cur] = heap[cur/2];
			cur /= 2;
		}
		else
		{
			break;
		}
	}
	
	heap[cur] = uData;
}

userData heapGet()
{
	userData retData = heap[1];
	heap[1] = heap[size];
	size--;

	int up = 1;
	int down = 2;

	while(down <= size)
	{
		if(down + 1 <= size && heap[down+1].score > heap[down].score)
		{
			down++;
		}
		
		if(heap[up].score < heap[down].score)
		{
			userData temp = heap[up];
			heap[up] = heap[down];
			heap[down] = temp;
			
			up = down;
			down*=2;
		}
		else
		{
			break;
		}
	}

	return retData;
}

void initHeap()
{
	size = 0;

     FILE *userDataFile = fopen("userData.txt", "rb");
     userData uData;
     
     while(fread(&uData, sizeof(userData), 1, userDataFile) > 0)
     {   
         printf("UID : %d, Score : %d\n", uData.UID, uData.score);
         heapSet(uData);
     }
     fclose(userDataFile);
}

void ranktxtWrite()
{
	FILE *rankFile = fopen("rank.txt", "w");
	userData uData;
	
	fprintf(rankFile, "=====rank.txt=====\n");
	for(int i = 0; i < 20; i++)
	{
		if(size == 0)
		{
			break;
		}
		
		uData = heapGet();
		fprintf(rankFile, "Rank %d : %s - %d\n", (i+1), uData.name, uData.score);
	}
	
	fclose(rankFile);
}

int main()
{
	initHeap();
	ranktxtWrite();		
}
