#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <curses.h>
#include <termios.h>
#include <fcntl.h>
#include <iostream>

#define ROW 10
#define COLUMN 50 

pthread_mutex_t frog_mutex;
pthread_mutex_t map_mutex;
pthread_mutex_t log_mutex;
int status = 0;//0 for continue, 1 for win, -1 for Lose, 2 for quit
struct Node{
	int x , y; 
	Node( int _x , int _y ) : x( _x ) , y( _y ) {}; 
	Node(){} ; 
} frog;
char map[ROW+10][COLUMN];
int logs[9][3];// 9 logs with 3 parameters.
// logs[i][0] means head position
// logs[i][1] means tail position
// logs[i][2] means direction, -1 for left, 1 for right
void writeLogsToMap()
{
    pthread_mutex_lock(&map_mutex);
    for (int i = 0; i < 9; i++)// 9 logs in total
    {
        if (logs[i][0] < logs[i][1])//head position < tail position
        {
            for (int col = 0; col < COLUMN-1; col++)
            {
                if (col >= logs[i][0] && col <= logs[i][1]) map[i+1][col] = '=';
                else map[i+1][col] = ' ';
            }
        }
        else
        {
            for (int col = 0; col < COLUMN-1; col++)
            {
                if (col > logs[i][1] && col < logs[i][0]) map[i+1][col] = ' ' ;
                else map[i+1][col] = '=';
            }
        }
    }
    pthread_mutex_unlock(&map_mutex);
}
// Determine a keyboard is hit or not. If yes, return 1. If not, return 0. 
int kbhit(void){
	struct termios oldt, newt;
	int ch;
	int oldf;

	tcgetattr(STDIN_FILENO, &oldt);

	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	if(ch != EOF)
	{
		ungetc(ch, stdin);
		return 1;
	}
	return 0;
}

void setStatus()// check game's status
{
	int frogX = frog.x;
	int frogY = frog.y;
	pthread_mutex_lock(&map_mutex);
	if (frogX == 0) // reach the oppposite river bank
	{
		status = 1;// win
		pthread_mutex_unlock(&map_mutex);
		return;	
	}

	if (map[frogX][frogY] == ' ') // Fall in to river
	{
		status = -1;//lose
		pthread_mutex_unlock(&map_mutex);
		return;
	}

	if (frogY < 0 || frogY > COLUMN-2) //cross left or right boundary
	{
		status = -1;//lose
		pthread_mutex_unlock(&map_mutex);
		return;
	}
	pthread_mutex_unlock(&map_mutex);	
}

void frogUp()
{
	pthread_mutex_lock(&frog_mutex);
	frog.x += -1;
	pthread_mutex_unlock(&frog_mutex);	
}
void frogDown()
{
	pthread_mutex_lock(&frog_mutex);
	frog.x += 1;
	pthread_mutex_unlock(&frog_mutex);
}
void frogLeft()
{
	pthread_mutex_lock(&frog_mutex);
	frog.y += -1;
	pthread_mutex_unlock(&frog_mutex);
}
void frogRight()
{
	pthread_mutex_lock(&frog_mutex);
	frog.y += 1;
	pthread_mutex_unlock(&frog_mutex);
}

void *logs_move(void* UNUSED)
{
	while(status == 0)// status is continue
	{
		usleep(100000);
        //Move the logs
        for (int log_id = 0; log_id < 9; log_id++)
        {
            if(logs[log_id][2] == -1)//-1 means left, log moves to left
            {
                logs[log_id][0] = ((logs[log_id][0] - 1) + COLUMN-1) % (COLUMN-1);
                logs[log_id][1] = ((logs[log_id][1] - 1) + COLUMN-1) % (COLUMN-1);
            }
            else// log moves to right
            {
                logs[log_id][0] = ((logs[log_id][0] + 1)) % (COLUMN-1);
                logs[log_id][1] = ((logs[log_id][1] + 1)) % (COLUMN-1);
            }
            if (frog.x == log_id+1)//frog on the log
            {
                if (logs[log_id][2] == -1) frogLeft();//frog moves with the log
                else frogRight();
            }
        }
        writeLogsToMap();// write logs to map
	}
	pthread_exit(NULL);
}

void writeFrogAndBanksToMap()
{
    pthread_mutex_lock(&map_mutex);
    for(int j = 0; j < COLUMN - 1; ++j) map[ROW][j] = '|' ;//write lower bank
    for(int j = 0; j < COLUMN - 1; ++j)	map[0][j] = '|' ;//write higher bank
    map[frog.x][frog.y] = '0';//write position of frog to map
    pthread_mutex_unlock(&map_mutex);
}

void*frog_move(void* UNUSED)
{
	while(status == 0)
	{
		if(kbhit())// check keyboard hits, to change frog's position or quit game.
		{
			char moveDirection = getchar();
			switch(moveDirection)
			{
				case 'W': case 'w':
				{
					frogUp();
					break;
				}
				case 'A': case 'a' :
				{
					frogLeft();
					break;
				}
				case 'S': case 's':
				{
					if (frog.x == ROW) continue;
					frogDown();
					break;
				}
				case 'D': case 'd' :
				{
					frogRight();
					break;
				}
				case 'Q': case 'q':
				{
					status = 2;//quit
					puts("\033[H\033[2J");//clear screen
					pthread_exit(NULL);
				}
			}
		}

		setStatus();// set game status according to frog's position
		if (status == -1) pthread_exit(NULL);//lose

        writeFrogAndBanksToMap();// write frog and banks to map

		puts("\033[H\033[2J");//clear screen
		for(int i = 0; i <= ROW; ++i) puts(map[i]);// Print the map into screen

		usleep(10000);
	}
	pthread_exit(NULL);
}

int main( int argc, char *argv[] )
{
	// Initialize the river map and frog's starting position
	memset( map , 0, sizeof( map ) ) ;
	int i , j ; 
	for( i = 1; i < ROW; ++i ){	
		for( j = 0; j < COLUMN - 1; ++j )	
			map[i][j] = ' ' ;  
	}	

	for( j = 0; j < COLUMN - 1; ++j ) map[ROW][j] = map[0][j] = '|' ;
    for( j = 0; j < COLUMN - 1; ++j ) map[0][j] = map[0][j] = '|' ;

	frog = Node( ROW, (COLUMN-1) / 2 ) ; 
	map[frog.x][frog.y] = '0' ; 

	//initialize logs
	srand((unsigned) time(NULL));
	for (int log_id = 0; log_id < 9; log_id++)
	{
		int length = 15;
		int head = rand() % 48;
		int tail = head + length - 1;
		tail = (tail + COLUMN-1) % (COLUMN-1);
		if (log_id % 2 == 1) 
        {
            logs[log_id][0] = head;
            logs[log_id][1] = tail;
            logs[log_id][2] = 1;
        }
		else
        {
            logs[log_id][0] = head;
            logs[log_id][1] = tail;
            logs[log_id][2] = -1;
        }
	}
    writeLogsToMap();
	/*  Create pthreads for wood move and frog control.  */
	pthread_mutex_init(&frog_mutex, NULL);
	pthread_mutex_init(&log_mutex, NULL);
	pthread_mutex_init(&map_mutex, NULL);
	void *unused;
    pthread_t pthread_log;
    pthread_t pthread_frog;
	pthread_create(&pthread_frog, NULL, frog_move, unused);
    pthread_create(&pthread_log, NULL, logs_move, unused);
	pthread_join(pthread_frog, NULL);
    pthread_join(pthread_log, NULL);
	/*  Display the output for user: win, lose or quit.  */
	std::system("clear");
	if (status == 1) std::cout << "You win the game!!\n";
	else if (status == -1) std::cout << "You lose the game!!\n";
	else if (status == 2) std::cout << "You exit the game.\n";
	
	return 0;
}
