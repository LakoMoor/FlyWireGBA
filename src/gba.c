#include "fly.h"
#define REG16(a) (*(volatile uint16_t*)(a))
#define REG32(a) (*(volatile uint32_t*)(a))
static uint32_t clock_cycles(void){uint16_t hi,lo;do{hi=REG16(0x04000104);lo=REG16(0x04000100);}while(hi!=REG16(0x04000104));return ((uint32_t)hi<<16)|lo;}
static void vblank(void){while(REG16(0x04000006)>=160){}while(REG16(0x04000006)<160){}}
static void dma(const void *src,void *dst,unsigned words){REG32(0x040000D4)=(uint32_t)src;REG32(0x040000D8)=(uint32_t)dst;REG32(0x040000DC)=0x84000000|words;}
int main(void){
 REG16(0x04000208)=0;REG16(0x04000204)=0x4317;
 REG16(0x04000000)=0x0080; /* forced blank */
 dma(palette,(void*)0x05000000,128);
 REG16(0x04000100)=0;REG16(0x04000104)=0;REG16(0x04000106)=0x0084;REG16(0x04000102)=0x0080;
 sim_reset();unsigned page=1;uint32_t last=clock_cycles(),sim_clock=last,accumulator=559240;
 for(;;){
  uint32_t start=clock_cycles();accumulator+=start-sim_clock;sim_clock=start;
  uint16_t keys=(~REG16(0x04000130))&0x3ff;
  while(accumulator>=559240){sim_step(keys);accumulator-=559240;}
  render();
  fly.render_cycles=(int)(clock_cycles()-start);
  if(REG16(0x04000006)<160||REG16(0x04000006)>170)vblank();
  while(clock_cycles()-last<550000)vblank();
  uint32_t now=clock_cycles(),elapsed=now-last;last=now;
  dma(pixels,(void*)(0x06000000+page*0xa000),W*H/4);
  REG16(0x04000000)=(uint16_t)(0x0404|(page<<4));page^=1;
  fly.frame_cycles=(int)elapsed;fly.fps=elapsed?16777216/elapsed:0;
  fly.cpu=clamp(elapsed?fly.render_cycles*100/(int)elapsed:0,0,999);
 }
}
