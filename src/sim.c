#include "fly.h"
Fly fly;
const Object food[FOOD_COUNT]={{-95,-80,13},{100,45,14},{-60,115,12},{120,-115,11}};
const Object obstacles[OBSTACLE_COUNT]={{40,-55,22},{-110,40,18},{65,115,16}};
const char *const state_names[6]={"IDLE","WALK","FLIGHT","FEED","GROOM","REST"};
static const int16_t sine[65]={0,6,13,19,25,31,38,44,50,56,62,68,74,80,86,92,98,104,109,115,121,126,132,137,142,147,152,157,162,167,172,177,181,185,190,194,198,202,206,210,213,217,220,223,226,229,231,234,236,239,241,243,245,247,248,250,251,252,253,254,255,255,256,256,256};
int clamp(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
int isin(int a){a&=255; if(a<64)return sine[a]; if(a<128)return sine[128-a]; if(a<192)return -sine[a-128];return -sine[256-a];}
int icos(int a){return isin(a+64);}
static int approx_distance(int dx,int dz){if(dx<0)dx=-dx;if(dz<0)dz=-dz;return dx>dz?dx+(dz>>1):dz+(dx>>1);}
void sim_reset(void){
 uint8_t *p=(uint8_t*)&fly;for(unsigned i=0;i<sizeof(fly);i++)p[i]=0;
 fly.x=-40*256;fly.z=-10*256;fly.heading=148;fly.energy=900;fly.hunger=480;fly.auto_mode=1;fly.zoom=0;fly.show_links=1;fly.rng=0xF17E1234;fly.fps=30;
}
void neural_step(void){
 /* One 33.3 ms illustrative LIF tick. Anatomical edges are real; this is not
    the calibrated 0.1 ms Shiu model. Synchronous spikes have one-tick delay. */
 for(int i=0;i<NEURONS;i++)fly.input[i]=0;
 for(unsigned e=0;e<edge_count;e++)if(fly.spike[edges[e].pre]){
  int j=edges[e].post;fly.input[j]=(int16_t)clamp(fly.input[j]+edges[e].weight,-2048,2048);
 }
 fly.spike_count=0;fly.active=0;fly.motor=0;
 for(int i=0;i<NEURONS;i++){
  int drive=0;
  if(neuron_group[i]==0)drive=fly.stimulus*3; /* sugar contact only */
  int v=fly.voltage[i]-fly.voltage[i]/8+fly.input[i]+drive;
  fly.spike[i]=0;
  if(fly.refractory[i]){fly.refractory[i]--;v=0;}
  else if(v>=100){fly.spike[i]=1;v=0;fly.refractory[i]=1;fly.glow[i]=255;fly.spike_count++;}
  fly.voltage[i]=(int16_t)clamp(v,-200,200);
  if(fly.glow[i])fly.glow[i]=(uint8_t)(fly.glow[i]>24?fly.glow[i]-24:0);
  if(fly.glow[i])fly.active++;
  if(neuron_group[i]==2&&fly.spike[i])fly.motor++;
 }
 fly.spikes_total+=fly.spike_count;fly.second_spikes+=fly.spike_count;
 if(fly.ticks%STEP_HZ==0){fly.rate=fly.second_spikes;fly.second_spikes=0;}
 fly.history[fly.history_pos]=fly.spike_count;fly.history_pos=(fly.history_pos+1)%HISTORY;
}
void sim_step(uint16_t keys){
 uint16_t hit=keys&~fly.last_keys;fly.last_keys=keys;
 if(hit&KEY_R)fly.page=(fly.page+1)%4;
 if(hit&KEY_L)fly.page=(fly.page+3)%4;
 if(hit&KEY_START)fly.paused=!fly.paused;
 if(hit&KEY_SELECT)fly.auto_mode=!fly.auto_mode;
 if(fly.page==1){
  if(hit&KEY_RIGHT)fly.selected=(fly.selected+1)%NEURONS;
  if(hit&KEY_LEFT)fly.selected=(fly.selected+NEURONS-1)%NEURONS;
  if(hit&KEY_UP)fly.selected=(fly.selected+NEURONS-8)%NEURONS;
  if(hit&KEY_DOWN)fly.selected=(fly.selected+8)%NEURONS;
  if(hit&KEY_A)fly.show_links=!fly.show_links;
 }
 if(fly.page==2&&(hit&KEY_B))fly.stats_detail=!fly.stats_detail;
 if(fly.page==2&&(hit&KEY_A)){int page=fly.page;sim_reset();fly.page=page;fly.last_keys=keys;return;}
 if(fly.page==0&&(hit&KEY_A)&&(keys&KEY_B))fly.zoom=(fly.zoom+1)%3;
 if(fly.paused)return;
 fly.ticks++;
 int x=fly.x/256,z=fly.z/256;
 fly.food_dist=10000;fly.nearest=0;
 for(int i=0;i<FOOD_COUNT;i++){int d=approx_distance(food[i].x-x,food[i].z-z)-food[i].r;if(d<fly.food_dist){fly.food_dist=d;fly.nearest=i;}}
 fly.food_dist=clamp(fly.food_dist,0,10000);fly.odor=clamp(100-fly.food_dist/2,0,100);
 fly.touch=0;int oldx=fly.x,oldz=fly.z;
 int move=0,turn=0,flying=(keys&KEY_B)&&fly.page==0&&!(keys&KEY_A);
 int grooming=(keys&KEY_A)&&fly.page==0&&!flying&&!(keys&KEY_B);
 if(fly.auto_mode){
  int tx=food[fly.nearest].x-x,tz=food[fly.nearest].z-z;
  int cross=icos(fly.heading)*tx-isin(fly.heading)*tz;
  int dot=isin(fly.heading)*tx+icos(fly.heading)*tz;
  turn=cross>500?2:(cross<-500?-2:0);if(dot<0&&!turn)turn=3;
  move=fly.food_dist>1;
  if(fly.hunger<80){move=(fly.ticks%180)<75;turn=(fly.ticks%180)<10?2:0;grooming=!move;}
  if(fly.energy<100){move=0;grooming=0;}
  if(fly.state==FLIGHT&&fly.height>0)flying=1;
  /* On contact with an obstacle, briefly fly over it. */
  if(fly.ticks%240>190&&fly.state_ticks[FLIGHT]<fly.ticks/3)flying=1;
 }
 if(!fly.auto_mode&&fly.page==0){turn=((keys&KEY_RIGHT)?3:0)-((keys&KEY_LEFT)?3:0);move=(keys&KEY_UP)?1:((keys&KEY_DOWN)?-1:0);}
 if(fly.auto_mode&&flying&&fly.ticks%240<180)flying=0;
 fly.heading=(fly.heading+turn)&255;
 if(flying&&fly.energy>0)fly.height=clamp(fly.height+2,0,36);else fly.height=clamp(fly.height-2,0,36);
 fly.speed=move*(fly.height>4?460:220);
 if(grooming)fly.speed=0;
 if(fly.energy==0)fly.speed=0;
 fly.x+=isin(fly.heading)*fly.speed/256;fly.z+=icos(fly.heading)*fly.speed/256;
 for(int i=0;i<OBSTACLE_COUNT;i++){
  int dx=fly.x/256-obstacles[i].x,dz=fly.z/256-obstacles[i].z,r=obstacles[i].r+5;
  if(fly.height<24&&dx*dx+dz*dz<r*r){fly.x=oldx;fly.z=oldz;fly.touch=100;fly.collisions++;if(fly.auto_mode){fly.heading=(fly.heading+24)&255;fly.height=clamp(fly.height+4,0,36);}break;}
 }
 fly.x=clamp(fly.x,-150*256,150*256);fly.z=clamp(fly.z,-150*256,150*256);
 if(fly.x==oldx&&fly.z==oldz)fly.speed=0;
 fly.distance+=(uint32_t)approx_distance(fly.x-oldx,fly.z-oldz);
 fly.stimulus=(fly.food_dist<5&&fly.height==0)?50:0;
 neural_step();
 if(fly.height)fly.state=FLIGHT;
 else if(grooming)fly.state=GROOM;
 else if(fly.stimulus&&fly.hunger>0)fly.state=FEED;
 else if(fly.speed)fly.state=WALK;
 else if(fly.energy<100)fly.state=REST;
 else fly.state=IDLE;
 fly.state_ticks[fly.state]++;
 /* Feeding is gated by the MN9 response from the reduced anatomical circuit. */
 if(fly.state==FEED&&fly.motor){fly.hunger=clamp(fly.hunger-8,0,1000);fly.energy=clamp(fly.energy+12,0,1000);fly.meals++;}
 if(fly.ticks%30==0){fly.hunger=clamp(fly.hunger+3,0,1000);fly.energy=clamp(fly.energy-(fly.state==FLIGHT?5:fly.speed?2:-3),0,1000);}
}
