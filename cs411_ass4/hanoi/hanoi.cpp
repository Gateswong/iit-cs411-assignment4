// ============================================================================
// CS411 - Assignment #4
// Spline Curves (Towers of Hanoi)
//
// Lu Wang
// A20315534
//
// This code is based on the code from professor.
//
// Tab Size: 4
// ============================================================================
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include "GL/glut.h"

// ============================================================================
// Data Structure
// ============================================================================

// 3D Vector
typedef struct {
	float x, y, z;
} Vec3d;

// Piece of ring on rods
typedef struct {
	Vec3d pos;			// piece position
	Vec3d dir;			// piece orientation
} Piece;

// A piece which is moving from one rod to another
typedef struct {
	int index;			// piece index
	int inMotion;		// in motion flag
	float u;			// location parameter in [0, 1]
	Vec3d startPos;		// starting position
	Vec3d targetPos;	// target position
} ActivePiece;

// Game Context
typedef struct {
	int curW;			// Current window width
	int curH;			// Current window height
	int score;			// Number of moves
	int prevTime;		// the previous rendering time
	float stepSize;		// Parameter step size on the path
	int fps;
} Context;

#define MAX_PIECES 20

// Game Board
typedef struct {
	float r;			// base disk radius
	float xMin, yMin,
		xMax, yMax;		// base plate coordinates
	Vec3d pos[3][MAX_PIECES];
						// Possible piece positions on board
						// pos[rod, stack]
	int occupancy[3][3];
} Board;

Piece piece[MAX_PIECES];
Board board;
ActivePiece activePiece;
Context C;

// ============================================================================
// Game Initialization
// ============================================================================

void initGame()
{
	int rod,stack,i;

	// initialize the board
	board.r = 1.0;
	board.xMin = 0.0; board.yMin = 0.0;
	board.xMax = 6.0 * board.r; board.yMax = 2.0 * board.r;
	float xc = (board.xMax-board.xMin)/2.0, yc = (board.yMax-board.yMin)/2.0;
	float dx = (board.xMax-board.xMin)/3.0;
	float r = board.r;

	// initialize the occupancy matrix
	for (rod = 0; rod < 3; rod ++) {
		for(stack = 0; stack < 3; stack ++) {
			board.occupancy[rod][stack]= -1;
		}
	}
	for(stack = 0; stack < 3; stack ++) {
		board.occupancy[0][stack] = 2 - stack; 
	}

	// initialize the board position matrix
	for (rod = 0; rod < 3; rod ++) {
		for(stack = 0; stack < 3; stack ++) {
			board.pos[rod][stack].x = r + rod*dx;
			board.pos[rod][stack].y = yc;
			board.pos[rod][stack].z = (stack + 1)*0.3;
		}
	}

	// initialize the piece positions
	for(i = 0; i < 3; i ++) {
		piece[i].pos.x = board.pos[0][2-i].x; 
		piece[i].pos.y = board.pos[0][2-i].y; 
		piece[i].pos.z = board.pos[0][2-i].z; 
	}

	// initialize the piece orientations
	for(i = 0; i < 3; i ++){
		piece[i].dir.x = 0.0;
		piece[i].dir.y = 0.0;
		piece[i].dir.z = 1.0;
	}

	// initialize the active piece
	activePiece.index = -1;
	activePiece.inMotion = 0;
	activePiece.u = 0;
}

void init(void) 
{
	GLfloat light_position[] = { 0.0, 0.0, 0.0, 1.0 };
	glClearColor (0.0, 0.0, 0.0, 0.0);
	glShadeModel (GL_SMOOTH);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);

	C.curW = 0;
	C.curH = 0;
	C.score = 0;
	C.stepSize = 0.02;
	C.fps = 50;
	C.prevTime = glutGet(GLUT_ELAPSED_TIME);

	initGame();
}

// ============================================================================
// Drawing function
// ============================================================================

void draw3dDisk(float x, float y, float r, float h)
{
	GLUquadric *quad;
	int slices = 100;
	int stacks = 10;
	quad = gluNewQuadric();
	glPushMatrix();  
	glTranslatef(x, y, 0.0);
	gluCylinder(quad, r, r, h, slices, stacks);
	glTranslatef(0.0, 0.0, h);
	gluDisk(quad, 0.0, r, slices, stacks);
	glPopMatrix();
	gluDeleteQuadric(quad);    
}

void drawBoard(Board &board)
{
	float xc = (board.xMax - board.xMin)/2.0, yc = (board.yMax - board.yMin)/2.0;
	float dx = (board.xMax-board.xMin)/3.0;
	float r = board.r;

	float mat_white[]  = { 1.0, 1.0, 1.0, 1.0 };
	float mat_yellow[] = { 1.0, 1.0, 0.0, 1.0 };

	// draw a plane
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_white);
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
	glBegin(GL_QUADS);
	glNormal3f(0.0,0.0,1.0);
	glVertex3f(board.xMin, board.yMin, 0.0);
	glVertex3f(board.xMax, board.yMin, 0.0);
	glVertex3f(board.xMax, board.yMax, 0.0);
	glVertex3f(board.xMin, board.yMax, 0.0);
	glEnd();

	// draw the rods
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat_yellow);
	draw3dDisk(r+0.0*dx, yc, 0.1*r, 2.0*r);
	draw3dDisk(r+1.0*dx, yc, 0.1*r, 2.0*r);
	draw3dDisk(r+2.0*dx, yc, 0.1*r, 2.0*r);
	draw3dDisk(r+0.0*dx, yc, 0.8*r, 0.1*r);
	draw3dDisk(r+1.0*dx, yc, 0.8*r, 0.1*r);
	draw3dDisk(r+2.0*dx, yc, 0.8*r, 0.1*r);
}

void drawPiece(int index)
{
	int i;
	int slices=100;
	int stacks=10;
	float r,g,b;
	float mat[4]   = { 1.0, 1.0, 1.0, 1.0 };
	float emOn[4]  = { 0.3, 0.3, 0.3, 1.0 };
	float emOff[4] = { 0.0, 0.0, 0.0, 1.0 };
	float radius;

	// determine the piece color
	switch(index){
	case 0: r=1.0; g=0.0; b=0.0; break;
	case 1: r=0.0; g=1.0; b=0.0; break;
	case 2: r=0.0; g=0.0; b=1.0; break;
	default:
		printf("Error: invalid disk index %d\n\7",index);
		exit(0);
	}
	mat[0]=r; mat[1]=g; mat[2]=b;
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat);
	if(index==activePiece.index){
		glMaterialfv(GL_FRONT, GL_EMISSION, emOn);
	}  

	// determine the piece radius
	switch(index){
	case 0: radius=0.2*board.r; break;
	case 1: radius=0.4*board.r; break;
	case 2: radius=0.6*board.r; break;
	default:
		printf("Error: invalid disk index %d\n\7",index);
		exit(0);
	}

	// draw the piece
	glPushMatrix();
	glTranslatef(piece[index].pos.x, piece[index].pos.y, piece[index].pos.z);
	glutSolidTorus(0.2*board.r, radius, stacks, slices);
	glPopMatrix();
	glMaterialfv(GL_FRONT, GL_EMISSION, emOff);
}



// ============================================================================
// Game logic
// ============================================================================

// Move a piece from one rod to another
void movePiece(int fromRod, int toRod)
{
	int i,j;

	// determine the piece index
	if (fromRod==toRod || fromRod<0 || fromRod>2 || toRod<0 || toRod>2) return;
	for(i=2; i>=0 && board.occupancy[fromRod][i] <0; i--);
	if(i<0 || (i==0 && board.occupancy[fromRod][i] <0)) return; // empty pole
	activePiece.startPos.x=board.pos[fromRod][i].x;
	activePiece.startPos.y=board.pos[fromRod][i].y;
	activePiece.startPos.z=board.pos[fromRod][i].z;
	//printf("From rod %d location %d index %d\n",fromRod, i, board.occupancy[fromRod][i]);

	// set the current piece
	activePiece.index = board.occupancy[fromRod][i];
	activePiece.inMotion = 1;
	activePiece.u = 0;

	// determine the target position
	for(j=0; board.occupancy[toRod][j] >=0 && j<=2; j++);
	activePiece.targetPos.x=board.pos[toRod][j].x;
	activePiece.targetPos.y=board.pos[toRod][j].y;
	activePiece.targetPos.z=board.pos[toRod][j].z;

	// update the occupancy matrix
	board.occupancy[fromRod][i]= -1;
	board.occupancy[toRod][j]= activePiece.index;
}

// Calculate the interpolate path for active piece
void interpolatePath(Vec3d startPos, Vec3d targetPos, float u, Vec3d &interpPos)
{
	// TODO replace these lines:
	// ============================================
	interpPos.x = (u)*targetPos.x+(1-u)*startPos.x;
	interpPos.y = (u)*targetPos.y+(1-u)*startPos.y;
	interpPos.z = (u)*targetPos.z+(1-u)*startPos.z;
	// ============================================
}

// ============================================================================
// Utility
// ============================================================================
void help()
{
	printf("\n");
	printf("Keys:\n\n");
	printf("  esc = exit\n");
	printf("  h = display keys help\n");
	printf("\n");
}

// ============================================================================
// OpenGL specifications
// ============================================================================

// OpenGL reshape
void reshape (int w, int h)
{
	C.curW=w; C.curH=h;
	glViewport (0, 0, (GLsizei) w, (GLsizei) h); 
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective(45.0, (GLfloat) w/(GLfloat) h, 0.1, 20.0);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity();
}

// OpenGL display
void display(void)
{
	float xc=(board.xMax-board.xMin)/2.0, yc=(board.yMax-board.yMin)/2.0;
	float dx=(board.xMax-board.xMin)/3.0;
	float r = board.r;
	char str[80];
	int i;

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity ();
	gluLookAt(xc, -8.0*r, 3.0*r,  xc, yc, 3.0,   0.0, 0.0, 1.0);
	drawBoard(board);
	for(i=0; i<3; i++) { drawPiece(i); }
	glFlush();
	glutSwapBuffers();
}

// OpenGL idle
void idle(void)
{
	float u;
	int currentTime = glutGet(GLUT_ELAPSED_TIME);
	int elapsed = currentTime-C.prevTime; // time difference [ms]
	Vec3d interpPos;

	// check if a new frame is required
	if (elapsed < (1000.0/C.fps)) return;

	// update the previous rendering time
	C.prevTime=currentTime;

	// move a piece
	if(activePiece.inMotion==1){
		activePiece.u+=C.stepSize;
		u=activePiece.u;
		if(u>1) u=1;
		if(activePiece.index>=0 && activePiece.index<=2)
			interpolatePath(activePiece.startPos,activePiece.targetPos,u,
			piece[activePiece.index].pos);
		if(u>=1)    
		{
			activePiece.index = -1;
			activePiece.inMotion=0;
			activePiece.u=0.0;
		}
		glutPostRedisplay();
	}
}

// Handle keyboard events
void keyboard(unsigned char key, int x, int y)
{
	switch (key) {
	case 27:
		printf("Total number of moves: %d\n",C.score);
		exit(0);
		break;
	case 'h':
		help();
		break;
	}
}

// Handle mouse events
void mouse(int button,int state,int x, int y)
{
	int i, rod, piece, p, r, fromRod, toRod;

	// ignore mouse input while in motion
	if (activePiece.inMotion) { return; }

	// handle disk pick up
	if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {

		// identify the selected rod
		rod = int(0.5 + x/(C.curW/4.0));
		if (rod == 0) { rod=1; }
		if (rod == 4) { rod=3; }
		rod --;

		// find the top disk on the rod
		for (i = 2; i >= 0 && board.occupancy[rod][i] < 0; i --);
		if (i >= 0 && board.occupancy[rod][i] >= 0) { piece = board.occupancy[rod][i]; }
		else { piece = -1; }

		// activate a piece
		if(activePiece.index < 0 && piece >= 0) { activePiece.index = piece; }

		// deactivate a piece
		else if(activePiece.index == piece) { activePiece.index = -1; }

		// move a piece
		else if(activePiece.index >= 0) {
			// find the rod of the active piece
			for(p = 0; p < 3; p ++) { 
				for(r = 0; r < 3; r ++) { 
					if(board.occupancy[r][p]==activePiece.index) {
						fromRod = r;
					}
				}
			}
			toRod = rod;
			// verify that the move is valid and move
			if((activePiece.index < piece || piece == -1) && !activePiece.inMotion) {
				movePiece(fromRod,toRod);
				C.score ++;
			}
		}
		glutPostRedisplay();
	}
}

// ============================================================================
// Program entrance
// ============================================================================
int main(int argc, char** argv)
{
  // check transferred arguments
  if(argc < 1) {
	printf("\nUsage 3d-hanoi\n\n");
	exit(0);
  }
  help();

  glutInit(&argc, argv); 
  // use a double buffer for animation
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize (500, 500); 
  glutInitWindowPosition (100, 100);
  glutCreateWindow (argv[0]);
  
  init ();
  glutDisplayFunc(display); 
  glutReshapeFunc(reshape);
  glutKeyboardFunc(keyboard);
  glutMouseFunc(mouse);
  glutIdleFunc(idle);
   
  glutMainLoop();
  return 0;
}
