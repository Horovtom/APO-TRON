/*******************************************************************
  Simple program to check LCD functionality on MicroZed
  based MZ_APO board designed by Petr Porazil at PiKRON

  mzapo_lcdtest.c       - main and only file

  (C) Copyright 2004 - 2017 by Pavel Pisa
      e-mail:   pisa@cmp.felk.cvut.cz
      homepage: http://cmp.felk.cvut.cz/~pisa
      work:     http://www.pikron.com/
      license:  any combination GPL, LGPL, MPL or BSD licenses

 *******************************************************************/


#define _POSIX_C_SOURCE 200112L

#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <memory.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <pthread.h>
#include "writing.c"

char *memdev = "/dev/mem";

#define SPILED_REG_BASE_PHYS 0x43c40000
#define SPILED_REG_SIZE      0x00004000
#define NORTH 0
#define EAST 1
#define SOUTH 2
#define WEST 3

#define LINEWIDTH 10
#define SPILED_REG_LED_LINE_o           0x004
#define SPILED_REG_LED_RGB1_o           0x010
#define SPILED_REG_LED_RGB2_o           0x014
#define SPILED_REG_LED_KBDWR_DIRECT_o   0x018

#define SPILED_REG_KBDRD_KNOBS_DIRECT_o 0x020
#define SPILED_REG_KNOBS_8BIT_o         0x024

#define PARLCD_REG_BASE_PHYS 0x43c00000
#define PARLCD_REG_SIZE      0x00004000

#define PARLCD_REG_CMD_o                0x0008
#define PARLCD_REG_DATA_o               0x000C
#define HEIGHT 32
#define WIDTH 48
#define SERVER_PORT 12346
#define CLIENT_PORT 12345
#define BROADCAST_ADDRESS "192.168.202.184"


//--------DISPLAY-----------------------------------
//region variables
unsigned char *parlcd_mem_base;
int player_colours[10] = {0x0000, 0xFFFF, 0xF800, 0x07E0, 0x7800, 0x07FF, 0xFFE0, 0xF800, 0xF800, 0xF800};


//endregion
//region functions
void *map_phys_address(off_t region_base, size_t region_size, int opt_cached) {
    unsigned long mem_window_size;
    unsigned long pagesize;
    unsigned char *mm;
    unsigned char *mem;
    int fd;

    fd = open(memdev, O_RDWR | (!opt_cached ? O_SYNC : 0));
    if (fd < 0) {
        fprintf(stderr, "cannot open %s\n", memdev);
        return NULL;
    }

    pagesize = sysconf(_SC_PAGESIZE);

    mem_window_size = ((region_base & (pagesize - 1)) + region_size + pagesize - 1) & ~(pagesize - 1);

    mm = mmap(NULL, mem_window_size, PROT_WRITE | PROT_READ,
            MAP_SHARED, fd, region_base & ~(pagesize - 1));
    mem = mm + (region_base & (pagesize - 1));

    if (mm == MAP_FAILED) {
        fprintf(stderr, "mmap error\n");
        return NULL;
    }

    return mem;
}

void parlcd_write_cmd(unsigned char *parlcd_mem_base, uint16_t cmd) {
    *(volatile uint16_t *) (parlcd_mem_base + PARLCD_REG_CMD_o) = cmd;
}

void parlcd_write_data(unsigned char *parlcd_mem_base, uint16_t data) {
    *(volatile uint16_t *) (parlcd_mem_base + PARLCD_REG_DATA_o) = data;
}

void parlcd_write_data2x(unsigned char *parlcd_mem_base, uint32_t data) {
    *(volatile uint32_t *) (parlcd_mem_base + PARLCD_REG_DATA_o) = data;
}

void parlcd_delay(int msec) {
    struct timespec wait_delay = {.tv_sec = msec / 1000,
        .tv_nsec = (msec % 1000) * 1000 * 1000};
    clock_nanosleep(CLOCK_MONOTONIC, 0, &wait_delay, NULL);
}

void parlcd_hx8357_init(unsigned char *parlcd_mem_base) {
    // toggle RST low to reset
    /*
        digitalWrite(_rst, HIGH);
        parlcd_delay(50);
        digitalWrite(_rst, LOW);
        parlcd_delay(10);
        digitalWrite(_rst, HIGH);
        parlcd_delay(10);
     */
    parlcd_write_cmd(parlcd_mem_base, 0x1);
    parlcd_delay(30);

#ifdef HX8357_B
    // Configure HX8357-B display
    parlcd_write_cmd(parlcd_mem_base, 0x11);
    parlcd_delay(20);
    parlcd_write_cmd(parlcd_mem_base, 0xD0);
    parlcd_write_data(parlcd_mem_base, 0x07);
    parlcd_write_data(parlcd_mem_base, 0x42);
    parlcd_write_data(parlcd_mem_base, 0x18);

    parlcd_write_cmd(parlcd_mem_base, 0xD1);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x07);
    parlcd_write_data(parlcd_mem_base, 0x10);

    parlcd_write_cmd(parlcd_mem_base, 0xD2);
    parlcd_write_data(parlcd_mem_base, 0x01);
    parlcd_write_data(parlcd_mem_base, 0x02);

    parlcd_write_cmd(parlcd_mem_base, 0xC0);
    parlcd_write_data(parlcd_mem_base, 0x10);
    parlcd_write_data(parlcd_mem_base, 0x3B);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x02);
    parlcd_write_data(parlcd_mem_base, 0x11);

    parlcd_write_cmd(parlcd_mem_base, 0xC5);
    parlcd_write_data(parlcd_mem_base, 0x08);

    parlcd_write_cmd(parlcd_mem_base, 0xC8);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x32);
    parlcd_write_data(parlcd_mem_base, 0x36);
    parlcd_write_data(parlcd_mem_base, 0x45);
    parlcd_write_data(parlcd_mem_base, 0x06);
    parlcd_write_data(parlcd_mem_base, 0x16);
    parlcd_write_data(parlcd_mem_base, 0x37);
    parlcd_write_data(parlcd_mem_base, 0x75);
    parlcd_write_data(parlcd_mem_base, 0x77);
    parlcd_write_data(parlcd_mem_base, 0x54);
    parlcd_write_data(parlcd_mem_base, 0x0C);
    parlcd_write_data(parlcd_mem_base, 0x00);

    parlcd_write_cmd(parlcd_mem_base, 0x36);
    parlcd_write_data(parlcd_mem_base, 0x0a);

    parlcd_write_cmd(parlcd_mem_base, 0x3A);
    parlcd_write_data(parlcd_mem_base, 0x55);

    parlcd_write_cmd(parlcd_mem_base, 0x2A);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x01);
    parlcd_write_data(parlcd_mem_base, 0x3F);

    parlcd_write_cmd(parlcd_mem_base, 0x2B);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x01);
    parlcd_write_data(parlcd_mem_base, 0xDF);

    parlcd_delay(120);
    parlcd_write_cmd(parlcd_mem_base, 0x29);

    parlcd_delay(25);

#else
    // HX8357-C display initialisation

    parlcd_write_cmd(parlcd_mem_base, 0xB9); // Enable extension command
    parlcd_write_data(parlcd_mem_base, 0xFF);
    parlcd_write_data(parlcd_mem_base, 0x83);
    parlcd_write_data(parlcd_mem_base, 0x57);
    parlcd_delay(50);

    parlcd_write_cmd(parlcd_mem_base, 0xB6); //Set VCOM voltage
    //parlcd_write_data(parlcd_mem_base, 0x2C);    //0x52 for HSD 3.0"
    parlcd_write_data(parlcd_mem_base, 0x52);    //0x52 for HSD 3.0"

    parlcd_write_cmd(parlcd_mem_base, 0x11); // Sleep off
    parlcd_delay(200);

    parlcd_write_cmd(parlcd_mem_base, 0x35); // Tearing effect on
    parlcd_write_data(parlcd_mem_base, 0x00);    // Added parameter

    parlcd_write_cmd(parlcd_mem_base, 0x3A); // Interface pixel format
    parlcd_write_data(parlcd_mem_base, 0x55);    // 16 bits per pixel

    //parlcd_write_cmd(parlcd_mem_base, 0xCC); // Set panel characteristic
    //parlcd_write_data(parlcd_mem_base, 0x09);    // S960>S1, G1>G480, R-G-B, normally black

    //parlcd_write_cmd(parlcd_mem_base, 0xB3); // RGB interface
    //parlcd_write_data(parlcd_mem_base, 0x43);
    //parlcd_write_data(parlcd_mem_base, 0x00);
    //parlcd_write_data(parlcd_mem_base, 0x06);
    //parlcd_write_data(parlcd_mem_base, 0x06);

    parlcd_write_cmd(parlcd_mem_base, 0xB1); // Power control
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x15);
    parlcd_write_data(parlcd_mem_base, 0x0D);
    parlcd_write_data(parlcd_mem_base, 0x0D);
    parlcd_write_data(parlcd_mem_base, 0x83);
    parlcd_write_data(parlcd_mem_base, 0x48);


    parlcd_write_cmd(parlcd_mem_base, 0xC0); // Does this do anything?
    parlcd_write_data(parlcd_mem_base, 0x24);
    parlcd_write_data(parlcd_mem_base, 0x24);
    parlcd_write_data(parlcd_mem_base, 0x01);
    parlcd_write_data(parlcd_mem_base, 0x3C);
    parlcd_write_data(parlcd_mem_base, 0xC8);
    parlcd_write_data(parlcd_mem_base, 0x08);

    parlcd_write_cmd(parlcd_mem_base, 0xB4); // Display cycle
    parlcd_write_data(parlcd_mem_base, 0x02);
    parlcd_write_data(parlcd_mem_base, 0x40);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x2A);
    parlcd_write_data(parlcd_mem_base, 0x2A);
    parlcd_write_data(parlcd_mem_base, 0x0D);
    parlcd_write_data(parlcd_mem_base, 0x4F);

    parlcd_write_cmd(parlcd_mem_base, 0xE0); // Gamma curve
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x15);
    parlcd_write_data(parlcd_mem_base, 0x1D);
    parlcd_write_data(parlcd_mem_base, 0x2A);
    parlcd_write_data(parlcd_mem_base, 0x31);
    parlcd_write_data(parlcd_mem_base, 0x42);
    parlcd_write_data(parlcd_mem_base, 0x4C);
    parlcd_write_data(parlcd_mem_base, 0x53);
    parlcd_write_data(parlcd_mem_base, 0x45);
    parlcd_write_data(parlcd_mem_base, 0x40);
    parlcd_write_data(parlcd_mem_base, 0x3B);
    parlcd_write_data(parlcd_mem_base, 0x32);
    parlcd_write_data(parlcd_mem_base, 0x2E);
    parlcd_write_data(parlcd_mem_base, 0x28);

    parlcd_write_data(parlcd_mem_base, 0x24);
    parlcd_write_data(parlcd_mem_base, 0x03);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x15);
    parlcd_write_data(parlcd_mem_base, 0x1D);
    parlcd_write_data(parlcd_mem_base, 0x2A);
    parlcd_write_data(parlcd_mem_base, 0x31);
    parlcd_write_data(parlcd_mem_base, 0x42);
    parlcd_write_data(parlcd_mem_base, 0x4C);
    parlcd_write_data(parlcd_mem_base, 0x53);
    parlcd_write_data(parlcd_mem_base, 0x45);
    parlcd_write_data(parlcd_mem_base, 0x40);
    parlcd_write_data(parlcd_mem_base, 0x3B);
    parlcd_write_data(parlcd_mem_base, 0x32);

    parlcd_write_data(parlcd_mem_base, 0x2E);
    parlcd_write_data(parlcd_mem_base, 0x28);
    parlcd_write_data(parlcd_mem_base, 0x24);
    parlcd_write_data(parlcd_mem_base, 0x03);
    parlcd_write_data(parlcd_mem_base, 0x00);
    parlcd_write_data(parlcd_mem_base, 0x01);

    parlcd_write_cmd(parlcd_mem_base, 0x36); // MADCTL Memory access control
    //parlcd_write_data(parlcd_mem_base, 0x48);
    parlcd_write_data(parlcd_mem_base, 0xE8);
    parlcd_delay(20);

    parlcd_write_cmd(parlcd_mem_base, 0x21); //Display inversion on
    parlcd_delay(20);

    parlcd_write_cmd(parlcd_mem_base, 0x29); // Display on

    parlcd_delay(120);
#endif
}

void writeToLCD(int colours[3], char map[HEIGHT][WIDTH], unsigned char *parlcd_mem_base) {
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < 320; i++) {
        for (int j = 0; j < 480; j++) {
            if ((i % LINEWIDTH == 0) || (j % LINEWIDTH == 0)) {
                parlcd_write_data(parlcd_mem_base, 0xFFFF);
            } else {
                parlcd_write_data(parlcd_mem_base, (uint16_t) player_colours[map[i / LINEWIDTH][j / LINEWIDTH]]);
            }

        }
    }

}
//endregion


//--------NETWORKING---------------------------------------------
//region variables
//are we connected to another player
int netstatus = 0;
/**
 * are we a server?
 */
int server = 0;
/**
 * Used to determine the last state of {@link #server}
 */
int lastServer;
int connectCounter = 0;
int CONNECTDELAY = 2;
//region sender
struct sockaddr_in servSenderSockaddr;
struct sockaddr_in cliSenderSockaddr;
int cliSenderFD;
int servSenderFD;
//endregion
//region receiver
struct sockaddr_in servListenerSockaddr;
struct sockaddr_in cliListenerSockaddr;
int servListenerFD;
int cliListenerFD;
//endregion
int connectedPlayerCount = 0;
//region recieved info
char recieved_gameworld[HEIGHT][WIDTH];
//endregion
//endregion

//region functions
//region senders
/**
* Used to create sender for anybody who wants it: 
 * Used by {@link #createClientSender()}
 * Used by {@link #createServerSender()}
 */
void createSender(int port, struct sockaddr_in *sockaddr, int *fd);

/**
 * Creates sender socket for the client
 */
void createClientSender();

/**
 * Creates broadcast server
 */
void createServerSender();

/**
 * Sends message from the client socket
 */
void clientSend(char *buf);

/**
 * Sends message from the server socket
 */
void serverSend(char *buf);

/**
 * Uses {@link #serverSend} to send stuff from server send socket
 */
void sendMap(char gameworld[HEIGHT][WIDTH]);

/**
 * Broadcasts player table to the network. Used in server mode to notify clients that they are connected.
 */
void serverSendPTable(char *playerIDs);
//endregion

//region receivers

/**
 * Creates Listener socket for the server
 */
void createServerListener();

/**
 * Creates Listener socket for the client
 */
void createClientListener();

/**
 * Used to create listener for anybody who wants it: 
 * Used by {@link #createClientListener()}
 * Used by {@link #createServerListener()}
 */
void createListener(int port, struct sockaddr_in *sockaddr, int *fd);

/*
 * Listen for message
 */
ssize_t listen_message(char *buf, int length, int fd);

/*
 * Client's function for listen thread
 * usage:
 * pthread_t td;
 * pthread_create(&td, NULL, clientListen, NULL);
 */
void* clientListen(void* args);

/*
 * Server's function for listen thread
 * usage:
 * pthread_t td;
 * pthread_create(&td, NULL, serverListen, NULL);
 */
void* serverListen(void* args);

//endregion
//endregion

//---------IMPLEMENTATION OF NETWORKING---------------------------

void createListener(int port, struct sockaddr_in *sockaddr, int *fd) {
    *fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (*fd <= 0) {
        perror("socket() failed.");
        return;
    }

    // set timeout to 2 seconds.
    struct timeval timeV;
    timeV.tv_sec = 0;
    timeV.tv_usec = 200000;

    if (setsockopt(*fd, SOL_SOCKET, SO_RCVTIMEO, &timeV, sizeof(timeV)) == -1) {
        perror("setsockopt failed");
        close(*fd);
        exit(1);
    }

    // bind the port
    memset(&sockaddr, 0, sizeof(sockaddr));

    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons((uint16_t) port);
    sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);

    int status = bind(*fd, (struct sockaddr *) &sockaddr, sizeof(sockaddr));
    if (status == -1) {
        close(*fd);
        perror("bind() failed.");
        exit(1);
    }

//    struct sockaddr_in receiveSockaddr;
//    socklen_t receiveSockaddrLen = sizeof(receiveSockaddr);
//
//    size_t bufSize = 16384;
//    void *buf = malloc(bufSize);
//    ssize_t result = recvfrom(fd, buf, bufSize, 0, (struct sockaddr *)&receiveSockaddr, &receiveSockaddrLen);

}

void createClientListener() {
    createListener(SERVER_PORT, &cliListenerSockaddr, &cliListenerFD);
}

void createServerListener() {
    createListener(CLIENT_PORT, &servListenerSockaddr, &servListenerFD);
}

void createSender(int port, struct sockaddr_in *sockaddr, int *fd) {
    if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    memset(&sockaddr, 0, sizeof(sockaddr));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons((uint16_t) port);
    sockaddr->sin_addr.s_addr = INADDR_BROADCAST;

    int reuse = 1;

    if (setsockopt(*fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                   sizeof(reuse)) == -1) {
        perror("setsockopt (SO_REUSEADDR)");
        exit(1);
    }

    int broadcast = 1;
    if (setsockopt(*fd, SOL_SOCKET, SO_BROADCAST, &broadcast,
                   sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }
}

void createClientSender() {
    createSender(CLIENT_PORT, &cliSenderSockaddr, &cliSenderFD);
}

void sendMap(char gameworld[HEIGHT][WIDTH]) {
	
    char msg[WIDTH * HEIGHT];
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            msg[(j + i*WIDTH)] = (gameworld[i][j]);
        }
    }
    serverSend(msg);
}

void createServerSender() {
    createSender(SERVER_PORT, &servSenderSockaddr, &servSenderFD);
}

void serverSendPTable(char *playerIDs) {
    char buf[128] = "Game: ";
    strcat(buf, playerIDs);
}

void serverSend(char *buf) {
    sendto(servSenderFD, buf, sizeof(buf), 0, (const struct sockaddr *) &servSenderSockaddr,
           sizeof(servSenderSockaddr));
}

void clientSend(char *buf) {
    sendto(cliSenderFD, buf, sizeof(buf), 0, (const struct sockaddr *) &cliSenderSockaddr, sizeof(cliSenderSockaddr));
}

ssize_t listen_message(char *buf, int length, int fd) {
	struct sockaddr_in receiveSockaddr;
	socklen_t receiveSockaddrLen = sizeof(receiveSockaddr);
    ssize_t result = recvfrom(fd, buf, (size_t) length, 0, (struct sockaddr *) &receiveSockaddr, &receiveSockaddrLen);
	return result;
}

void* clientListen(void* args){
	char msg[HEIGHT*WIDTH];
	while(1){
        if (listen_message(msg, HEIGHT * WIDTH, cliListenerFD) > (HEIGHT * WIDTH) - 2) {
			for (int i = 0; i<HEIGHT*WIDTH; i++){
				recieved_gameworld[i/WIDTH][i%WIDTH]=msg[i];
			}
		}
		struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 5 * 1000 * 1000};
		clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
	}
}


// ------------GAME------------------------------------------------
//region variables
//region game values
char gameworld[HEIGHT][WIDTH];
char canvas[HEIGHT][WIDTH];
int gamestatus = 0;
//endregion

//region variables for server simulation
int sim_xspawn[8] = {5, 10, 15, 20, 25, 30, 35, 5};
int sim_yspawn[8] = {5, 10, 15, 20, 25, 5, 10, 25};
int sim_x[8];
int sim_y[8];
char sim_dir[8];
int sim_alive[8];
int sim_count_alive;
//endregion

//region variables for client
char cli_pid = 0;
char cli_dir = NORTH;
//endregion

//region functions
/**
 * sets the game into an initial condition and initializes variables.
 */
void resetGame();

/**
 * Loop with ingame logic.
 */
void gameLoop(int r, int g, int b, int button, uint32_t dir);

/**
 * Loop logic and drawing in menu
 */
void menuLoop(int r, int g, int b, int button, uint32_t dir);

/**
 * This function forwards program to current state
 * Forwards to either 	{@link #gameLoop(int, int, int, int, uint32_t)}
 * 				or		{@link #menuLoop(int, int, int, int, uint32_t)}
 */
void logicLoop(int r, int g, int b, int button, uint32_t dir);

/**
 * Game loop for the server
 * Called by: {@link #gameLoop(int, int, int, int, uint32_t)}
 */
void serverLoop(int r, int g, int b, int button, uint32_t dir);

/**
 * Game loop for the client
 * Called by: {@link #gameLoop(int, int, int, int, uint32_t)}
 */
void clientLoop(int r, int g, int b, int button, uint32_t dir);

/**
 * Gets table of players, used by the server
 * @return
 */
char *getPlayerIDTable();
//endregion
//endregion


//----------IMPLEMENTATION OF GAME---------------------------------

void *serverListen(void *args) {
    char msg[HEIGHT * WIDTH];
    int l;
    while (1) {
        if ((l = (int) listen_message(msg, HEIGHT * WIDTH, servListenerFD)) > 0 && l < 4) {
            sim_dir[msg[0]] = msg[1];
        }
        //struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 5 * 1000 * 1000};
        //clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }
}

void serverLoop(int r, int g, int b, int button, uint32_t dir) {
	//SERVER
        //pass server players actions as a client would.
        sim_dir[0] = (char) dir;
        //simulate the game world
        for (int pid = 0; pid < connectedPlayerCount; ++pid) {
            //update a single player
            if (sim_alive[pid]) {
                //player alive
                switch (sim_dir[pid]) {
                    case NORTH:
                        sim_y[pid]--;
                        break;
                    case EAST:
                        sim_x[pid]++;
                        break;
                    case SOUTH:
                        sim_y[pid]++;
                        break;
                    case WEST:
                        sim_x[pid]--;
                        break;
                }
                if (gameworld[sim_y[pid]][sim_x[pid]] == 0) {
                    //safe ground, paint the tile and do nothing
                    gameworld[sim_y[pid]][sim_x[pid]] = (char) (pid + 2);
                } else {
                    //collided, reduce player count
                    sim_alive[pid] = 0;
                    sim_count_alive -= 1;
                    if (sim_count_alive <= 1) {
                        //victory - only one player left alive
                        //reset game and start over.
                        resetGame();
                        return;
                    }
                }
            } else {
                //dead player
                continue;
            }
        }
        //send map
    sendMap(gameworld);
}

void clientLoop(int r, int g, int b, int button, uint32_t dir) {
	//CLIENT

        //receive world information
        for (int x = 0; x < WIDTH ; ++x) {
            for (int y = 0; y < HEIGHT ; ++y) {
                if(x>0 && x < WIDTH-2 && y>0 && y<HEIGHT-2) {
                    gameworld[y][x] = recieved_gameworld[y][x]; //TODO
                } else {
                    gameworld[y][x] = cli_pid;
                }
            }
        }
        //broadcast my dir
        char msg[2];
        sprintf(msg, "%d%d", cli_pid, cli_dir);
        clientSend(msg);
}

void resetGame() {
    sim_count_alive = connectedPlayerCount;
    for (int pid = 0; pid < connectedPlayerCount; ++pid) {
        sim_x[pid] = sim_xspawn[pid];
        sim_y[pid] = sim_yspawn[pid];
        sim_dir[pid] = NORTH;
        sim_alive[pid] = 1;
    }
    //wipe the board
    for (int x = 1; x < WIDTH - 2; ++x) {
        for (int y = 1; y < HEIGHT - 2; ++y) {
            gameworld[y][x] = 0;
        }
    }
}

void gameLoop(int r, int g, int b, int button, uint32_t dir) {
    if (server == 1) {
        serverLoop(r, g, b, button, dir);
    } else {
        clientLoop(r, g, b, button, dir);
    }

    //both client and server can leave into menu.
    if (button) {
        gamestatus = 0;
    }
}

void menuLoop(int r, int g, int b, int button, uint32_t dir) {
    char col;
    //
    switch (dir) {
        case NORTH:
            //SERVER MODE!
            server = 1;
            if(lastServer==0) {
                createServerSender();
                createServerListener();
            }

            serverSendPTable(getPlayerIDTable());
            //Show graphic for being in server mode
            col = 1;
            //Starting game by button
            if (button) {
                col = 3;
                gamestatus = 1;
		pthread_t td;
		pthread_create(&td, NULL, serverListen, NULL);
            }
            break;
        default:
            server = 0;
            if (lastServer == 1) {
                createClientListener();
                createClientSender();
            }
            col = 0;
            break;
    }
    lastServer = server;

    if (r || g || b);

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            //if (i > 5 && i < 10 && j > 5 && j < 10) {
            if (maskText("255.1234567", 2, 2, j, i)) {
                canvas[i][j] = col;
            } else {
                canvas[i][j] = 0;
            }
        }

    }
}

void logicLoop(int r, int g, int b, int button, uint32_t dir) {
    if (gamestatus == 1) {
        //We are in game
        gameLoop(r, g, b, button, dir);
        writeToLCD(player_colours, gameworld, parlcd_mem_base);
    } else {
        //We are in menu
        menuLoop(r, g, b, button, dir);
        writeToLCD(player_colours, canvas, parlcd_mem_base);
    }
}

char *getPlayerIDTable() {
    //TODO: IMPLEMENT
    return NULL;
}

int main(int argc, char *argv[]) {
    unsigned char *mem_base;

    mem_base = map_phys_address(SPILED_REG_BASE_PHYS, SPILED_REG_SIZE, 0);

    if (mem_base == NULL)
        exit(1);
    uint32_t rgb_knobs_value;
    int last_val;
    int i, j;
    for (i = 0; i < HEIGHT; ++i) {
        for (j = 0; j < WIDTH; ++j) {
            if (i == 0 || j == 0 || i == HEIGHT - 1 || j == WIDTH - 1) {
	    	gameworld[i][j] = 1;
		recieved_gameworld[i][j] = 1;
            } else {
	    	gameworld[i][j] = 0;
		recieved_gameworld[i][j] = 0;
	    }
        }

    }
    unsigned c;

    parlcd_mem_base = map_phys_address(PARLCD_REG_BASE_PHYS, PARLCD_REG_SIZE, 0);

    if (parlcd_mem_base == NULL)
        exit(1);

    parlcd_hx8357_init(parlcd_mem_base);

    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (i = 0; i < 320; i++) {
        for (j = 0; j < 480; j++) {
            c = 0;
            parlcd_write_data(parlcd_mem_base, (uint16_t) c);
        }
    }



    /*parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (i = 0; i < 320 ; i++) {
      for (j = 0; j < 480 ; j++) {
        c = ((i & 0x1f) << 11) | (j & 0x1f);
        if (i < 10)
          c |= 0x3f << 5;
        if (j < 10)
          c |= 0x3f << 5;
        parlcd_write_data(parlcd_mem_base, c);
      }
    }*/
    uint32_t dir = 0;
    last_val = *(volatile uint32_t *) (mem_base + SPILED_REG_KNOBS_8BIT_o);
    while (1) {
        struct timespec loop_delay = {.tv_sec = 0, .tv_nsec = 200 * 1000 * 1000};

        /**(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = 0x0001;
        *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = 0x0002;
        *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = 0x0004;
        *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = 0x0008;
        *(volatile uint32_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = 0x0010;
        *(volatile uint16_t*)(parlcd_mem_base + PARLCD_REG_DATA_o) = 0x0020;*/
        rgb_knobs_value = *(volatile uint32_t *) (mem_base + SPILED_REG_KNOBS_8BIT_o);
        int b = (rgb_knobs_value) & 0xFF;
        int g = (rgb_knobs_value >> 8) & 0xFF;
        int r = (rgb_knobs_value >> 16) & 0xFF;
        int button = ((rgb_knobs_value & 0x1000000) != 0);
        if (b > last_val + 3) {
            dir = ((dir) + 1) % 4;
            last_val = b;
        } else if (b < last_val - 3) {
            dir = ((dir) - 1) % 4;
            last_val = b;
        }
        if (r) { if (g) { if (b); }}

        logicLoop(r, g, b, button, dir);


        clock_nanosleep(CLOCK_MONOTONIC, 0, &loop_delay, NULL);
    }

    return 0;
}
