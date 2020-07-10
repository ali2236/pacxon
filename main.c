//
//   Ali Ghanbari
//     - pacxon -
//
/**
* TODO:
*   - Add levels and progression
*/
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wincon.h>
#include <time.h>
#define X 20 //65
#define Y 10 //20

// enums!
enum objectType {
    PACMAN,
    BOUNCYGHOST,
    BLOCKGHOST,
    INVERTEDBOUNCYGHOST
};
enum directions {
    N,NE,E,SE,S,SW,W,NW,null
};
enum colors {
    BLACK,
    BLUE,
    GREEN,
    CYAN,
    RED,
    MAGENTA,
    BROWN,
    LIGHTGRAY,
    DARKGRAY,
    LIGHTBLUE,
    LIGHTGREEN,
    LIGHTCYAN,
    LIGHTRED,
    LIGHTMAGENTA,
    YELLOW,
    WHITE
};

//typedefs
typedef struct {
    short x;
    short y;
} Vector2;

typedef struct {
    char shape;
    short color;
} StaticGameObject;

typedef struct GameObject {
    char shape;
    short color;
    enum objectType type;
    enum directions dir;
    Vector2 position;
    short speed; // lower is better . min is 1
    short active;
    void (*update)(struct GameObject*);
} DynamicGameObject;

// player data
short level = 1;
short maxLevel = 4;
short lives = 4;
short entitesCount = 2;
long score = 0;
short progress = 0;
int isRunning = 1;
int isLost = 0;
int isGoingToNextLevel = 0;
int sleep = 200;
int autoDebug = 0;
int debug = 0;
long frames = 0;

// Arrays
DynamicGameObject *entities = NULL;
StaticGameObject render_buffer[Y][X];
short bricks[Y][X];

// Shapes
char pacman_shape = 'C';
char ghost_shape = 'A';

// game shapes
DynamicGameObject pacMan;
StaticGameObject brick;
StaticGameObject nothing;
DynamicGameObject bouncyGhost;
DynamicGameObject blockGhost;
DynamicGameObject invertedBouncyGhost;

// functions
void makeTypes();
void putInitialBricks();
void updateRenderBuffer();
void SetColor(short colorId);
void instantiateEntities();
void updateEntities();
void levelManager();
DynamicGameObject CloneDynamicGameObject(DynamicGameObject GameObject);



int main() {
    levelManager();
    return 0;
}

/////////////////// game
void levelManager() {
    //start the initial game instance
    game();
    do {
        if(isLost) {
            break;
        } else if(isGoingToNextLevel) {
            game();
        }
    } while(isRunning);
    SetColor(WHITE);
    system("cls");
    if(isGoingToNextLevel>maxLevel) {
        printf("YOU WON THE GAME!");
    } else if(isLost) {
        printf("YOU LOST");
    }
    Sleep(2000);
    _getch();
}

void game() {
    srand(time(NULL));
    makeTypes();
    putInitialBricks();
    instantiateEntities();
    isRunning = 1;
    while(isRunning) {
        system("cls");
        updateRenderBuffer();
        printScreen();
        updateEntities();
        frames++;
        Sleep(sleep);
    }
}

void lose() {
    if(lives>0) {
        lives--;
        entities[0] = CloneDynamicGameObject(pacMan);
        int i,j;
        //removeAll 2's
        for(i=1; i<Y-1; i++) {
            for(j=1; j<X-1; j++) {
                if(bricks[i][j]==2) {
                    bricks[i][j] = 0;
                }
            }
        }
    } else {
        isRunning = 0;
        isLost = 1;
    }
}

void win() {
    level++;
    //restart
    if(level<=maxLevel) {
        isGoingToNextLevel = 1;
        isRunning = 0;
    } else {
        isRunning = 0;
    }
}

void calculateProgress() {
    int i,j;
    int count;
    int filled;
    for(i=1; i<Y-1; i++) {
        for(j=1; j<X-1; j++) {
            count++;
            if(bricks[i][j]==1) {
                filled++;
            }
        }
    }
    progress = ((float)filled/count)*100;
}
///////////////////

/////////////////// filling
enum directions getDirection(Vector2 block,short type,int inverted) {
    if(bricks[block.y-1][block.x]==type) { //check top
        return inverted?S:N;
    } else if(bricks[block.y+1][block.x]==type) { //check bottom
        return inverted?N:S;
    } else if(bricks[block.y][block.x+1]==type) { //check right
        return inverted?W:E;
    } else if(bricks[block.y][block.x-1]==type) { //check left
        return inverted?E:W;
    } else {
        return null;
    }
};

enum directions getDirection8(Vector2 block,short type,int inverted) {
    if(block.x<0 || block.x>X-1 || block.y<0 || block.y>Y-1) {
        return null;
    }
    enum directions d = getDirection(block,type,inverted);
    if(d!=null) {
        return d;
    }
    if(bricks[block.y-1][block.x+1]==type) { //check NE
        return inverted?SW:NE;
    } else if(bricks[block.y+1][block.x+1]==type) { //check SE
        return inverted?NW:SE;
    } else if(bricks[block.y+1][block.x-1]==type) { //check SW
        return inverted?NE:SW;
    } else if(bricks[block.y-1][block.x-1]==type) { //check NW
        return inverted?SE:NW;
    } else {
        return null;
    }
};

void labelAreas(Vector2 position,enum directions direction,short label) {
    while(bricks[position.y][position.x]!=1) {
        if(bricks[position.y][position.x] != 2) {
            bricks[position.y][position.x] = label;
        }
        if(direction==N) {
            position.y = position.y - 1;
        } else if(direction==E) {
            position.x = position.x + 1;
        } else if(direction==S) {
            position.y = position.y + 1;
        } else if(direction==W) {
            position.x = position.x - 1;
        } else if(direction==null) {
            bricks[position.y][position.x] = label;
        }
        if(bricks[position.y][position.x]==2) {
            break;
        }
    }
    if(debug) {
        system("cls");
        printScreen();
    }
}

void fillMap(Vector2 lineEnd) {
    // find the start of the line
    enum directions direction;
    short area1 = 7,area2 = 9;
    Vector2 block = lineEnd;
    direction = getDirection(block,1,1);
    while(block.x!=NULL) {
        //label areas
        if(direction==N) {
            labelAreas(block,E,area1);
            labelAreas(block,W,area2);
            labelAreas(block,null,1);
        } else if(direction==E) {
            labelAreas(block,S,area1);
            labelAreas(block,N,area2);
            labelAreas(block,null,1);
        } else if(direction==S) {
            labelAreas(block,W,area1);
            labelAreas(block,E,area2);
            labelAreas(block,null,1);
        } else if(direction==W) {
            labelAreas(block,N,area1);
            labelAreas(block,S,area2);
            labelAreas(block,null,1);
        }
        //get the next block
        if(bricks[block.y-1][block.x]==2 || bricks[block.y-1][block.x]==4) { //check top
            block = (Vector2) {
                block.x,block.y-1
            };
            direction = N;
        } else if(bricks[block.y][block.x+1]==2 || bricks[block.y][block.x+1]==4) { //check right
            block = (Vector2) {
                block.x+1,block.y
            };
            direction = E;
        } else if(bricks[block.y+1][block.x]==2 || bricks[block.y+1][block.x]==4) { //check bottom
            block = (Vector2) {
                block.x,block.y+1
            };
            direction = S;
        } else if(bricks[block.y][block.x-1]==2 || bricks[block.y][block.x-1]==4) { //check left
            block = (Vector2) {
                block.x-1,block.y
            };
            direction = W;
        } else {
            block.x = NULL;
        }
    }
    //label the unlabeled
    int i,j;
    //left to right
    for(i=1; i<Y-1; i++) {
        for(j=1; j<X-1; j++) {
            if(bricks[i][j]==0) {
                if(getDirection((Vector2) {
                j,i
            },area1,1)!=null) {
                    bricks[i][j] = area1;
                } else if(getDirection((Vector2) {
                j,i
            },area2,1)!=null) {
                    bricks[i][j] = area2;
                }
            }

        }
        if(debug) {
            system("cls");
            printScreen();
            Sleep(10);
        }
    }
    //right to left
    for(i=Y-2; i>0; i--) {
        for(j=X-2; j>0; j--) {
            if(bricks[i][j]==0) {
                if(getDirection((Vector2) {
                j,i
            },area1,1)!=null) {
                    bricks[i][j] = area1;
                } else if(getDirection((Vector2) {
                j,i
            },area2,1)!=null) {
                    bricks[i][j] = area2;
                }
            }

        }
        if(debug) {
            system("cls");
            printScreen();
            Sleep(10);
        }
    }
    // flag areas
    short flag1=1,flag2=1;
    for(int k=1; k<entitesCount; k++) {
        DynamicGameObject entity = entities[k];
        if(entity.type==BOUNCYGHOST) {
            short onTopOf = bricks[entity.position.y][entity.position.x];
            if(onTopOf==area1) {
                flag1=0;
            } else if(onTopOf==area2) {
                flag2=0;
            }
        } else if(entity.type==BLOCKGHOST) {
            Vector2 pos = entities[k].position;
            if(getDirection8(pos,area1,0)!=null) {
                flag1=0;
            } else if(getDirection8(pos,area2,0)!=null) {
                flag2=0;
            }
        }
    }

    //clean labels
    for(i=1; i<Y; i++) {
        for(j=1; j<X; j++) {
            short b = bricks[i][j];
            if(b==2) {
                bricks[i][j]=1;
            } else if(b==area1) {
                if(flag1) {
                    bricks[i][j]=1;
                } else {
                    bricks[i][j]=0;
                }
            } else if(b==area2) {
                if(flag2) {
                    bricks[i][j]=1;
                } else {
                    bricks[i][j]=0;
                }
            }
        }
        if(debug) {
            system("cls");
            printScreen();
            Sleep(10);
        }
    }
}
///////////////////

/////////////////// moving
void move_pacman(int y,int x,DynamicGameObject *pacman) {
    Vector2 pos = pacman->position;
    pos.x += x;
    pos.y += y;
    if(pos.x>=0 && pos.x<X && pos.y>=0 && pos.y<Y) {
        if(bricks[pos.y][pos.x]==1) {
            if(bricks[pacman->position.y][pacman->position.x]==2) { //splited map
                Vector2 p = {pacman->position.x,pacman->position.y};
                if(autoDebug) {
                    debug = 1;
                }
                fillMap(p);
                calculateProgress();
                if(progress>=80) {
                    win();
                }
                if(autoDebug) {
                    debug = 0;
                }
            }
            pacman->position = pos;
        } else if(bricks[pos.y][pos.x]==2) {
            if(bricks[pacman->position.y][pacman->position.x]==2) {
                // do nothing
            } else {
                lose();
            }
        } else {
            bricks[pos.y][pos.x] = 2;
            pacman->position = pos;
        }
    }
}

Vector2 randomGhostPosition(enum objectType type) {
    Vector2 v2;
    if(type == BOUNCYGHOST) {
        v2.x = (rand()%(X-2))+1;
        v2.y = (rand()%(Y-2))+1;
        return v2;
    } else if(type == BLOCKGHOST) {
        if(rand()%2) { //on right side |
            v2.x = X-1;
            v2.y = (rand()%(Y-3))+1;
        } else { //on bottom side _
            v2.x = (rand()%(X-3))+1;
            v2.y = Y-1;
        }
        return v2;
    }
}

enum directions randomGhostRotation() {
    int r = rand()%4;
    switch (r) {
    case 0:
        return NE;
    case 1:
        return SE;
    case 2:
        return SW;
    case 3:
        return NW;
    default:
        return NE;
    }
};
///////////////////

/////////////////// AI
void pacman_update(DynamicGameObject *pacman) {
    while(_kbhit()) {
        char c = _getch();
        c = _getch();
        if(c==77) { //right
            move_pacman(0,1,pacman);
        } else if(c==75) { // left
            move_pacman(0,-1,pacman);
        } else if(c==72) { // up
            move_pacman(-1,0,pacman);
        } else if(c==80) { //down
            move_pacman(1,0,pacman);
        }
        for(int k=1; k<entitesCount; k++) {
            if(entities[k].position.x==pacman->position.x &&
                    entities[k].position.y==pacman->position.y) {
                lose();
            }
        }
    }
}

void bouncy_AI(DynamicGameObject *bouncyghost) {
    if(bouncyghost->dir==NE) {
        short block = bricks[bouncyghost->position.y-1][bouncyghost->position.x+1];
        if(block==2) {
            lose();
        } else if(block==1) {
            //Bounce
            enum directions d = getDirection((bouncyghost->position),1,0);
            if(d==E) {
                bouncyghost->dir=NW;
            } else if(d==N) {
                if(bricks[bouncyghost->position.y][bouncyghost->position.x+1]==1) {
                    bouncyghost->dir=SW;
                } else {
                    bouncyghost->dir=SE;
                }
            } else if(d==null) {
                bouncyghost->dir=randomGhostRotation();
            }
        } else if(block==0) {
            //MOVE
            bouncyghost->position.x++;
            bouncyghost->position.y--;
        }
    } else if(bouncyghost->dir==SE) {
        short block = bricks[bouncyghost->position.y+1][bouncyghost->position.x+1];
        if(block==2) {
            lose();
        } else if(block==1) {
            //Bounce
            enum directions d = getDirection((bouncyghost->position),1,0);
            if(d==E) {
                bouncyghost->dir=SW;
            } else if(d==S) {
                if(bricks[bouncyghost->position.y][bouncyghost->position.x+1]==1) {
                    bouncyghost->dir=NW;
                } else {
                    bouncyghost->dir=NE;
                }
            } else if(d==null) {
                bouncyghost->dir=randomGhostRotation();
            }
        } else if(block==0) {
            //MOVE
            bouncyghost->position.x++;
            bouncyghost->position.y++;
        }
    } else if(bouncyghost->dir==SW) {
        short block = bricks[bouncyghost->position.y+1][bouncyghost->position.x-1];
        if(block==2) {
            lose();
        } else if(block==1) {
            //Bounce
            enum directions d = getDirection((bouncyghost->position),1,0);
            if(d==W) {
                bouncyghost->dir=SE;
            } else if(d==S) {
                if(bricks[bouncyghost->position.y][bouncyghost->position.x-1]==1) {
                    bouncyghost->dir=NE;
                } else {
                    bouncyghost->dir=NW;
                }
            } else if(d==null) {
                bouncyghost->dir=randomGhostRotation();
            }
        } else if(block==0) {
            //MOVE
            bouncyghost->position.x--;
            bouncyghost->position.y++;
        }
    } else if(bouncyghost->dir==NW) {
        short block = bricks[bouncyghost->position.y-1][bouncyghost->position.x-1];
        if(block==2) {
            lose();
        } else if(block==1) {
            //Bounce
            enum directions d = getDirection((bouncyghost->position),1,0);
            if(d==W) {
                bouncyghost->dir=NE;
            } else if(d==N) {
                if(bricks[bouncyghost->position.y][bouncyghost->position.x-1]==1) {
                    bouncyghost->dir=SE;
                } else {
                    bouncyghost->dir=SW;
                }
            } else if(d==null) {
                bouncyghost->dir=randomGhostRotation();
            }
        } else if(block==0) {
            //MOVE
            bouncyghost->position.x--;
            bouncyghost->position.y--;
        }
    }
}

void block_AI(DynamicGameObject *blockghost) {
    short x = blockghost->position.x;
    short y = blockghost->position.y;
    enum directions d = blockghost->dir;
    //check left
    if(d==N) {
        if(bricks[y][x-1]==1) {
            blockghost->dir = W;
        }
    } else if(d==E) {
        if(bricks[y-1][x]==1) {
            blockghost->dir = N;
        }
    } else if(d==S) {
        if(bricks[y][x+1]==1) {
            blockghost->dir = E;
        }
    } else if(d==W) {
        if(bricks[y+1][x]==1) {
            blockghost->dir = S;
        }
    }
    //move
    d = blockghost->dir;
    if(d==N) {
        blockghost->position.y--;
    } else if(d==E) {
        blockghost->position.x++;
    } else if(d==S) {
        blockghost->position.y++;
    } else if(d==W) {
        blockghost->position.x--;
    }

    //check for PACMAN
    x = blockghost->position.x;
    y = blockghost->position.y;
    if(entities[0].position.x == x && entities[0].position.y == y) {
        lose();
    }
}

void inverted_bouncy_AI(DynamicGameObject *ghost) {
    if(progress >= 5) {
        //check is spawned
        if(ghost->active==0) {
            short x;
            short y;
            do {
                x = (rand()%(X-3))+1;
                y = (rand()%(Y-3))+1;
            } while(bricks[y][x]!=1);
            ghost->position = (Vector2) {
                x,y
            };
            ghost->active = 1;
            return;
        }
    }
    //MOVE
    short x = ghost->position.x;
    short y = ghost->position.y;
    short canMove = 0;
    enum directions d = ghost->dir;
    if(ghost->dir==NE) {
        x++;
        y--;
        if(x>X-1 || y<0) {
            if(x>X-1 && y<0) {
                d = SW;
            } else if(x>X-1) {
                d = NW;
            } else if(y<0) {
                d = SE;
            }
        } else {
            short block = bricks[y][x];
            if(block==0) {
                //bounce
                enum directions _d = getDirection((Vector2) {
                    x,y
                },0,0);
                if(_d==N) {
                    d = SE;
                } else if(_d==E) {
                    d = NW;
                } else {
                    d = SW;
                }
            } else if(block==1) {
                //MOVE
                canMove = 1;
            } else if(block==2) {
                lose();
            }
        }
    }
    if(ghost->dir==SE) {
        x++;
        y++;
        if(x>X-1 || y>Y-1) {
            if(x>X-1 && y>Y-1) {
                d = NW;
            } else if(x>X-1) {
                d = SW;
            } else if(y>Y-1) {
                d = NE;
            }
        } else {
            short block = bricks[y][x];
            if(block==0) {
                //bounce
                enum directions _d = getDirection((Vector2) {
                    x,y
                },0,0);
                if(_d==S) {
                    d = NE;
                } else if(_d==E) {
                    d = SW;
                } else {
                    d = NW;
                }
            } else if(block==1) {
                //MOVE
                canMove = 1;
            } else if(block==2) {
                lose();
            }
        }
    }
    if(ghost->dir==SW) {
        x--;
        y++;
        if(x<0 || y>Y-1) {
            if(x<0 && y>Y-1) {
                d = NE;
            } else if(x<0) {
                d = SE;
            } else if(y>Y-1) {
                d = NW;
            }
        } else {
            short block = bricks[y][x];
            if(block==0) {
                //bounce
                enum directions _d = getDirection((Vector2) {
                    x,y
                },0,0);
                if(_d==S) {
                    d = NW;
                } else if(_d==W) {
                    d = SE;
                } else {
                    d = NE;
                }
            } else if(block==1) {
                //MOVE
                canMove = 1;
            } else if(block==2) {
                lose();
            }
        }
    }
    if(ghost->dir==NW) {
        x--;
        y--;
        if(x<0 || y<0) {
            if(x<0 && y<0) {
                d = SE;
            } else if(x<0) {
                d = NE;
            } else if(y<0) {
                d = SW;
            }
        } else {
            short block = bricks[y][x];
            if(block==0) {
                //bounce
                enum directions _d = getDirection((Vector2) {
                    x,y
                },0,0);
                if(_d==N) {
                    d = SW;
                } else if(_d==W) {
                    d = NE;
                } else {
                    d = SE;
                }
            } else if(block==1) {
                //MOVE
                canMove = 1;
            } else if(block==2) {
                lose();
            }
        }
    }
    if(canMove) {
        ghost->position.x = x;
        ghost->position.y = y;
    }
    ghost->dir = d;
}
///////////////////

void makeTypes() {
    //PAC man
    pacMan.shape = pacman_shape;
    pacMan.color = YELLOW;
    pacMan.position = (Vector2) {
        0,0
    };
    pacMan.type = PACMAN;
    pacMan.speed = 1;
    pacMan.active = 1;
    pacMan.update = pacman_update;
    //Brick
    brick.shape = '#';
    brick.color = 1;
    //Nothing
    nothing.shape = ' ';
    nothing.color = 0;
    //Bouncy Ghost
    bouncyGhost.shape = ghost_shape;
    bouncyGhost.color = LIGHTRED;
    bouncyGhost.type = BOUNCYGHOST;
    bouncyGhost.speed = 2;
    bouncyGhost.active = 1;
    bouncyGhost.update = bouncy_AI;
    //BlockGhost
    blockGhost.shape = ghost_shape;
    blockGhost.color = LIGHTMAGENTA;
    blockGhost.type = BLOCKGHOST;
    blockGhost.speed = 2;
    blockGhost.active = 1;
    blockGhost.update = block_AI;
    //InvertedBouncyGhost
    invertedBouncyGhost.shape = ghost_shape;
    invertedBouncyGhost.color = LIGHTCYAN;
    invertedBouncyGhost.type = INVERTEDBOUNCYGHOST;
    invertedBouncyGhost.speed = 2;
    invertedBouncyGhost.active = 0;
    invertedBouncyGhost.position = (Vector2) {
        100,100
    };
    invertedBouncyGhost.update = inverted_bouncy_AI;
}



void printScreen() {
    SetColor(15);
    printf("lives:%d\tscore:%d\tprogress:%d%%/100%%\n",lives,score,progress);
    int i,j;
    for(i=0; i<Y; i++) {
        for(j=0; j<X; j++) {
            if(debug) {
                SetColor(15);
                printf("%d",bricks[i][j]);
            } else {
                StaticGameObject g = render_buffer[i][j];
                SetColor(g.color);
                printf("%c",g.shape);
            }
        }
        printf("\n");
    }
}

void putInitialBricks() {
    int i,j;
    for(i=0; i<Y; i++) {
        for(j=0; j<X; j++) {
            if(i==0 || j==0 || i==Y-1 || j==X-1) {
                bricks[i][j] = 1;
            } else {
                bricks[i][j] = 0;
            }
        }
    }
}

StaticGameObject DynamicToStaticGameObject(DynamicGameObject g) {
    StaticGameObject s;
    s.shape = g.shape;
    s.color = g.color;
    return s;
}

void updateRenderBuffer() {
    int i,j;
    // put bricks
    for(i=0; i<Y; i++) {
        for(j=0; j<X; j++) {
            if(bricks[i][j]==1 || bricks[i][j]==2) {
                render_buffer[i][j] = brick;
            } else {
                render_buffer[i][j] = nothing;
            }
        }
    }
    // put entities
    for(i=0; i<entitesCount; i++) {
        if(entities[i].active) {
            render_buffer[entities[i].position.y][entities[i].position.x] = DynamicToStaticGameObject(entities[i]);
        }
    }
}

void SetColor(short colorId) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),colorId);
}

//clone
DynamicGameObject CloneDynamicGameObject(DynamicGameObject GameObject) {
    return GameObject;
}

enum directions getBlockGhostDirection(DynamicGameObject *blockghost) {
    int x = blockghost->position.x;
    int y = blockghost->position.y;
    short left = x>0?bricks[y][x-1]:NULL;
    if(left==1) {
        return E;
    } else if(left==0) {
        return N;
    }
};

void getLevelEntityCount(int *bouncy,int *blocky,int *ibouncy) {
    switch (level) {
    case 0:
        *bouncy = 1;
        break;
    case 1:
        *bouncy = 2;
        break;
    case 2:
        *bouncy = 2;
        *blocky = 1;
        break;
    case 3:
        *bouncy = 2;
        *ibouncy = 1;
        break;
    case 4:
        *bouncy = 2;
        *blocky = 1;
        *ibouncy = 1;
        break;
    default:
        *bouncy = 2;
        break;
    }
}

void instantiateEntities() {
    //set entities count
    if(entities != NULL) {
        free(entities);
    }
    int i,index=0;
    int bouncyGhost_count = 0;
    int blockGhost_count = 0;
    int invertedbouncyGhost_count = 0;

    getLevelEntityCount(&bouncyGhost_count,&blockGhost_count,&invertedbouncyGhost_count);

    entitesCount = 1 + bouncyGhost_count + blockGhost_count + invertedbouncyGhost_count;
    //Allocate Memory
    entities = malloc(entitesCount*sizeof(DynamicGameObject));
    //PACMAN
    entities[index++] = CloneDynamicGameObject(pacMan);
    //BOUNCYGHOST
    for(i=0; i<bouncyGhost_count; i++) {
        DynamicGameObject g = CloneDynamicGameObject(bouncyGhost);
        g.position = randomGhostPosition(BOUNCYGHOST);
        g.dir = randomGhostRotation();
        entities[index++] = g;
    }
    //BLOCKGHOST
    for(i=0; i<blockGhost_count; i++) {
        DynamicGameObject g = CloneDynamicGameObject(blockGhost);
        g.position = randomGhostPosition(BLOCKGHOST);
        g.dir = getBlockGhostDirection(&g);
        entities[index++] = g;
    }
    //INVERTEDBOUNCYGHOST
    for(i=0; i<invertedbouncyGhost_count; i++) {
        DynamicGameObject g = CloneDynamicGameObject(invertedBouncyGhost);
        g.dir = randomGhostRotation(BOUNCYGHOST);
        entities[index++] = g;
    }
}

void updateEntities() {
    for(int i=0; i<entitesCount; i++) {
        if(entities[i].update != NULL) {
            if(frames%entities[i].speed==0) {
                entities[i].update(&entities[i]);
            }
        }
    }
}