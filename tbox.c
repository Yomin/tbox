/*
 * Copyright (c) 2012 Martin Rödel aka Yomin
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy 
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 * copies of the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in 
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN 
 * THE SOFTWARE.
 */

#define ALSA_PCM_NEW_HW_PARAMS_API
#include <alsa/asoundlib.h>
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include "conio.h"

#define size(X) sizeof(X)/sizeof(*X)

struct Button
{
    char c;
    int f1, f2;
};

struct Button* find(char c, struct Button* button, int size)
{
    int x;
    for(x=0; x<size; x++)
        if(c == button[x].c)
            return button+x;
    return 0;
}

void output(snd_pcm_t *handle, snd_pcm_uframes_t frames, unsigned char* buffer)
{
    int ret = snd_pcm_writei(handle, buffer, frames);
    if(ret == -EPIPE) /* underrun */
        snd_pcm_prepare(handle);
    else if(ret < 0)
        printf("failed while writei: %s\n", snd_strerror(ret));
}

int getF()
{
    int x;
    char c[2];
    for(x=0; x<2; x++)
    {
        if(!kbhit())
            return 0;
        c[x] = getch();
    }
    if(c[0] != 79)
        return 0;
    switch(c[1])
    {
        case 81: return 2;
        case 82: return 3;
        default: return 0;
    }
}

int main(int argc, char* argv[])
{
    snd_pcm_t *handle;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames = 32;
    snd_pcm_format_t format = SND_PCM_FORMAT_U8;
    unsigned int channels = 1;
    unsigned int samplerate = 11025;
    double duration = 0.300;
    double ramp = 0.002;
    
    struct Button dtmf[] = {
        {'1', 697, 1209},
        {'2', 697, 1336},
        {'3', 697, 1477},
        {'4', 770, 1209},
        {'5', 770, 1336},
        {'6', 770, 1477},
        {'7', 852, 1209},
        {'8', 852, 1336},
        {'9', 852, 1477},
        {'0', 941, 1336},
        
        {'A', 697, 1633},
        {'B', 770, 1633},
        {'C', 852, 1633},
        {'D', 941, 1633},
        
        {'*', 941, 1209},
        {'E', 941, 1209},
        {'#', 941, 1477},
        {'F', 941, 1477},
        
        {'G', 350, 440},    /* us dial */
        {'H', 440, 480},    /* us ringback */
        {'I', 480, 620},    /* us busy */
        {'J', 350, 450},    /* uk dial */
        {'K', 400, 450},    /* uk ringback */
        {'L', 425, 0},      /* eu dial/ringback/busy */
        
        {'X',  20,   20},
        {'Y', 100,  100},
        {'Z', 500,  500}
    };
    
    struct Button blue[] = {
        {'1', 700, 900},
        {'2', 700, 1100},
        {'3', 900, 1100},
        {'4', 700, 1300},
        {'5', 900, 1300},
        {'6', 1100, 1300},
        {'7', 700, 1500},
        {'8', 900, 1500},
        {'9', 1100, 1500},
        {'0', 1300, 1500},
        {'A', 700, 1700},   /* ST3 */
        {'B', 900, 1700},   /* ST2 */
        {'K', 1100, 1700},  /* KP */
        {'C', 1300, 1700},  /* KP/ST2 */
        {'S', 1500, 1700}   /* ST */
    };
    
    int ret, size, dir = 0, ivalue, bufcount, buttonsize = size(dtmf);
    double sample, value;
    unsigned char *buffer;
    struct Button *button, *buttons = dtmf;
    char c;
    
    ret = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
    if(ret < 0)
    {
        printf("failed to open pcm device: %s\n", snd_strerror(ret));
        return 1;
    }
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(handle, params);
    snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(handle, params, format);
    snd_pcm_hw_params_set_channels(handle, params, channels);
    snd_pcm_hw_params_set_rate_near(handle, params, &samplerate, &dir);
    snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
    
    ret = snd_pcm_hw_params(handle, params);
    if(ret < 0)
    {
        printf("failed to set hw parameters: %s\n", snd_strerror(ret));
        return 2;
    }
    
    snd_pcm_hw_params_get_period_size(params, &frames, &dir);
    
    size = frames * channels;
    buffer = (unsigned char*) malloc(size);
    
    while(1)
    {
        switch((c = toupper(getch())))
        {
            case 27:
                switch(getF())
                {
                    case 2: /* F2 */
                        buttons = dtmf;
                        buttonsize = size(dtmf);
                        continue;
                    case 3: /* F3 */
                        buttons = blue;
                        buttonsize = size(blue);
                        continue;
                    default:
                        continue;
                }
            default:
                button = find(c, buttons, buttonsize);
        }
        if(c == 'Q')
            break;
        if(!button)
            continue;
        
        bufcount = 0;
        value = 0;
        
        snd_pcm_prepare(handle);
        
        for(sample = 0.0; sample<duration; sample += 1/(samplerate*1.0))
        {
            value = sin(2*M_PI*button->f1*sample) + sin(2*M_PI*button->f2*sample);
            if(sample<ramp)
                value *= sample/ramp;
            if(sample>duration-ramp)
                value *= (duration-sample)/ramp;
            ivalue = floor(value*(0xFF00/4)+0xFF00/2) + (ivalue&~0xFF00);
            buffer[bufcount++] = (ivalue&0xFF00)>>8;
            if(bufcount == size)
            {
                output(handle, frames, buffer);
                bufcount = 0;
            }
        }
    }
    
    snd_pcm_drain(handle);
    snd_pcm_close(handle);
    snd_pcm_hw_free(handle);
    snd_config_update_free_global();
    free(buffer);
    
    return 0;
}

