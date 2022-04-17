//
// Created by t3 on 20/02/2022.
//

// this all uses the modern functionality, maybe add a toggle for the old functionality in future
// Using https://tobiasvl.github.io/blog/write-a-chip-8-emulator/#0nnn-execute-machine-language-routine as a reference for the implementation of instructions
// What lessons have I learnt (this is part of a research project):
// Read documentation properly, this lead to hours and hours of debugging
// Do not try and write portable code, instead write simple code and use portable libraries.
// This means rather than writing code to support multiple graphics libraries, use a multiplatform library like sdl
// This allows the emulator to be more deeply interwoven with the surrounding packaging allowing simpler code with just as much cross platform support (even my calculator supports sdl)
// This means that polling for input can happen on demand rather than as part of a separate loop
// Do not write code when tired, you make more mistakes and this takes up more debugging time
// all sorts of syntax issues are created.
// Think of how long a project will take in the worst case scenario then triple that time, this is how long your project will take
// Don't push yourself too hard, sprints are often less productive then spreading your work out
// Don't be afraid to ask for help from others
// Software development is a marathon not a sprint and you should iteratively test debug and improve
// Start with the minimum viable product and work your way up from there
// know what your actually testing lol, a bunch of the roms that I tried and got confused by were actually for superchip and xo chip extensions rather than standard chip 8 roms

#include "Chip8.h"

const unsigned char font[80] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

Chip8::Chip8(const string &path) : path(path) {
    running = true;
    //set our pixels to black
    memset(pixels, 0, 64*32* sizeof(uint32_t));
    //copy our font to the correct location we don't need the safe variant as source is in our app
    memcpy(memory+80, font, sizeof(font));

    //copy the file being loaded by the user into memory in 512 bit chunks, this is the largest size chunk possible.
    FILE * pFile;
    pFile = fopen(path.c_str(), "r");
    if (pFile != nullptr) {
        //get file size
        long size;
        fseek (pFile , 0 , SEEK_END);
        size = ftell (pFile);
        rewind (pFile);
        if (size < 4096 - 512) {
            //fuck you hexdump (it swaps bytes in a halfword)
            if (fread (memory+512,1,size,pFile) == size){
                //we have done everything we need to so the emu has been successfully initialised
                initialised = true;
            }
        }
        fclose (pFile);
    }
}

void Chip8::ClearScreen() {
    memset(pixels, 0, 64*32* sizeof(uint32_t));
    draw = true;
}

bool Chip8::DrawPixel(int x, int y) {
    if ((y<32)&(x<64)) {
        draw=  true;
        if (pixels[y * 64 + x] == 0) {
            pixels[y * 64 + x] = (4294967295);
            //no collision
            return false;
        } else {
            pixels[y * 64 + x] = 0;
            // flipped pixel, collision
            return true;
        }
    }
    else
    {
        return false;
    }
}

bool Chip8::DrawLine(int x, int y, char data) {
    bool collision = false;
    for (int i = 0; i < 8; i++){
        if((data & 0x80) == 0x80)
        {
            if (DrawPixel(x+i, y))
                collision = true;
        }
        data <<= 1;
    }
    return collision;
}

bool Chip8::DrawSprite(int x, int y, int height)
{
    y %= 32;
    x %= 64;
    bool collision = false;
    for (int i = 0; i < height; i++)
    {
        if (DrawLine(x, y+i, memory[I+i]))
            collision = true;
    }
    return collision;

}

void Chip8::ExecuteCycle() {
    //idk what to call this imo its instruction others say opcode but I say its instruction since it has both op code and operand
    char addrnum;
    short instruction = memory[pc] << 8 | memory[pc+1];
    //print current instruction
    printf("Instruction: %04hX ", instruction);
    printf("v0: %d ", V[0]);
    printf("v1: %d ", V[1]);
    printf("vf: %d\n", V[0xF]);
    switch (instruction & 0xF000) {
        case (0x0000):
            switch(instruction & 0X00FF) {
                //00E0
                //clear the screen
                case (0x00E0):
                    ClearScreen();
                    pc+=2;
                    break;
                //00EE
                //return to mem address from stack, (subroutine) reverses 2NNN jump
                case (0x00EE):
                    //decrement stack pointer
                    sp--;
                    //move program counter to value on stack
                    pc = stack[sp];
                    //move the pc forward by one (2)
                    pc+=2;
                    break;
                default:
                    // in the original machine emulators this would jump the pc to some real address and execute the platform's actual machine code.
                    printf("Unknown op code");
                    pc+=2;
            }
            break;
        //1NNN
        //Jump to NNN
        case (0x1000):
            pc = (instruction & 0x0FFF);
            break;
        //2NNN
        //Jump to NNN but push the current program counter to the stack so that we can return later
        case (0x2000):
            stack[sp] = pc;
            sp++;
            pc = (instruction & 0x0FFF);
            break;
        //3XNN
        //Skip one instruction if VX == NN
        case (0x3000):
            pc += V[(instruction & 0x0F00) >>8] == (instruction & 0x00FF) ? 4:2;
            break;
        //4XNN
        //Skip one instruction if VX != NN
        case (0x4000):
            pc += V[(instruction & 0x0F00) >>8] != (instruction & 0x00FF) ? 4:2;
            break;
        //5XY0
        //Skip one instruction if VX == VY
        case (0x5000):
            pc += V[(instruction & 0x0F00) >>8] == V[(instruction & 0x00F0) >>4] ? 4:2;
            break;
        //6XNN
        //Set VX to NN
        case (0x6000):
            V[(instruction & 0x0F00) >>8] = instruction & 0x00FF;
            pc+=2;
            break;
        //7XNN
        //Add the value NN to VX
        case (0x7000):
            V[(instruction & 0x0F00) >>8] += instruction & 0x00FF;
            pc+=2;
            break;
        //Logic and Arithmetic instructions
        case (0x8000):
            switch(instruction & 0X000F) {
                //8XY0
                //Set VX to VY
                case (0x0000):
                    V[(instruction & 0x0F00) >>8] = V[(instruction & 0x00F0) >>4];
                    break;
                //8XY1
                //Set VX to the binary or of VX and VY
                case (0x0001):
                    V[(instruction & 0x0F00) >>8] |= V[(instruction & 0x00F0) >>4];
                    break;
                //8XY2
                //Set VX to the binary and of VX and VY
                case (0x0002):
                    V[(instruction & 0x0F00) >>8] &= V[(instruction & 0x00F0) >>4];
                    break;
                //8XY3
                //Set VX to the logical xor of VX and VY
                case (0x0003):
                    V[(instruction & 0x0F00) >>8] ^= V[(instruction & 0x00F0) >>4];
                    break;
                //8XY4
                //VX is set to VX + VY
                case (0x0004):
                    V[(instruction & 0x0F00) >>8] += V[(instruction & 0x00F0) >>4];
                    if (V[(instruction & 0x00F0) >>4] > V[(instruction & 0x0F00) >>8])
                        //overflow
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    break;
                //8XY5
                //VX is set to VX - VY
                case (0x0005):
                    V[(instruction & 0x0F00) >>8] -= V[(instruction & 0x00F0) >>4];
                    if (V[(instruction & 0x00F0) >>4] < V[(instruction & 0x0F00) >>8])
                        //overflow
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    break;
                //8XY6
                //VX is shifted 1 bit to the right
                case (0x0006):
                    V[0xF] = V[(instruction & 0x0F00) >> 8] & 0x1;
                    V[(instruction & 0x0F00) >> 8] >>= 1;
                    break;
                //8XY7
                //VX is set to the result of VY - VX
                case (0x0007):
                    V[(instruction & 0x0F00) >>8] = V[(instruction & 0x00F0) >>4] - V[(instruction & 0x0F00) >>8];
                    if (V[(instruction & 0x00F0) >>4] > V[(instruction & 0x0F00) >>8])
                        //overflow
                        V[0xF] = 1;
                    else
                        V[0xF] = 0;
                    break;
                //8XYE
                //VX is shifted 1 bit to the left
                case (0x000E):
                    V[0xF] = V[(instruction  & 0x0F00) >> 8] >> 7;
                    V[(instruction & 0x0F00)>>8] <<= 1;
                    break;
                default:
                    // if you are here something is a bit scuffed
                    printf("Unknown op code");
            }
            // none of these touch the pc and whilst this is non-standard I can't be bothered to go back and add it to all the others
            pc+=2;
            break;
        //9XY0
        //Skips if VX != VY
        case (0x9000):
            pc += V[(instruction & 0x0F00) >>8] != V[(instruction & 0x00F0) >>4] ? 4:2;
            break;
        //ANNN
        //Sets the index register I to the value NNN
        case (0xA000):
            I = instruction & 0x0FFF;
            pc+=2;
            break;
        //BNNN
        //Jump to NN + V0
        case (0xB000):
            pc = (instruction & 0x0FF) + V[(instruction & 0x0F00) >>8];
            //pc = (instruction & 0x0FF) + V[0];
            break;
        //CXNN
        //Generate a random number and binary AND it with NN and store it in VX
        case (0xC000):
            V[(instruction & 0x0F00) >>8] = rand() % 0x100 & (instruction & 0xFF);
            pc+=2;
            break;
        //DXYN
        //Draw a sprite of height N and with 8 at X: VX and Y: VY, load the sprite from I
        case (0xD000):
            V[0xF] = DrawSprite(V[(instruction & 0x0F00) >>8], V[(instruction & 0x00F0) >>4], instruction & 0x000F);
            pc+=2;
            break;
        //Handle input
        case (0xE000):
            switch(instruction & 0x000F)
            {
                //EXA1
                //Skip one instruction if the key in VX is not pressed
                case (0x0001):
                    if (!keys[V[(instruction & 0x0F00) >>8]& 0x0F])
                        pc+=2;
                    break;
                //EX9E
                //Skip one instruction if the key in VX is pressed
                case (0x000E):
                    if (keys[V[(instruction & 0x0F00) >>8]& 0x0F])
                        pc+=2;
                    break;
            }
            pc+=2;
            break;
        case (0xF000):
            switch(instruction & 0X00FF) {
                bool pressed;
                case (0x0007):
                    V[(instruction & 0x0F00) >>8] = delaytimer;
                    break;
                case (0x000A):
                    pressed = false;
                    for (int i = 0; i < 16; i++)
                        if (keys[i])
                            pressed = true;
                    if (!pressed)
                        pc-=2;
                    //maybe wait for new key to be pressed but for now I don't care
                    break;
                case (0x0015):
                    delaytimer = V[(instruction & 0x0F00) >>8];
                    break;
                case (0x0018):
                    soundtimer = V[(instruction & 0x0F00) >>8];
                    break;
                case (0x001E):
                    I += V[(instruction & 0x0F00) >>8];
                    if (I > 0x0FFF)
                        V[0xF]=1;
                    I &= 0x0FFF;
                    break; // might need to truncate to three in future
                case (0x0029):
                    I = (V[(instruction & 0x0F00) >>8] & 0x0F) * 5;
                    break;
                case (0x0033):
                    memory[I] = V[(instruction & 0x0F00) >>8] / 100;
                    memory[I+1] =  V[(instruction & 0x0F00) >>8] % 100 / 10;
                    memory[I+2] = V[(instruction & 0x0F00) >>8] % 10;
                    break;
                case (0x0055):
                    for (int i = 0; i <= ((instruction & 0x0F00) >>8); i++)
                        memory[I+i] = V[i];
                    break;
                case (0x0065):
                    for (int i = 0; i <= ((instruction & 0x0F00) >>8); i++)
                        V[i] = memory[I+i];
                    break;
                default:
                    // if you are here something is a bit scuffed
                    printf("Unknown op code");
            }
            // only some of these touch the pc and whilst this is non-standard I can't be bothered to go back and add it to all the others
            pc+=2;
            break;
        default:
            printf("Unknown op code");
            pc+=2;
            break;
    }
}