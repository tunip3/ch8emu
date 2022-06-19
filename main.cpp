#include <iostream>
#include <SDL.h>
#include <SDL_audio.h>
#include "Chip8.h"
#include <queue>
#include <cmath>


const int AMPLITUDE = 28000;
const int FREQUENCY = 44100;

//this beeper code was stolen from stack overflow, because I didn't want to write a whole audio system just for a beep
struct BeepObject
{
    double freq;
    int samplesLeft;
};

class Beeper
{
private:
    double v;
    std::queue<BeepObject> beeps;
public:
    Beeper();
    ~Beeper();
    void beep(double freq, int duration);
    void generateSamples(Sint16 *stream, int length);
    void wait();
};

void audio_callback(void*, Uint8*, int);

Beeper::Beeper()
{
    SDL_AudioSpec desiredSpec;

    desiredSpec.freq = FREQUENCY;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = 2048;
    desiredSpec.callback = audio_callback;
    desiredSpec.userdata = this;

    SDL_AudioSpec obtainedSpec;

    // you might want to look for errors here
    SDL_OpenAudio(&desiredSpec, &obtainedSpec);

    // start play audio
    SDL_PauseAudio(0);
}

Beeper::~Beeper()
{
    SDL_CloseAudio();
}

void Beeper::generateSamples(Sint16 *stream, int length)
{
    int i = 0;
    while (i < length) {

        if (beeps.empty()) {
            while (i < length) {
                stream[i] = 0;
                i++;
            }
            return;
        }
        BeepObject& bo = beeps.front();

        int samplesToDo = std::min(i + bo.samplesLeft, length);
        bo.samplesLeft -= samplesToDo - i;

        while (i < samplesToDo) {
            stream[i] = AMPLITUDE * std::sin(v * 2 * M_PI / FREQUENCY);
            i++;
            v += bo.freq;
        }

        if (bo.samplesLeft == 0) {
            beeps.pop();
        }
    }
}

void Beeper::beep(double freq, int duration)
{
    BeepObject bo;
    bo.freq = freq;
    bo.samplesLeft = duration * FREQUENCY / 1000;

    SDL_LockAudio();
    beeps.push(bo);
    SDL_UnlockAudio();
}

void Beeper::wait()
{
    int size;
    do {
        SDL_Delay(20);
        SDL_LockAudio();
        size = beeps.size();
        SDL_UnlockAudio();
    } while (size > 0);

}

void audio_callback(void *_beeper, Uint8 *_stream, int _length)
{
    Sint16 *stream = (Sint16*) _stream;
    int length = _length / 2;
    Beeper* beeper = (Beeper*) _beeper;

    beeper->generateSamples(stream, length);
}

Beeper beeper;

Uint32 reduce_timer(Uint32 interval, void *param)
{

    Chip8 *chip8 = (Chip8 *) param;
    SDL_Event event;
    SDL_UserEvent userevent;

    /* In this example, our callback pushes an SDL_USEREVENT event
    into the queue, and causes our callback to be called again at the
    same interval: */

    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = userevent;

    SDL_PushEvent(&event);

    if (chip8->delaytimer != 0)
        chip8->delaytimer--;
    if (chip8->soundtimer != 0)
    {
        beeper.beep(1000, 1020/60);
        chip8->soundtimer--;
    }
    chip8->instructioncounter=0;
    return(interval);
}

int main(int argc, char *argv[]) {
    //make sure a path to the rom is provided
    if (argc < 2)
    {
        printf("You must provide a rom file to load");
        return 0;
    }

    //create chip8
    SDL_Scancode scanCodes[16] = {SDL_SCANCODE_X,
                              SDL_SCANCODE_1,
                              SDL_SCANCODE_2,
                              SDL_SCANCODE_3,
                              SDL_SCANCODE_Q,
                              SDL_SCANCODE_W,
                              SDL_SCANCODE_E,
                              SDL_SCANCODE_A,
                              SDL_SCANCODE_S,
                              SDL_SCANCODE_D,
                              SDL_SCANCODE_Z,
                              SDL_SCANCODE_C,
                              SDL_SCANCODE_4,
                              SDL_SCANCODE_R,
                              SDL_SCANCODE_F,
                              SDL_SCANCODE_V};
    Chip8 chip8 = Chip8(argv[1]);
    if (chip8.initialised) {
        SDL_Event event;
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);

        SDL_Window *window = SDL_CreateWindow(
                "CH8EMU",
                SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED,
                960,
                480,
                0
        );

        SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
        SDL_RenderSetLogicalSize(renderer, 64, 32);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        //create texture
        SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, 64, 32);
        int i = 0;

        SDL_TimerID timerId = SDL_AddTimer(1000/60, reduce_timer, &chip8);

        while (chip8.running) {
            while (SDL_PollEvent(&event)) {
                char keyDown;
                switch (event.type) {
                    case (SDL_QUIT):
                        chip8.running = false;
                        SDL_RemoveTimer(timerId);
                        break;
                    case (SDL_KEYDOWN):
                        keyDown = event.key.keysym.scancode;
                        for (int i = 0; i < 16; i++)
                            if (keyDown == scanCodes[i])
                                chip8.keys[i]=true;
                        break;
                    case (SDL_KEYUP):
                        keyDown = event.key.keysym.scancode;
                        for (int i =0; i < 16; i++)
                            if (keyDown == scanCodes[i])
                                chip8.keys[i]=false;
                        break;
                }
            }
            if (chip8.draw) {
                SDL_UpdateTexture(texture, nullptr, chip8.pixels, 64 * sizeof(uint32_t));
                SDL_RenderClear(renderer);
                SDL_RenderCopy(renderer, texture, nullptr, nullptr);
                SDL_RenderPresent(renderer);
                chip8.draw = false;
            }
            if (chip8.instructioncounter < 12) {
                chip8.ExecuteCycle();
                chip8.instructioncounter +=1;
            }
        }

        //clean up
        SDL_RemoveTimer(timerId);
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    }
    std::cout << "Exited!" << std::endl;
    return 0;
}
