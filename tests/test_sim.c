#include "fly.h"
#include <assert.h>
#include <stdio.h>
int main(void){
 assert(edge_count>0&&edge_count<=MAX_EDGES);
 for(unsigned i=0;i<edge_count;i++){assert(edges[i].pre<NEURONS&&edges[i].post<NEURONS);assert(edges[i].weight!=0);}
 sim_reset();fly.auto_mode=0;fly.heading=0;int x=fly.x,z=fly.z;
 for(int i=0;i<15;i++)sim_step(KEY_UP);assert(fly.z>z&&fly.x==x&&fly.state==WALK);
 for(int i=0;i<20;i++)sim_step(KEY_B);assert(fly.height==36&&fly.state==FLIGHT);
 for(int i=0;i<20;i++)sim_step(0);assert(fly.height==0);
 sim_step(KEY_A);assert(fly.state==GROOM);
 sim_step(0);sim_step(KEY_A|KEY_B);assert(fly.zoom==1);sim_step(0);sim_step(KEY_A|KEY_B);assert(fly.zoom==2);render();sim_step(0);sim_step(KEY_A|KEY_B);assert(fly.zoom==0);
 sim_step(0);sim_step(KEY_START);assert(fly.paused);uint32_t ticks=fly.ticks;
 for(int i=0;i<20;i++)sim_step(0);assert(fly.ticks==ticks);sim_step(KEY_START);assert(!fly.paused);
 sim_step(0);sim_step(KEY_R);assert(fly.page==1);sim_step(0);sim_step(KEY_RIGHT);assert(fly.selected==1);
 sim_reset();fly.auto_mode=0;fly.heading=0;fly.x=obstacles[0].x*256;fly.z=(obstacles[0].z-obstacles[0].r-6)*256;
 for(int i=0;i<15;i++)sim_step(KEY_UP);assert(fly.collisions>0&&fly.touch==100);
 uint32_t collisions=fly.collisions;fly.height=36;
 for(int i=0;i<40;i++)sim_step(KEY_UP|KEY_B);assert(fly.z>obstacles[0].z*256&&fly.collisions==collisions);
 sim_reset();fly.energy=0;sim_step(0);assert(fly.state==REST);
 sim_reset();fly.auto_mode=0;fly.x=food[0].x*256;fly.z=food[0].z*256;int hunger=fly.hunger;
 for(int i=0;i<300;i++)sim_step(0);
 printf("Contact circuit: %u spikes; %u feeding ticks; hunger %d -> %d\n",fly.spikes_total,fly.meals,hunger,fly.hunger);
 assert(fly.meals>0&&fly.hunger<hunger);
 sim_reset();for(int i=0;i<10800;i++){sim_step(0);assert(fly.x>=-150*256&&fly.x<=150*256&&fly.z>=-150*256&&fly.z<=150*256);assert(fly.energy>=0&&fly.energy<=1000&&fly.hunger>=0&&fly.hunger<=1000);if(i%30==0){fly.page=(i/30)%4;fly.zoom=(i/120)%3;fly.stats_detail=(i/240)%2;render();}}
 printf("Autonomous 6 minutes: path %u, feeding ticks %u, collisions %u\n",fly.distance/256,fly.meals,fly.collisions);assert(fly.distance>0&&fly.meals>0);
 /* Exercise the actual ROM sprite decoder in every view/direction/pose
    under ASan/UBSan, including launch/landing and folded-flight legs. */
 sim_reset();fly.auto_mode=0;fly.page=0;
 const int counts[6]={4,8,10,6,8,2};
 for(int view=0;view<3;view++)for(int heading=0;heading<256;heading+=4)
  for(int state=0;state<6;state++)for(int phase=0;phase<counts[state];phase++){
   fly.zoom=view;fly.heading=heading;fly.state=state;
   fly.height=state==FLIGHT?(phase==8?4:phase==9?14:36):0;
   fly.speed=state==WALK?220:0;fly.distance=(uint32_t)phase*256;
   fly.ticks=phase*(state==GROOM?2:state==FEED?3:state==REST?30:state==IDLE?12:1);
   render();
  }
 puts("All 7296 model view/direction/animation combinations rendered safely");
 /* A distant rock used to be painted over the compound eyes in this pose. */
 sim_reset();fly.auto_mode=0;fly.x=40*256;fly.z=-20*256;
 fly.heading=148;fly.state=GROOM;fly.ticks=6;render();
 int eyes=0;
 for(int y=54;y<77;y++)for(int x=65;x<96;x++){
  unsigned c=palette[pixels[y*W+x]];int r=c&31,g=(c>>5)&31,b=(c>>10)&31;
  if(r>12&&r>2*g&&r>2*b)eyes++;
 }
 assert(eyes>=2);puts("Distant rock does not cover the fly's eyes");
 puts("Simulation tests passed");return 0;
}
