#include "fly.h"
#define REG16(a) (*(volatile uint16_t*)(a))
#define REG32(a) (*(volatile uint32_t*)(a))
__attribute__((used,section(".save_signature"))) const char save_signature[]="SRAM_V113";
static uint8_t save_record[PET_SAVE_SIZE];
static uint32_t save_generation;
static unsigned save_bank;
static __attribute__((section(".romcode"),noinline)) void save_load(void){
 volatile const uint8_t *sram=(volatile const uint8_t*)0x0e000000;
 Pet best=fly.pet;int found=0,resume=0;
 for(unsigned bank=0;bank<2;bank++){
  unsigned offset=bank*256;
  if(sram[offset+132]!=0xa5)continue;
  for(int i=0;i<PET_SAVE_SIZE;i++)save_record[i]=sram[offset+i];
  uint32_t generation=0;for(int i=0;i<4;i++)generation|=(uint32_t)sram[offset+128+i]<<(i*8);
  if(pet_decode(save_record)&&(!found||(int32_t)(generation-save_generation)>0)){
   best=fly.pet;save_generation=generation;save_bank=bank;found=1;resume=save_record[16]==1;
  }
 }
 fly.pet=best;
 if(found&&resume){fly.page=4;pet_enter();}
}
static __attribute__((section(".romcode"),noinline)) void save_store(void){
 if(!fly.pet.dirty)return;
 volatile uint8_t *sram=(volatile uint8_t*)0x0e000000;
 unsigned bank=save_bank^1,offset=bank*256;uint32_t generation=save_generation+1;
 pet_encode(save_record);sram[offset+132]=0;
 for(int i=0;i<PET_SAVE_SIZE;i++)sram[offset+i]=save_record[i];
 for(int i=0;i<4;i++)sram[offset+128+i]=(uint8_t)(generation>>(i*8));
 sram[offset+132]=0xa5;save_bank=bank;save_generation=generation;fly.pet.dirty=0;
}
static uint32_t clock_cycles(void){uint16_t hi,lo;do{hi=REG16(0x04000104);lo=REG16(0x04000100);}while(hi!=REG16(0x04000104));return ((uint32_t)hi<<16)|lo;}
static void vblank(void){while(REG16(0x04000006)>=160){}while(REG16(0x04000006)<160){}}
static void dma(const void *src,void *dst,unsigned words){REG32(0x040000D4)=(uint32_t)src;REG32(0x040000D8)=(uint32_t)dst;REG32(0x040000DC)=0x84000000|words;}
int main(void){
 REG16(0x04000208)=0;
 /* Keep the cartridge timings selected by BIOS/flashcart firmware. */
 REG16(0x04000000)=0x0080; /* forced blank */
 /* Mode 4 uses affine BG2; a launcher may leave a transformed background. */
 REG16(0x0400000c)=0;
 REG16(0x04000020)=0x100;REG16(0x04000022)=0;
 REG16(0x04000024)=0;REG16(0x04000026)=0x100;
 REG32(0x04000028)=0;REG32(0x0400002c)=0;
 REG16(0x04000050)=0;REG16(0x04000052)=0;REG16(0x04000054)=0;
 dma(palette,(void*)0x05000000,128);
 REG16(0x04000100)=0;REG16(0x04000104)=0;REG16(0x04000106)=0x0084;REG16(0x04000102)=0x0080;
 sim_reset();save_load();unsigned page=1;uint32_t last=clock_cycles(),sim_clock=last,accumulator=559240;
 for(;;){
  uint32_t start=clock_cycles();accumulator+=start-sim_clock;sim_clock=start;
  uint16_t keys=(~REG16(0x04000130))&0x3ff;
  while(accumulator>=559240){sim_step(keys);accumulator-=559240;}
  render();save_store();
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
