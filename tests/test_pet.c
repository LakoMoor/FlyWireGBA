#include "fly.h"
#include <assert.h>
#include <stdio.h>
static void advance(int n){while(n--)sim_step(0);}
static void action(int a){advance(31);fly.pet.action=a;sim_step(KEY_A);sim_step(0);}
int main(void){
 sim_reset();sim_step(KEY_L);assert(fly.page==4&&fly.pet.mode);advance(1);
 int hunger=fly.pet.hunger;action(CARE_FEED);advance(120);
 assert(fly.pet.feeds>0&&fly.pet.hunger<hunger&&fly.meals>0);
 fly.pet.water=100;action(CARE_WATER);assert(fly.pet.water>400);
 fly.pet.clean=100;action(CARE_CLEAN);assert(fly.pet.clean==1000);
 fly.pet.stress=600;int bond=fly.pet.bond;action(CARE_PET);assert(fly.pet.stress<500&&fly.pet.bond>bond);
 int energy=fly.pet.energy;action(CARE_PLAY);assert(fly.pet.energy<energy&&fly.pet.play_timer>0);
 fly.pet.energy=300;action(CARE_SLEEP);energy=fly.pet.energy;advance(90);assert(fly.pet.sleeping&&fly.pet.energy>energy&&fly.state==REST);
 action(CARE_SLEEP);assert(!fly.pet.sleeping);
 fly.pet.health=400;action(CARE_MEDIC);assert(fly.pet.health==620&&fly.pet.medicine>0);action(CARE_MEDIC);assert(fly.pet.health==620&&fly.pet.feedback==10);
 int health=fly.pet.health;action(CARE_HIT);assert(fly.pet.health==health-100&&fly.pet.hits==1&&fly.state==FLIGHT&&fly.pet.fear_timer>0);
 for(int i=0;i<60;i++)sim_step(KEY_A);assert(fly.pet.hits==1);
 uint32_t age=fly.pet.age;sim_step(0);sim_step(KEY_START);advance(60);assert(fly.pet.age==age+1);sim_step(KEY_START);sim_step(0);
 sim_step(KEY_R);assert(fly.page==0&&!fly.pet.mode);age=fly.pet.age;advance(120);assert(fly.pet.age==age);
 sim_step(KEY_L);sim_step(0);sim_step(KEY_L);assert(fly.page==3&&fly.pet.mode);age=fly.pet.age;advance(60);assert(fly.pet.age>age);
 sim_step(KEY_L);assert(fly.page==2);health=fly.pet.health;sim_step(0);sim_step(KEY_A);assert(fly.pet.health==health&&fly.pet.mode&&fly.page==2);
 uint8_t record[PET_SAVE_SIZE];pet_encode(record);Pet saved=fly.pet;pet_reset();assert(pet_decode(record));assert(fly.pet.health==saved.health&&fly.pet.feeds==saved.feeds&&fly.pet.hits==saved.hits&&!fly.pet.mode);
 record[30]^=1;health=fly.pet.health;assert(!pet_decode(record)&&fly.pet.health==health);
 fly.pet.health=1001;pet_encode(record);assert(!pet_decode(record));
 sim_reset();sim_step(KEY_L);advance(20000);assert(fly.pet.dead&&fly.pet.health==0);age=fly.pet.age;action(CARE_MEDIC);assert(fly.pet.dead&&fly.pet.age==age);
 for(int i=0;i<150;i++)sim_step(KEY_A|KEY_B);assert(!fly.pet.dead&&fly.pet.age>=60&&fly.pet.age<100&&fly.pet.hits==0);
 for(int v=0;v<3;v++)for(int a=0;a<CARE_COUNT;a++){fly.zoom=v;fly.pet.action=a;fly.pet.menu=1;render();fly.pet.menu=0;action(a);render();}
 puts("Pet care, MN9 feeding, neglect, reset, pause, page routing and save validation passed");
 return 0;
}
