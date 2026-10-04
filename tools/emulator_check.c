/* Build against mGBA 0.10.5; reads actual emulated RAM and framebuffers. */
#include <mgba/core/core.h>
#include <mgba/core/config.h>
#include <mgba/core/log.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/memory.h>
#include <mgba/internal/arm/arm.h>
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
#define PREAD(field) core->busRead32(core,base+offsetof(Fly,pet)+offsetof(Pet,field))
#define PWRITE(field,value) core->busWrite32(core,base+offsetof(Fly,pet)+offsetof(Pet,field),(uint32_t)(value))
#define READ(field) core->busRead32(core,base+offsetof(Fly,field))
static void run(int n,unsigned keys){core->setKeys(core,keys);for(int i=0;i<n;i++)core->runFrame(core);}
static void save(const char *path){FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 160\n255\n");for(int i=0;i<240*160;i++){unsigned char rgb[3]={screen[i]&255,(screen[i]>>8)&255,(screen[i]>>16)&255};fwrite(rgb,1,3,f);}fclose(f);}
#define WRITE(field,value) core->busWrite32(core,base+offsetof(Fly,field),(uint32_t)(value))
static void model_capture(const char *path){FILE *f=fopen(path,"wb");assert(f);fprintf(f,"P6\n178 118\n255\n");for(int y=18;y<136;y++)for(int x=0;x<178;x++){color_t c=screen[y*240+x];unsigned char rgb[3]={c&255,(c>>8)&255,(c>>16)&255};fwrite(rgb,1,3,f);}fclose(f);}
static void flashcart_boot(void){
 core->reset(core);GBASkipBIOS(core->board);
 struct ARMCore *cpu=core->cpu;cpu->cpsr.i=1;
 /* Emulate launcher state rather than a clean emulator power-on. */
 GBAStore16(cpu,0x04000204,0x400c,NULL);
 GBAStore16(cpu,0x04000020,0x200,NULL);GBAStore16(cpu,0x04000022,0x40,NULL);
 GBAStore16(cpu,0x04000024,-0x40,NULL);GBAStore16(cpu,0x04000026,0x200,NULL);
 GBAStore32(cpu,0x04000028,30*256,NULL);GBAStore32(cpu,0x0400002c,20*256,NULL);
 GBAStore16(cpu,0x04000050,0x00c4,NULL);GBAStore16(cpu,0x04000054,16,NULL);
 GBAStore16(cpu,0x04000200,0x20,NULL);GBAStore16(cpu,0x04000208,1,NULL);
 GBAStore16(cpu,0x04000108,0xfff0,NULL);GBAStore16(cpu,0x0400010a,0xc0,NULL);
 GBAStore32(cpu,0x040000b0,0x0203fff0,NULL);GBAStore32(cpu,0x040000b4,0x06014000,NULL);
 GBAStore32(cpu,0x040000b8,0xa7400004,NULL);
 run(120,0);
 assert(READ(ticks)>30&&core->busRead16(core,0x04000204)==0x400c);
 /* Affine registers are write-only: verify their effect in pixels below. */
 assert(core->busRead16(core,0x04000050)==0);
 assert(core->busRead16(core,0x0400010a)==0&&core->busRead16(core,0x040000ba)==0);
 assert(core->busRead16(core,0x04000208)==0&&core->busRead16(core,0x04000200)==0);
 /* The whole displayed specimen must match, including affine hardware output. */
 WRITE(paused,1);run(8,0);
 for(unsigned i=0;i<sizeof(fly);i++)((uint8_t*)&fly)[i]=core->busRead8(core,base+i);
 render();int matched=0;
 for(int y=18;y<136;y++)for(int x=0;x<178;x++){
  unsigned c=palette[pixels[y*W+x]],actual=screen[y*W+x];
  int expected[3]={(c&31)*255/31,((c>>5)&31)*255/31,((c>>10)&31)*255/31};
  for(int channel=0;channel<3;channel++){int diff=(int)((actual>>(8*channel))&255)-expected[channel];assert(diff>=-1&&diff<=1);}matched++;
 }
 printf("Flashcart warm boot: %d pixels match, loader WAITCNT preserved, IRQ/DMA/timers stopped\n",matched);
 save("build/emulator-flashcart.ppm");
}
static int demo_frame;
static void demo(int frames,unsigned keys){for(int i=0;i<frames;i++){run(6,keys);char name[80];snprintf(name,sizeof(name),"build/demo-%03d.ppm",demo_frame++);save(name);}}
int main(int argc,char **argv){
 if(argc!=3){fprintf(stderr,"usage: emulator_check ROM FLY_SYMBOL_ADDRESS\n");return 1;}
 setbuf(stdout,NULL);mLogSetDefaultLogger(&logger);base=(uint32_t)strtoul(argv[2],NULL,16);core=mCoreFind(argv[1]);assert(core&&core->init(core));mCoreInitConfig(core,"flygba-test");
 core->setVideoBuffer(core,screen,240);assert(mCoreLoadFile(core,argv[1]));core->reset(core);core->busWrite8(core,0x0e000084,0);core->busWrite8(core,0x0e000184,0);core->reset(core);
 if(getenv("FLYGBA_WARM_BOOT_ONLY")){flashcart_boot();core->deinit(core);return 0;}
 run(120,0);printf("Boot: ticks=%u fps=%u cpu=%u frame_cycles=%u render_cycles=%u\n",READ(ticks),READ(fps),READ(cpu),READ(frame_cycles),READ(render_cycles));save("build/emulator-arena.ppm");assert(READ(ticks)>=45&&READ(ticks)<=65);
 run(16,KEY_SELECT);run(16,0);assert(READ(auto_mode)==0);
 run(50,KEY_B|KEY_UP);assert(READ(height)>0&&READ(state)==FLIGHT);save("build/emulator-flight.ppm");
 run(60,0);
 run(16,KEY_A);assert(READ(state)==GROOM);save("build/emulator-groom.ppm");run(16,0);
 core->busWrite32(core,base+offsetof(Fly,x),-95*256);core->busWrite32(core,base+offsetof(Fly,z),-80*256);
 core->busWrite32(core,base+offsetof(Fly,hunger),480);run(120,0);assert(READ(meals)>0);save("build/emulator-feed.ppm");
 run(16,KEY_A|KEY_B);run(30,0);assert(READ(zoom)==1);save("build/emulator-zoom.ppm");
 run(16,KEY_A|KEY_B);run(90,0);printf("Overhead: zoom=%u fps=%u cpu=%u\n",READ(zoom),READ(fps),READ(cpu));assert(READ(zoom)==2&&READ(fps)>=15);save("build/emulator-2d.ppm");
 run(16,KEY_R);run(120,0);assert(READ(page)==1);save("build/emulator-brain.ppm");
 run(16,KEY_R);run(120,0);assert(READ(page)==2);save("build/emulator-stats.ppm");printf("Metrics: fps=%u cpu=%u\n",READ(fps),READ(cpu));assert(READ(fps)>=25);
 run(16,KEY_B);run(60,0);assert(READ(stats_detail)==1);save("build/emulator-body.ppm");
 run(16,KEY_START);run(16,0);uint32_t tick=READ(ticks);run(120,0);assert(READ(ticks)==tick&&READ(paused));
 run(16,KEY_START);run(20,0);assert(READ(ticks)>tick);
 run(16,KEY_R);run(30,0);save("build/emulator-help.ppm");
 /* Visual checks from actual cartridge output, frozen through emulated RAM
    so every direction/behavior can be inspected without an opaque overlay. */
 core->reset(core);run(120,0);WRITE(auto_mode,0);WRITE(paused,1);WRITE(x,0);WRITE(z,0);
 const int pose_ticks[6]={0,3,4,6,6,0};
 for(int view=0;view<3;view++)for(int state=0;state<6;state++)for(int direction=0;direction<16;direction++){
  WRITE(zoom,view);WRITE(state,state);WRITE(heading,direction*16);WRITE(height,state==FLIGHT?36:0);
  WRITE(ticks,pose_ticks[state]);WRITE(distance,3*256);WRITE(speed,state==WALK?220:0);
  run(8,0);assert(READ(paused)&&READ(page)==0&&READ(state)==(unsigned)state&&READ(heading)==(unsigned)direction*16&&READ(zoom)==(unsigned)view);
  /* Compare the compiled ARM framebuffer to the native renderer. The scene
     is frozen; timer telemetry outside the viewport may legitimately differ. */
  for(unsigned i=0;i<sizeof(fly);i++)((uint8_t*)&fly)[i]=core->busRead8(core,base+i);
  render();
  for(int y=18;y<136;y++)for(int x=0;x<178;x++){
   unsigned c=palette[pixels[y*W+x]],actual=screen[y*240+x];
   int expected[3]={(c&31)*255/31,((c>>5)&31)*255/31,((c>>10)&31)*255/31};
   for(int channel=0;channel<3;channel++){int diff=(int)((actual>>(channel*8))&255)-expected[channel];assert(diff>=-1&&diff<=1);}
  }
  char name[96];snprintf(name,sizeof(name),"build/model-%d-%d-%02d.ppm",view,state,direction);model_capture(name);
 }
 puts("mGBA: 288 direction/behavior/view framebuffers match native rendering");
 WRITE(zoom,0);WRITE(x,40*256);WRITE(z,-20*256);WRITE(heading,148);
 WRITE(state,GROOM);WRITE(height,0);WRITE(ticks,6);run(8,0);
 int visible_eyes=0;
 for(int y=54;y<77;y++)for(int x=65;x<96;x++){
  unsigned c=screen[y*240+x];int r=c&255,g=(c>>8)&255,b=(c>>16)&255;
  if(r>96&&r>2*g&&r>2*b)visible_eyes++;
 }
 assert(visible_eyes>=2);save("build/emulator-occlusion.ppm");
 puts("mGBA: distant obstacle preserves compound-eye visibility");
 /* A deliberate walkthrough: walk, turn, take off, land, groom, feed,
    neural activity and telemetry, captured without replacing game pixels. */
 core->reset(core);run(120,0);run(16,KEY_SELECT);run(16,0);
 WRITE(x,-40*256);WRITE(z,0);WRITE(heading,24);
 demo(16,KEY_UP);demo(8,KEY_RIGHT);demo(16,KEY_B|KEY_UP);demo(10,0);demo(12,KEY_A);demo(2,0);
 WRITE(x,-95*256);WRITE(z,-80*256);WRITE(hunger,480);WRITE(heading,24);demo(12,0);
 run(16,KEY_R);demo(10,0);run(16,KEY_R);demo(10,0);
 /* Care actions use the real keypad; RAM setup chooses deterministic needs. */
 run(16,KEY_R);run(30,0);assert(READ(page)==3);
 run(16,KEY_R);run(60,0);assert(READ(page)==4&&PREAD(mode));
 printf("Pet: fps=%u cpu=%u\n",READ(fps),READ(cpu));assert(READ(fps)>=15);save("build/emulator-pet.ppm");run(16,KEY_B);run(16,0);assert(PREAD(menu));save("build/emulator-pet-menu.ppm");
 run(16,KEY_A);run(120,0);assert(PREAD(feeds)>0&&PREAD(hunger)<450);save("build/emulator-pet-feed.ppm");
 PWRITE(action,CARE_HIT);uint32_t health=PREAD(health);run(16,KEY_A);run(16,0);
 assert(PREAD(health)==health-100&&PREAD(hits)==1&&READ(state)==FLIGHT);save("build/emulator-pet-hit.ppm");
 run(80,0);PWRITE(action,CARE_SLEEP);run(16,KEY_A);run(30,0);assert(PREAD(sleeping));save("build/emulator-pet-sleep.ppm");
 uint32_t hits=PREAD(hits),feeds=PREAD(feeds);health=PREAD(health);
 void *sram=NULL;size_t save_size=core->savedataClone(core,&sram);assert(save_size==32768&&sram);
 core->deinit(core);core=mCoreFind(argv[1]);assert(core&&core->init(core));mCoreInitConfig(core,"flygba-test");
 core->setVideoBuffer(core,screen,240);assert(mCoreLoadFile(core,argv[1]));core->reset(core);run(120,0);assert(core->savedataRestore(core,sram,save_size,true));free(sram);
 core->reset(core);run(30,0);assert(READ(page)==4&&PREAD(mode)&&PREAD(sleeping)&&PREAD(hits)==hits&&PREAD(feeds)==feeds&&PREAD(health)>=health);
 puts("mGBA: care, MN9 feeding, hit/flee, sleep and SRAM reboot persistence passed");
 /* Simulate an interrupted bank write: ignore its missing commit marker. */
 uint32_t g0=0,g1=0;for(int i=0;i<4;i++){g0|=(uint32_t)core->busRead8(core,0x0e000080+i)<<(8*i);g1|=(uint32_t)core->busRead8(core,0x0e000180+i)<<(8*i);}
 unsigned bank=g1>g0?1:0;core->busWrite8(core,0x0e000084+bank*256,0);
 core->reset(core);run(30,0);assert(READ(page)==4&&PREAD(hits)==1);
 puts("mGBA: interrupted SRAM write recovers the previous valid bank");
 /* Record a short pet walkthrough from actual cartridge output. */
 PWRITE(sleeping,0);PWRITE(action,CARE_PET);run(40,0);
 for(int a=0;a<CARE_COUNT;a++){
  PWRITE(action,a);run(16,KEY_B);run(16,0);
  for(int i=0;i<5;i++){run(6,0);char name[80];snprintf(name,sizeof(name),"build/pet-%03d.ppm",a*15+i);save(name);}
  run(16,KEY_A);
  for(int i=5;i<15;i++){run(6,0);char name[80];snprintf(name,sizeof(name),"build/pet-%03d.ppm",a*15+i);save(name);}
 }
 PWRITE(health,0);run(16,0);assert(PREAD(dead));save("build/emulator-pet-dead.ppm");
 run(200,KEY_A|KEY_B);assert(!PREAD(dead)&&PREAD(hits)==0);run(16,0);
 core->busWrite8(core,0x0e000084,0);core->busWrite8(core,0x0e000184,0);
 core->reset(core);run(30,0);assert(READ(page)==0&&PREAD(health)==1000);
 puts("mGBA: death, held-button new pet and invalid-save fallback passed");
 flashcart_boot();
 core->deinit(core);puts("mGBA boot, movement, flight, tabs and pause passed");return 0;
}
