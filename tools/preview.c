#include "fly.h"
#include <stdio.h>
#include <stdlib.h>
static void save(const char *name){render();FILE *f=fopen(name,"wb");if(!f)exit(1);fprintf(f,"P6\n240 160\n255\n");for(int i=0;i<W*H;i++){unsigned c=palette[pixels[i]];unsigned char rgb[3]={(c&31)*255/31,((c>>5)&31)*255/31,((c>>10)&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);}
int main(void){sim_reset();fly.auto_mode=0;for(int i=0;i<40;i++)sim_step(KEY_UP);save("build/arena.ppm");for(int i=0;i<20;i++)sim_step(KEY_B|KEY_UP);save("build/flight.ppm");fly.page=1;for(int i=0;i<80;i++){fly.stimulus=50;fly.ticks++;neural_step();}save("build/brain.ppm");fly.page=2;save("build/stats.ppm");fly.page=3;save("build/help.ppm");return 0;}
