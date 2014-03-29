// Purdue ECE568 Embedded System
// Group# 6
// Member: Yuhui Zheng, Yusheng Weijian, Calvin Holic
// Topic: iRobot Maze Traversal & Mine Detection

#include "createoi.h"
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

//MACRO Constants
#define CW90 84
#define CCW90 85
#define CW180 177
#define ROWS 5
#define COLS 13
#define START_COL 6
#define UNIT_DIST 577
#define CORRECTION 730
#define LEFT_SPEED 250
#define RIGHT_SPEED 247

//Structs
typedef struct Node Node;
struct Node{
	Node* prev;
	short wall_N, wall_S, wall_E, wall_W;
	short visited;
	short col, row;
	short direction_entered;
};

//Input event data format
typedef struct {
        struct timeval time;
        unsigned short type;
        unsigned short code;
        unsigned int value;
} input_event;

//Global Variables
short direction = 0; //0 = north, 1 = east, 2 = south, 3 = west
int foundflag = 0;
Node* maze[ROWS][COLS];
int cl, cfl, cr, cfr, bumpsense, wallsense, dist, ang;

//Function Headers
int explore_Node(Node * curNode);
void init_maze(void);
int explore_forward(Node *curNode);
int explore_left(Node *curNode);
int explore_right(Node *curNode);
int forward_open(Node * curNode);
int left_open(Node * curNode);
int right_open(Node * curNode);
void point_towards(short dir);
void check_bomb(void);
void read_all_sensors(void);
int user_start(void);
int attempt_move(Node *curNode);

//////////////////////////////////////////////////////////////////////////
//int main(int argc, char *argv[])
int main(void)
{
	if(user_start() == 1)
	{
		//if (argc != 5) return 1;
		//test_three(atoi(argv[1]),atoi(argv[2]),atoi(argv[3]),atoi(argv[4]));

		init_maze();
		maze[4][7]->visited = 1;
		maze[4][7]->direction_entered = 0;

		//Enter the maze
		directDrive(LEFT_SPEED,RIGHT_SPEED);
		dist = waitDistance(UNIT_DIST,1);
		directDrive(0,0);
		waitTime(.2);

		printf("Recurse!\n");

		explore_Node(maze[4][7]);

		return 1;
	}
	else printf("Did not enter work function\n");
}


int user_start()
{
	int fd;
	char *file_path = "/dev/input/event0";
	input_event *buffer;
	long pressed = -2;
	size_t nbytes;
	nbytes = sizeof(input_event);
	fd=open(file_path, O_RDONLY);
	if(fd >= 0)		printf("File opened ..\n");
	pressed = read(fd,buffer,nbytes);//Poll for key-press
	printf("%d, %d \n",buffer->value,buffer->code);//For debug
	printf("%d \n",pressed);//,errno);//For debug...
	close(fd);
	if(buffer->code)
		return 1; //Start the bot.
	else
		return 0;
}

/////////////////////////////////////////////////////////////////////////

void init_maze(void){
	int i, j;
	byte song[4]={70,48,60,32};

	for(i=0; i<ROWS; i++){
		for(j=0; j<COLS; j++){
			maze[i][j] = calloc(1,sizeof(Node));
			maze[i][j]->row = i; 
			maze[i][j]->col = j;
			maze[i][j]->direction_entered = -1;
			if (i == 0) maze[i][j]->wall_N = 1;
			if ((i == ROWS - 1)&&(j!=START_COL)) maze[i][j]->wall_S = 1;
			if (j == 0) maze[i][j]->wall_W = 1;
			if (j == COLS - 1) maze[i][j]->wall_E = 1;
		}
	}

	startOI_MT("/dev/ttyUSB0");
	writeSong(0,2,&song[0]);
	enterFullMode();
}

//Explore all paths from the current node
int explore_Node(Node * curNode){
	int temp_direction;

	//Condition these checks on foundflag so that we just backtrack as
	// soon as we find the bomb.
	check_bomb();
	if (forward_open(curNode) && !foundflag) {
		if (attempt_move(curNode)) {
			explore_forward(curNode);
		}
	}
	if (left_open(curNode) && !foundflag) {
		temp_direction = curNode->direction_entered - 1;
		if (temp_direction == -1) temp_direction = 3;
		point_towards(temp_direction);
		if (attempt_move(curNode)) {
			explore_left(curNode);
		}
	}
	if (right_open(curNode) && !foundflag) {
		temp_direction = curNode->direction_entered + 1;
		if (temp_direction == 4) temp_direction = 0;
		point_towards(temp_direction);
		if (attempt_move(curNode)) {
			explore_right(curNode);
		}
	}

	temp_direction = curNode->direction_entered - 2;
	if (temp_direction < 0) temp_direction += 4;
	point_towards(temp_direction);

	attempt_move(curNode);

	//Go backwards (opposite of direction entered)
	return 0;
}

int explore_forward(Node *curNode){
	switch ( curNode->direction_entered ) {
		case 0: //N
			//TODO: Try moving and see what happens
			if(( curNode->row - 1) >= 0) {
				curNode = maze[curNode->row - 1][ curNode->col];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 1: //E
			if(( curNode->col + 1) < COLS) {
				curNode = maze[curNode->row][ curNode->col + 1];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 2: //S
			if(( curNode->row + 1) < ROWS) {
				curNode = maze[curNode->row + 1][ curNode->col];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 3: //W
			if(( curNode->col - 1) >= 0) {
				curNode = maze[curNode->row][ curNode->col - 1];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		default:
			break;
	}
}

int explore_left(Node *curNode){
	switch ( curNode->direction_entered ) {
		case 0: //N
			if(( curNode->col - 1) >= 0) {
				curNode = maze[curNode->row][ curNode->col - 1];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 1: //E
			if(( curNode->row - 1) >= 0) {
				curNode = maze[curNode->row - 1][ curNode->col];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 2: //S
			if(( curNode->col + 1) < COLS) {
				curNode = maze[curNode->row][ curNode->col + 1];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 3: //W
			if(( curNode->row + 1) < ROWS) {
				curNode = maze[curNode->row + 1][ curNode->col];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		default:
			break;
	}
	
}

int explore_right(Node *curNode){
	switch ( curNode->direction_entered ) {
		case 0: //N
			if(( curNode->col + 1) < COLS) {
				curNode = maze[curNode->row][ curNode->col + 1];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 1: //E
			if(( curNode->row + 1) < ROWS) {
				curNode = maze[curNode->row + 1][ curNode->col];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 2: //S
			if(( curNode->col - 1) >= 0) {
				curNode = maze[curNode->row][ curNode->col - 1];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		case 3: //W
			if(( curNode->row - 1) >= 0) {
				curNode = maze[curNode->row - 1][ curNode->col];
				curNode->direction_entered = direction;
				curNode->visited = 1;
				explore_Node(curNode);
			}
			break;
		default:
			break;
	}
	
}


int forward_open(Node * curNode) {
	int rowchk = curNode->row;
	int colchk = curNode->col;

	switch ( curNode->direction_entered ) {
		case 0: //N
			rowchk--;
			if ( curNode->wall_N ) return 0;
			break;
		case 1: //E
			colchk++;
			if ( curNode->wall_E ) return 0;
			break;
		case 2: //S
			rowchk++;
			if ( curNode->wall_S ) return 0;
			break;
		case 3: //W
			colchk--;
			if ( curNode->wall_W ) return 0;
			break;
		default:
			break;
	}

	//CHECK IF NODE IN TAHT DIRECTION IS VISITED
	if (maze[rowchk][colchk]->visited) return 0;

	return 1;
}

int left_open(Node * curNode) {
	int rowchk = curNode->row;
	int colchk = curNode->col;

	switch ( curNode->direction_entered ) {
		case 0: //N
			colchk--;
			if ( curNode->wall_W ) return 0;
			break;
		case 1: //E
			rowchk--;
			if ( curNode->wall_N ) return 0;
			break;
		case 2: //S
			colchk++;
			if ( curNode->wall_E ) return 0;
			break;
		case 3: //W
			rowchk++;
			if ( curNode->wall_S ) return 0;
			break;
		default:
			break;
	}

	//CHECK IF NODE IN TAHT DIRECTION IS VISITED
	if (maze[rowchk][colchk]->visited) return 0;

	return 1;
}

int right_open(Node * curNode) {
	int rowchk = curNode->row;
	int colchk = curNode->col;

	switch ( curNode->direction_entered ) {
		case 0: //N
			colchk++;
			if ( curNode->wall_E ) return 0;
			break;
		case 1: //E
			rowchk++;
			if ( curNode->wall_S ) return 0;
			break;
		case 2: //S
			colchk--;
			if ( curNode->wall_W ) return 0;
			break;
		case 3: //W
			rowchk--;
			if ( curNode->wall_N ) return 0;
			break;
		default:
			break;
	}

	//CHECK IF NODE IN TAHT DIRECTION IS VISITED
	if (maze[rowchk][colchk]->visited) return 0;

	return 1;
}

void point_towards(short dir) {

	switch ( direction ) {
		case 0:
			switch ( dir ) {
				case 0:
					break;
				case 1:
					turn(100, -1, -CW90, 0);
					break;
				case 2:
					turn(100, -1, -CW180, 0);
					break;
				case 3:
					turn(100, 1, CCW90, 0);
					break;
			}
			break;
		case 1:
			switch ( dir ) {
				case 0:
					turn(100, 1, CCW90, 0);
					break;
				case 1:
					break;
				case 2:
					turn(100, -1, -CW90, 0);
					break;
				case 3:
					turn(100, -1, -CW180, 0);
					break;
			}
			break;
		case 2:
			switch ( dir ) {
				case 0:
					turn(100, -1, -CW180, 0);
					break;
				case 1:
					turn(100, 1, CCW90, 0);
					break;
				case 2:
					break;
				case 3:
					turn(100, -1, -CW90, 0);
					break;
			}
			break;
		case 3:
			switch ( dir ) {
				case 0:
					turn(100, -1, -CW90, 0);
					break;
				case 1:
					turn(100, -1, -CW180, 0);
					break;
				case 2:
					turn(100, 1, CCW90, 0);
					break;
				case 3:
					break;
			}
			break;
		default:
			break;
	}

	direction = dir;
}

int attempt_move(Node * curNode) {

	//CHECK IN FRONT
	directDrive(LEFT_SPEED,RIGHT_SPEED);
	dist = waitDistance(UNIT_DIST,1);
	bumpsense = readSensor(SENSOR_BUMPS_AND_WHEEL_DROPS);
	directDrive(0,0);
	waitTime(0.2);
	check_bomb();
	//TODO: Do we want this check_bomb?
	
	printf("bumpsense: %d\n",bumpsense);
	switch( bumpsense & 0x03 ){

		//Right bumper
		case 1:
			directDrive(-50,-150);
			waitDistance(-60,0);
			directDrive(-150,-50);
			waitDistance(-110,0);

			directDrive(LEFT_SPEED,RIGHT_SPEED);
			waitDistance(CORRECTION - dist,1);	// Make up the distance remaining
			check_bomb();
			//TODO: Do we want this check_bomb?

			directDrive(0,0);
			waitTime(0.2);
			return 1;
			break;

		//Left bumper
		case 2:
			directDrive(-150,-50);
			waitDistance(-60,0);
			directDrive(-50,-150);
			waitDistance(-110,0);

			directDrive(LEFT_SPEED,RIGHT_SPEED);
			waitDistance(CORRECTION - dist,1);
			// Make up the distance remaining
			check_bomb();
			//TODO: Do we want this check_bomb?

			directDrive(0,0);
			waitTime(0.2);
			return 1;
			break;
			
		//Bumped both
		case 3:
			printf("back up\n");
			directDrive(-150,-150);
			waitDistance(-100,0);
			directDrive(0,0);
			
			switch ( direction ) {
				case 0: //N
					curNode->wall_N = 1;
					if ((curNode->row - 1) >= 0) maze[curNode->row - 1][curNode->col]->wall_S = 1;
					break;
				case 1: //E
					curNode->wall_E= 1;
					if ((curNode->col + 1) < COLS) maze[curNode->row][curNode->col + 1]->wall_W = 1;
					break;

				case 2: //S
					curNode->wall_S = 1;
					if ((curNode->row + 1) < ROWS) maze[curNode->row + 1][curNode->col]->wall_N = 1;
					break;
				case 3: //W
					curNode->wall_W = 1;
					if ((curNode->col - 1) >= 0) maze[curNode->row][curNode->col - 1]->wall_E = 1;
					break;
				default:
					break;
			}
			// Set walls, in current node as well as neighbor
			
			waitTime(0.2);
			return 0;
			break;

		default:
			break;
	}

	return 1; // Move forward successful
}

void check_bomb(void){
	
	waitTime(.2);
	read_all_sensors();
	if (cl > 500 && cfl > 500 && cr > 500 && cfr > 500 && foundflag == 0) {
		foundflag = 1;
  		playSong(0);
		waitTime(2);
	}
}

void read_all_sensors() {
	cl = readSensor(SENSOR_CLIFF_LEFT_SIGNAL);
	cfl = readSensor(SENSOR_CLIFF_FRONT_LEFT_SIGNAL);
	cr = readSensor(SENSOR_CLIFF_RIGHT_SIGNAL);
	cfr = readSensor(SENSOR_CLIFF_FRONT_RIGHT_SIGNAL);
}