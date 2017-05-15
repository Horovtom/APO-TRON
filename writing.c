#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int char_zero[15] ={1,1,1,
                    1,0,1,
                    1,0,1,
                    1,0,1,
                    1,1,1,};
int char_one[15] = {0,0,1,
                    0,0,1,
                    0,0,1,
                    0,0,1,
                    0,0,1,};

int char_two[15] = {1,1,1,
                    0,0,1,
                    1,1,1,
                    1,0,0,
                    1,1,1,};

int char_three[15] ={1,1,1,
                     0,0,1,
                     1,1,1,
                     0,0,1,
                     1,1,1,};

int char_four[15] ={1,0,1,
                    1,0,1,
                    1,1,1,
                    0,0,1,
                    0,0,1,};

int char_five[15] ={1,1,1,
                    1,0,0,
                    1,1,1,
                    0,0,1,
                    1,1,1,};

int char_six[15] = {1,1,1,
                    1,0,0,
                    1,1,1,
                    1,0,1,
                    1,1,1,};

int char_seven[15] ={1,1,1,
                     0,0,1,
                     0,0,1,
                     0,0,1,
                     0,0,1,};

int char_eight[15] ={1,1,1,
                     1,0,1,
                     1,1,1,
                     1,0,1,
                     1,1,1,};

int char_nine[15] ={1,1,1,
                    1,0,1,
                    1,1,1,
                    0,0,1,
                    0,0,1,};

int char_dot[15] ={ 0,0,0,
                    0,0,0,
                    0,0,0,
                    0,0,0,
                    0,0,1,};

int char_space[15] ={ 0,0,0,
                      0,0,0,
                      0,0,0,
                      0,0,0,
                      0,0,0,};

void printnum(int * num) {
    int i;
    for(i=0;i<5;++i){
        printf("%d%d%d\n",num[i*3],num[i*3+1],num[i*3+2]);
    }
}

int* charToMask(char c) {
    switch(c) {
        case '0': return char_zero;
        case '1': return char_one;
        case '2': return char_two;
        case '3': return char_three;
        case '4': return char_four;
        case '5': return char_five;
        case '6': return char_six;
        case '7': return char_seven;
        case '8': return char_eight;
        case '9': return char_nine;
        case '.': return char_dot;
        default: return char_space;
    }
}

/*1 if mask, 0 if not*/
int maskText(char * text, int textx, int texty, int cursorx, int cursory) {
    int len = strlen(text);
    if(cursorx >= textx && cursorx < textx+len*4 && cursory >= texty && cursory < texty+5) {
        int inletterx = (cursorx - textx)%4;
        if(inletterx==3) {return 0;}
        int letter = (cursorx - textx)/4;
        int inlettery = (cursory - texty)%5;
        return charToMask(text[letter])[inlettery*3+inletterx];
    }
    return 0;
}