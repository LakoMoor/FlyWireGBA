/* Build against mGBA 0.10.5; reads actual emulated RAM and framebuffers. */
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include "fly.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
static void quiet(struct mLogger *l,int c,enum mLogLevel level,const char *f,va_list args){(void)l;(void)c;(void)level;(void)f;(void)args;}
static struct mLogger logger={quiet,NULL};
static struct mCore *core;
static color_t screen[240*160];
static uint32_t base;
#define READ(field) core->busRead32(core,base+offsetof(Fly,field))
static void run(int n,unsigned keys){core->setKeys(core,keys);for(int i=0;i<n;i++)core->runFrame(core);}
static void save(const char *path){FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 160\n255\n");for(int i=0;i<240*160;i++){unsigned char rgb[3]={screen[i]&255,(screen[i]>>8)&255,(screen[i]>>16)&255};fwrite(rgb,1,3,f);}fclose(f);}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: emulator_check ROM FLY_SYMBOL_ADDRESS\n");return 1;}
 setbuf(stdout,NULL);mLogSetDefaultLogger(&logger);base=(uint32_t)strtoul(argv[2],NULL,16);core=mCoreFind(argv[1]);assert(core&&core->init(core));mCoreInitConfig(core,"flygba-test");
 core->setVideoBuffer(core,screen,240);assert(mCoreLoadFile(core,argv[1]));core->reset(core);
 run(120,0);printf("Boot: ticks=%u fps=%u cpu=%u frame_cycles=%u render_cycles=%u\n",READ(ticks),READ(fps),READ(cpu),READ(frame_cycles),READ(render_cycles));save("build/emulator-arena.ppm");assert(READ(ticks)>=45&&READ(ticks)<=65);
 run(16,KEY_SELECT);run(16,0);assert(READ(auto_mode)==0);
 run(50,KEY_B|KEY_UP);assert(READ(height)>0&&READ(state)==FLIGHT);save("build/emulator-flight.ppm");
 run(60,0);
 run(16,KEY_A);assert(READ(state)==GROOM);save("build/emulator-groom.ppm");run(16,0);
 core->busWrite32(core,base+offsetof(Fly,x),-95*256);core->busWrite32(core,base+offsetof(Fly,z),-80*256);
 core->busWrite32(core,base+offsetof(Fly,hunger),480);run(120,0);assert(READ(meals)>0);save("build/emulator-feed.ppm");
 run(16,KEY_A|KEY_B);run(30,0);assert(READ(zoom)==1);save("build/emulator-zoom.ppm");
 run(16,KEY_A|KEY_B);run(90,0);assert(READ(zoom)==2&&READ(fps)>=15);save("build/emulator-2d.ppm");printf("2D: fps=%u cpu=%u\n",READ(fps),READ(cpu));
 run(16,KEY_R);run(120,0);assert(READ(page)==1);save("build/emulator-brain.ppm");
 run(16,KEY_R);run(120,0);assert(READ(page)==2);save("build/emulator-stats.ppm");printf("Metrics: fps=%u cpu=%u\n",READ(fps),READ(cpu));assert(READ(fps)>=25);
 run(16,KEY_B);run(60,0);assert(READ(stats_detail)==1);save("build/emulator-body.ppm");
 run(16,KEY_START);run(16,0);uint32_t tick=READ(ticks);run(120,0);assert(READ(ticks)==tick&&READ(paused));
 run(16,KEY_START);run(20,0);assert(READ(ticks)>tick);
 run(16,KEY_R);run(30,0);save("build/emulator-help.ppm");
 core->reset(core);unsigned keys[6]={0,KEY_UP|KEY_RIGHT,KEY_B|KEY_UP,0,KEY_A,0};
 for(int segment=0;segment<6;segment++){
  if(segment==1){run(16,KEY_SELECT);run(16,0);}
  for(int frame=0;frame<15;frame++){run(6,keys[segment]);char name[80];snprintf(name,sizeof(name),"build/demo-%03d.ppm",segment*15+frame);save(name);}
 }
 core->deinit(core);puts("mGBA boot, movement, flight, tabs and pause passed");return 0;
}
