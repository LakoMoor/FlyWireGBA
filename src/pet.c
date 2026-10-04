#include "fly.h"
#ifdef __arm__
#define PET_CODE __attribute__((section(".romcode"),noinline))
#else
#define PET_CODE __attribute__((noinline))
#endif
const char *const care_names[CARE_COUNT]={"FEED","WATER","TOUCH","PLAY","CLEAN","SLEEP","MEDIC","HIT"};
PET_CODE void pet_reset(void){
 uint8_t *bytes=(uint8_t*)&fly.pet;for(unsigned i=0;i<sizeof(Pet);i++)bytes[i]=0;
 fly.pet.health=1000;fly.pet.hunger=450;fly.pet.energy=850;fly.pet.water=800;
 fly.pet.clean=850;fly.pet.bond=500;fly.pet.stress=80;fly.pet.dirty=1;
}
PET_CODE void pet_enter(void){
 fly.pet.mode=1;fly.pet.dirty=1;fly.x=0;fly.z=0;fly.height=0;fly.heading=24;fly.speed=0;
 fly.energy=fly.pet.energy;fly.hunger=fly.pet.hunger;
}
PET_CODE void pet_leave(void){
 fly.pet.mode=0;fly.pet.dirty=1;fly.pet.menu=0;fly.x=-40*256;fly.z=-10*256;fly.height=0;
 fly.energy=900;fly.hunger=480;fly.auto_mode=1;fly.speed=0;
}
PET_CODE const char *pet_status(void){
 const Pet *p=&fly.pet;
 if(p->dead)return "DEAD";
 if(p->sleeping)return "ASLEEP";
 if(p->fear_timer||p->stress>650)return "AFRAID";
 if(p->health<300)return "INJURED";
 if(p->water<300)return "THIRSTY";
 if(p->hunger>650)return "HUNGRY";
 if(p->energy<200)return "TIRED";
 if(p->clean<300)return "DIRTY";
 return p->bond>700?"CONTENT":"CURIOUS";
}
static PET_CODE void care(void){
 Pet *p=&fly.pet;if(p->dead||p->cooldown)return;
 p->cooldown=15;p->feedback=p->action+1;p->feedback_timer=75;p->dirty=1;
 switch(p->action){
 case CARE_FEED:p->sleeping=0;p->food_timer=120;break;
 case CARE_WATER:p->water=clamp(p->water+350,0,1000);p->bond=clamp(p->bond+20,0,1000);break;
 case CARE_PET:p->stress=clamp(p->stress-140,0,1000);p->fear_timer=clamp(p->fear_timer-60,0,120);p->bond=clamp(p->bond+60,0,1000);break;
 case CARE_PLAY:
  if(p->energy<150){p->feedback=9;break;}
  p->sleeping=0;p->play_timer=120;p->energy-=80;p->bond=clamp(p->bond+70,0,1000);p->stress=clamp(p->stress-100,0,1000);break;
 case CARE_CLEAN:p->clean=1000;p->stress=clamp(p->stress-60,0,1000);break;
 case CARE_SLEEP:p->sleeping=!p->sleeping;p->food_timer=0;p->play_timer=0;p->fear_timer=0;break;
 case CARE_MEDIC:
  if(p->medicine){p->feedback=10;break;}
  p->health=clamp(p->health+220,0,1000);p->medicine=900;p->stress=clamp(p->stress-80,0,1000);break;
 case CARE_HIT:
  p->sleeping=0;p->food_timer=0;p->health=clamp(p->health-100,0,1000);
  p->stress=clamp(p->stress+250,0,1000);p->bond=clamp(p->bond-140,0,1000);
  p->fear_timer=120;p->hits++;fly.heading=(fly.heading+128)&255;break;
 }
}
PET_CODE void pet_step(uint16_t keys,uint16_t hit){
 Pet *p=&fly.pet;
 if(fly.page==4){
  if((keys&(KEY_A|KEY_B))==(KEY_A|KEY_B)){
   if(p->reset_hold>=0&&++p->reset_hold>=90){pet_reset();pet_enter();fly.last_keys=keys;p=&fly.pet;p->cooldown=30;p->reset_hold=-1;}
  }else{
   p->reset_hold=0;
   if(hit&KEY_RIGHT)p->action=(p->action+1)%CARE_COUNT;
   if(hit&KEY_LEFT)p->action=(p->action+CARE_COUNT-1)%CARE_COUNT;
   if(hit&KEY_DOWN)p->action=(p->action+2)%CARE_COUNT;
   if(hit&KEY_UP)p->action=(p->action+CARE_COUNT-2)%CARE_COUNT;
   if(hit&KEY_B)p->menu=!p->menu;
   if(hit&KEY_A){care();p->menu=0;}
  }
 }
 if(p->cooldown)p->cooldown--;
 if(p->medicine)p->medicine--;
 if(p->feedback_timer)p->feedback_timer--;
 fly.speed=0;fly.touch=0;fly.stimulus=0;
 if(!p->dead&&p->health==0){p->dead=1;p->sleeping=0;p->food_timer=0;p->fear_timer=0;p->dirty=1;}
 if(p->dead){fly.state=REST;fly.height=0;fly.energy=p->energy;fly.hunger=p->hunger;neural_step();return;}
 p->age++;
 if(p->age%30==0){
  p->hunger=clamp(p->hunger+(p->sleeping?2:5),0,1000);
  p->water=clamp(p->water-(p->sleeping?2:4),0,1000);p->clean=clamp(p->clean-3,0,1000);
  p->energy=clamp(p->energy+(p->sleeping?12:-2),0,1000);p->stress=clamp(p->stress-2,0,1000);
  int harm=(p->hunger>850?6:0)+(p->water<150?6:0)+(p->clean<150?2:0)+(p->energy<100?2:0);
  p->health=clamp(p->health-harm+(p->sleeping&&p->hunger<650&&p->water>400&&p->clean>500?3:0),0,1000);
 }
 if(p->age%900==0)p->dirty=1;
 if(p->fear_timer){
  p->fear_timer--;fly.state=FLIGHT;fly.height=clamp(fly.height+2,0,36);fly.speed=p->health>300?220:100;
 }else if(p->sleeping){fly.state=REST;fly.height=clamp(fly.height-2,0,36);}
 else if(p->food_timer){
  p->food_timer--;fly.height=clamp(fly.height-2,0,36);fly.state=FEED;
  if(!fly.height){fly.stimulus=50;fly.touch=40;}
 }else if(p->feedback_timer&&(p->feedback==CARE_CLEAN+1||p->feedback==CARE_PET+1)){fly.state=GROOM;fly.height=clamp(fly.height-2,0,36);}
 else if(p->play_timer){p->play_timer--;fly.state=WALK;fly.speed=180;fly.heading=(fly.heading+3)&255;fly.height=clamp(fly.height-2,0,36);}
 else{
  fly.height=clamp(fly.height-2,0,36);
  fly.state=p->energy<100?REST:(p->age%240<60?WALK:p->age%240>200?GROOM:IDLE);
  if(fly.state==WALK){fly.speed=100;if(p->age%30==0)fly.heading=(fly.heading+20)&255;}
 }
 int oldx=fly.x,oldz=fly.z;
 fly.x+=isin(fly.heading)*fly.speed/256;fly.z+=icos(fly.heading)*fly.speed/256;
 if(fly.x>70*256||fly.x<-70*256||fly.z>70*256||fly.z<-70*256){fly.heading=(fly.heading+128)&255;fly.x=clamp(fly.x,-70*256,70*256);fly.z=clamp(fly.z,-70*256,70*256);}
 int dx=fly.x-oldx,dz=fly.z-oldz;fly.distance+=(uint32_t)((dx<0?-dx:dx)+(dz<0?-dz:dz));
 neural_step();
 if(fly.state==FEED&&fly.stimulus&&fly.motor){p->hunger=clamp(p->hunger-8,0,1000);p->energy=clamp(p->energy+12,0,1000);p->feeds++;fly.meals++;p->dirty=1;}
 fly.energy=p->energy;fly.hunger=p->hunger;fly.state_ticks[fly.state]++;
}
static PET_CODE uint32_t checksum(const uint8_t *data,unsigned n){uint32_t h=2166136261u;for(unsigned i=0;i<n;i++)h=(h^data[i])*16777619u;return h;}
static PET_CODE void word(uint8_t *p,uint32_t v){for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(i*8));}
static PET_CODE uint32_t readword(const uint8_t *p){return p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
PET_CODE void pet_encode(uint8_t out[PET_SAVE_SIZE]){
 for(int i=0;i<PET_SAVE_SIZE;i++)out[i]=0;
 out[0]='F';out[1]='P';out[2]='E';out[3]='T';out[4]=1;out[5]=(uint8_t)sizeof(Pet);
 const uint8_t *raw=(const uint8_t*)&fly.pet;for(unsigned i=0;i<sizeof(Pet);i++)out[16+i]=raw[i];
 word(out+8,checksum(out+16,PET_SAVE_SIZE-16));
}
PET_CODE int pet_decode(const uint8_t in[PET_SAVE_SIZE]){
 if(in[0]!='F'||in[1]!='P'||in[2]!='E'||in[3]!='T'||in[4]!=1||in[5]!=sizeof(Pet)||readword(in+8)!=checksum(in+16,PET_SAVE_SIZE-16))return 0;
 Pet p;uint8_t *raw=(uint8_t*)&p;for(unsigned i=0;i<sizeof(Pet);i++)raw[i]=in[16+i];
 if((p.mode!=0&&p.mode!=1)||p.health<0||p.health>1000||p.hunger<0||p.hunger>1000||p.energy<0||p.energy>1000||p.water<0||p.water>1000||p.clean<0||p.clean>1000||p.stress<0||p.stress>1000||p.bond<0||p.bond>1000||p.action<0||p.action>=CARE_COUNT||p.food_timer<0||p.food_timer>120||p.fear_timer<0||p.fear_timer>120||p.play_timer<0||p.play_timer>120||p.cooldown<0||p.cooldown>30||p.medicine<0||p.medicine>900||p.feedback<0||p.feedback>10||p.feedback_timer<0||p.feedback_timer>75||(p.dead!=0&&p.dead!=1)||(p.sleeping!=0&&p.sleeping!=1))return 0;
 p.mode=0;p.menu=0;p.dirty=0;p.reset_hold=0;fly.pet=p;return 1;
}
_Static_assert(sizeof(Pet)<=PET_SAVE_SIZE-16,"Pet save exceeds SRAM record");
