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



//http://www.binarytides.com/programming-udp-sockets-c-linux/
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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include <unistd.h>
#include <strings.h>
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

void writeToLCD(int colours[3], int map[HEIGHT][WIDTH], unsigned char *parlcd_mem_base) {
    parlcd_write_cmd(parlcd_mem_base, 0x2c);
    for (int i = 0; i < 320; i++) {
        for (int j = 0; j < 480; j++) {
            if ((i % LINEWIDTH == 0) || (j % LINEWIDTH == 0)) {
                parlcd_write_data(parlcd_mem_base, 0xFFFF);
            } else {
                parlcd_write_data(parlcd_mem_base, (uint16_t) map[i / LINEWIDTH][j / LINEWIDTH]);
            }

        }
    }

}

//NETWORKING
//are we connected to another player
int netstatus = 0;
//are we a server?
int server = 0;
int connectCounter = 0;
int CONNECTDELAY = 2;

void broadcast() {
    if (connectCounter > CONNECTDELAY) {
        printf("Connecting\n");
        connectCounter = 0;
    } else {
        connectCounter++;
    }
}

// ------------
//game values
int x = 20;
int y = 20;
int player_colours[7] = {0x0000, 0xFFFF, 0xF800, 0x07E0, 0x7800, 0x07FF, 0xFFE0};
int gameworld[HEIGHT][WIDTH];
int canvas[HEIGHT][WIDTH];
int gamestatus = 0;
// ------------
int sim_x[8];
int sim_y[8];
int sim_dir[8];

void gameLoop(int r, int g, int b, int button, uint32_t dir) {
    if(server==1) {
        //SERVER
        
    } else {
        //CLIENT
        
    }
    switch (dir) {
        case NORTH:
            y--;
            break;
        case EAST:
            x++;
            break;
        case SOUTH:
            y++;
            break;
        case WEST:
            x--;
            break;
    }
    if (button) {
        gamestatus = 0;
    }
    if (gameworld[y][x] != player_colours[0]) {
        for (int i = 0; i < HEIGHT; ++i) {
            for (int j = 0; j < WIDTH; ++j) {
                if (gameworld[i][j] == player_colours[2]) gameworld[i][j] = player_colours[0];
            }

        }
        x = 20;
        y = 20;
    }
    gameworld[y][x] = player_colours[2];
}

int servSockFD;
int listenSocketFD;
pthread_t workerThread;
int connectedPlayerCount = 0;


void *getPlayers(void *arg) {
    connectedPlayerCount = 0;
    
//TODO:COMPLETE
    return NULL;
}

/**
 * Listener for broadcast
 */
void createListener() {
    if ((listenSocketFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in bindaddr;

    memset(&bindaddr, 0, sizeof(bindaddr));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(CLIENT_PORT);
    bindaddr.sin_addr.s_addr = INADDR_ANY;

    int yes = 1;

    if (setsockopt(listenSocketFD, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(yes)) == -1) {
        perror("setsockopt (SO_REUSEADDR)");
        exit(1);
    }

    if (bind(listenSocketFD, (struct sockaddr *) &bindaddr, sizeof(bindaddr)) == -1) {
        perror("bind");
        exit(1);
    }


    char buf[256];

    struct sockaddr_in sender;
    socklen_t length = sizeof(sender);
    recvfrom(listenSocketFD, &buf, sizeof(buf), 0, (struct sockaddr *) &sender, &length);
}

void createServer() {
    createListener();
    if ((servSockFD = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in bindaddr;

    memset(&bindaddr, 0, sizeof(bindaddr));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_port = htons(SERVER_PORT);
    bindaddr.sin_addr.s_addr = INADDR_BROADCAST;

    int yes = 1;

    if (setsockopt(servSockFD, SOL_SOCKET, SO_REUSEADDR, &yes,
                   sizeof(yes)) == -1) {
        perror("setsockopt (SO_REUSEADDR)");
        exit(1);
    }

    int broadcast = 1;

    if (setsockopt(servSockFD, SOL_SOCKET, SO_BROADCAST, &broadcast,
                   sizeof broadcast) == -1) {
        perror("setsockopt (SO_BROADCAST)");
        exit(1);
    }

    char buf[] = "Here is the game!";
    sendto(servSockFD, buf, sizeof(buf), 0, (const struct sockaddr *) &bindaddr, sizeof(bindaddr));

    pthread_create(&workerThread, NULL, getPlayers, "getting players... ");

}

void menuLoop(int r, int g, int b, int button, uint32_t dir) {
    broadcast();
    int col = player_colours[0];
    //
    switch (dir) {
        case NORTH:
            server = 1;
            createServer();
            col = player_colours[1];
            if (button) {
                col = player_colours[3];
                gamestatus = 1;
            }
            break;
        default:
            server = 0;
            pthread_join(workerThread, NULL);
            col = player_colours[0];
            break;
    }
    //
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

unsigned char *parlcd_mem_base;

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
            if (i == 0 || j == 0 || i == HEIGHT - 1 || j == WIDTH - 1) gameworld[i][j] = player_colours[1];
            else gameworld[i][j] = player_colours[0];
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
            parlcd_write_data(parlcd_mem_base, c);
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
