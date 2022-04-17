//
// Created by t3 on 20/02/2022.
//
#include <cinttypes>
#include <cstdlib>
#include <string>
#include <cstring>
#include <stdio.h>
using namespace std;

#ifndef CH8EMU_CHIP8_H
#define CH8EMU_CHIP8_H

class Chip8 {
private:
    //our memory
    unsigned char memory[4096];
    unsigned short opcode;
    unsigned char V[16];
    unsigned short I;
    //program counter points to current address being executed
    unsigned short pc = 0x200;
    //the length of this doesn't really matter as it varied from system to system but a minimum is 12
    unsigned short stack[16];
    //set to first free spot (0)
    unsigned short sp = 1;
public:
    bool initialised = false;
    bool draw = false;
    //path to rom, not sure we really need this?
    string path;
    uint32_t pixels[64*32* sizeof(uint32_t)];
    Chip8(const string &path);
    void ClearScreen();
    bool DrawPixel(int x, int y);
    bool DrawLine(int x, int y, char data);
    void ExecuteCycle();
    bool running = false;
    unsigned char delaytimer = 0;
    unsigned char soundtimer = 0;
    unsigned char instructioncounter = 0;
    bool keys[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    bool DrawSprite(int x, int y, int height);
};


#endif //CH8EMU_CHIP8_H
