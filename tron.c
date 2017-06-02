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
#define UNDEF 42
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

/* some RGB color definitions                                                 */
#define Black           0x0000      /*   0,   0,   0 */
#define Navy            0x000F      /*   0,   0, 128 */
#define DarkGreen       0x03E0      /*   0, 128,   0 */
#define DarkCyan        0x03EF      /*   0, 128, 128 */
#define Maroon          0x7800      /* 128,   0,   0 */
#define Purple          0x780F      /* 128,   0, 128 */
#define Olive           0x7BE0      /* 128, 128,   0 */
#define LightGrey       0xC618      /* 192, 192, 192 */
#define DarkGrey        0x7BEF      /* 128, 128, 128 */
#define Blue            0x001F      /*   0,   0, 255 */
#define Green           0x07E0      /*   0, 255,   0 */
#define Cyan            0x07FF      /*   0, 255, 255 */
#define Red             0xF800      /* 255,   0,   0 */
#define Magenta         0xF81F      /* 255,   0, 255 */
#define Yellow          0xFFE0      /* 255, 255,   0 */
#define White           0xFFFF      /* 255, 255, 255 */
#define Orange          0xFD20      /* 255, 165,   0 */
#define GreenYellow     0xAFE5      /* 173, 255,  47 */
#define Pink                        0xF81F

//--------DISPLAY-----------------------------------
//region variables
unsigned char *parlcd_mem_base;
int player_colours[10] = {Black, White, Pink, Green, Cyan, Yellow, Magenta, Red, White, Orange};

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

    pagesize = (unsigned long) sysconf(_SC_PAGESIZE);

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
                parlcd_write_data(parlcd_mem_base,
                                  (uint16_t) player_colours[(unsigned char) map[i / LINEWIDTH][j / LINEWIDTH]]);
            }

        }
    }

}
//endregion


//--------NETWORKING---------------------------------------------
//region variables
//are we connected to another player
/**
 * are we a server?
 */
int server = 0;
/**
 * Used to determine the last state of {@link #server}
 */
int lastServer;
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
void clientSend(char *buf, int size);

/**
 * Sends message from the server socket
 */
void serverSend(char *buf, int size);

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
/*
 * Client's function for listen thread
 * usage:
 * pthread_t td;
 * pthread_create(&td, NULL, clientListen, NULL);
 */
void *clientListen(void *args);

/*
 * Server's function for listen thread
 * usage:
 * pthread_t td;
 * pthread_create(&td, NULL, serverListen, NULL);
 */
void *serverListen(void *args);

ssize_t listen_message(char *buf, int length, char *ipbuff, int fd, int flags);
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
    memset(sockaddr, 0, sizeof(*sockaddr));

    sockaddr->sin_family = AF_INET;
    sockaddr->sin_port = htons((uint16_t) port);
    sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);

    int status = bind(*fd, (struct sockaddr *) sockaddr, sizeof(*sockaddr));
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

int hasServerL = 0, hasClientL = 0;

void createClientListener() {
    if (hasClientL == 0) {
        createListener(SERVER_PORT, &cliListenerSockaddr, &cliListenerFD);
        hasClientL = 1;
    }
}

void createServerListener() {
    if (hasServerL == 0) {
        createListener(CLIENT_PORT, &servListenerSockaddr, &servListenerFD);
        hasServerL = 1;
    }
}

void createSender(int port, struct sockaddr_in *sockaddr, int *fd) {
    if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    memset(sockaddr, 0, sizeof(*sockaddr));
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
            msg[(j + i * WIDTH)] = (gameworld[i][j]);
        }
    }
    printf("sendmap\n");
    serverSend(msg, sizeof(char)*WIDTH*HEIGHT);
}

void createServerSender() {
    createSender(SERVER_PORT, &servSenderSockaddr, &servSenderFD);
}

void serverSendPTable(char *playerIDs) {
    char buf[128] = "Game: ";
    strcat(buf, playerIDs);
}

void serverSend(char *buf, int size) {
    sendto(servSenderFD, buf, size, 0, (const struct sockaddr *) &servSenderSockaddr,
           sizeof(servSenderSockaddr));
}

void clientSend(char *buf, int size) {
    sendto(cliSenderFD, buf, size, 0, (const struct sockaddr *) &cliSenderSockaddr, sizeof(cliSenderSockaddr));
}

ssize_t listen_message(char *buf, int length, char *ipbuff, int fd, int flags) {
    struct sockaddr_in receiveSockaddr;
    socklen_t receiveSockaddrLen = sizeof(receiveSockaddr);
    fd_set watch_list;
    struct timeval tv;
    int a = 0;

    FD_ZERO(&watch_list);
    FD_SET(fd, &watch_list);
    tv.tv_sec = 0;
    tv.tv_usec = 1;
    a = select(fd + 1, &watch_list, NULL, NULL, &tv);
    if (a < 1) return 0;
    ssize_t result = recvfrom(fd, buf, (size_t) length, 0, (struct sockaddr *) &receiveSockaddr, &receiveSockaddrLen);
    char *iptmp = inet_ntoa(receiveSockaddr.sin_addr);
    strcpy(ipbuff, iptmp);
    return result;
}

void *clientListen(void *args) {
    char msg[HEIGHT * WIDTH];
    char ipmsg[100];
    while (1) {
        if (listen_message(msg, HEIGHT * WIDTH, ipmsg, cliListenerFD, MSG_WAITALL) > (HEIGHT * WIDTH) - 2) {
            for (int i = 0; i < HEIGHT * WIDTH; i++) {
                recieved_gameworld[i / WIDTH][i % WIDTH] = msg[i];
            }
        }
    }
}


// ------------GAME------------------------------------------------
//region variables
//region game values
char gameworld[HEIGHT][WIDTH];
char canvas[HEIGHT][WIDTH];
int gamestatus = 0;
pthread_t td;
int std = 0;
//endregion

//region variables for server simulation
int sim_xspawn[8] = {5, 10, 15, 20, 25, 30, 35, 5};
int sim_yspawn[8] = {5, 10, 15, 20, 25, 5, 10, 25};
int sim_x[8];
int sim_y[8];
char sim_dir[8] = {UNDEF,UNDEF,UNDEF,UNDEF,UNDEF,UNDEF,UNDEF,UNDEF};
int sim_alive[8];
int sim_count_alive;
//endregion

//region variables for client
char cli_pid;
char cli_dir = NORTH;
//endregion

void addPlayer(int id) {
    if(sim_dir[id]==UNDEF) {
        if (connectedPlayerCount < 7) {
            connectedPlayerCount += 1;
            sim_dir[id]=NORTH;
        }
    }
}

void clearPlayers() {
    connectedPlayerCount = 0;
    for(int i =0; i<8; ++i) {
        sim_dir[i] = UNDEF;
    }
}

char *getPlayers() {
    return "test";
}

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
    char ipmsg[100];
    int l;
    while (server) {
        if ((l = (int) listen_message(msg, HEIGHT * WIDTH, ipmsg, servListenerFD, 0)) > 0 && l < 4) {
            if (!gamestatus) addPlayer(msg[0]);
            sim_dir[(unsigned char) msg[0]] = msg[1];
//            printf(ipmsg);
        }
    }
    return NULL;
}

void serverLoop(int r, int g, int b, int button, uint32_t dir) {
    //SERVER
    //pass server players actions as a client would.
    sim_dir[(int) cli_pid] = (char) dir;
    //simulate the game world
    for (int pid = 0; pid < 8; ++pid) {
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
                default:
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
    cli_dir = dir;
    for (int x = 0; x < WIDTH; ++x) {
        for (int y = 0; y < HEIGHT; ++y) {
            if (x > 0 && x < WIDTH - 1 && y > 0 && y < HEIGHT - 1) {
                gameworld[y][x] = recieved_gameworld[y][x]; //TODO
            } else {
                gameworld[y][x] = cli_pid+2;
            }
        }
    }
    //broadcast my dir
    char msg[2];
    printf( "%d%d", cli_pid, cli_dir);
    msg[0]=cli_pid;
    msg[1]=cli_dir;
    clientSend(msg, 2*sizeof(char));
}

void resetGame() {
    printf("RESET GAME\n");
    for (int pid = 0; pid < 8; ++pid) {
        if (sim_dir[pid] !=UNDEF) {
            sim_x[pid] = sim_xspawn[pid];
            sim_y[pid] = sim_yspawn[pid];
            sim_dir[pid] = NORTH;
            sim_alive[pid] = 1;
        }
    }
    sim_count_alive = connectedPlayerCount + 1;
    //wipe the board
    for (int x = 1; x < WIDTH - 1; ++x) {
        for (int y = 1; y < HEIGHT - 1; ++y) {
            gameworld[y][x] = 0;
        }
    }
}

int laststatus = 0;

void gameLoop(int r, int g, int b, int button, uint32_t dir) {
    printf("GAME %d\n", sim_count_alive);
    if (laststatus == 0) {
        laststatus = 1;
        resetGame();
    }

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
    char col = 5;
    //
    switch (dir) {
        case NORTH:
            printf("SERVER\n");
            //SERVER MODE!
            server = 1;
            if (lastServer == 0) {
                printf("C > S\n");
                createServerSender();
                createServerListener();
            }
            if (std == 0) {
                if (pthread_create(&td, NULL, serverListen, NULL) == 0) {
                    printf("SERVER THREAD\n");
                    std = 1;
                }
            }
            sim_count_alive = 0;
            //Show graphic for being in server mode
            col = 3;
            //Starting game by button
            if (button) {
                col = 1;
                gamestatus = 1;
                sim_dir[(int) cli_pid]=NORTH;
                resetGame();
                /*for (int i = 0; i < 8; i++) {
                    if (sim_dir[i] == UNDEF) {
                        sim_alive[i] = 0;
                    } else {
                        sim_alive[i] = 1;
                        sim_count_alive++;
                    }
                }*/
            }
            break;
        default:
            printf("CLIENT\n");
            server = 0;
            if (std == 1) {
                pthread_join(td, NULL);
                std = 0;
            }
            if (lastServer == 1) {
                printf("S > C\n");
                clearPlayers();
                createClientListener();
                createClientSender();
            }
            col = 2;
            if (button) {
                //col = 3;
                gamestatus = 1;
                pthread_create(&td, NULL, clientListen, NULL);
                printf("CLIENT THREAD\n");
            }
            break;
    }
    lastServer = server;

    if (r || g || b);

    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            //if (i > 5 && i < 10 && j > 5 && j < 10) {
            char txt[50];
            sprintf(txt, "...%d...", connectedPlayerCount + 1);
            if (maskText(txt, 2, 2, j, i)) {
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
        laststatus = 0;
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

    if (argc != 2) {
        perror("MOAR ARGS");
        exit(1);
    } else {
        cli_pid = (char) atoi(argv[1]);
        if (cli_pid > 7 || cli_pid <0){
            perror("BAD ARGS");
            exit(1);
        }
    }
    if (mem_base == NULL)
        exit(1);
    uint32_t rgb_knobs_value;
    int last_val;
    int i, j;
    for (i = 0; i < HEIGHT; ++i) {
        for (j = 0; j < WIDTH; ++j) {
            if (i == 0 || j == 0 || i == HEIGHT - 1 || j == WIDTH - 1) {
                gameworld[i][j] = cli_pid+2;
                recieved_gameworld[i][j] = cli_pid+2;
            } else {
                gameworld[i][j] = 0;
                recieved_gameworld[i][j] = 0;
            }
        }

    }
    for (i = 0; i < 8; i++) {
        if(i!=(int)cli_pid) {
            sim_dir[i] = UNDEF;
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
