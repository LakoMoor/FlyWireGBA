#include "fly.h"
#include "font.h"
#define UI_PAGE __attribute__((noinline))
#ifdef __arm__
#define PET_PAGE __attribute__((section(".romcode"),noinline))
#else
#define PET_PAGE __attribute__((noinline))
#endif

/* One compact layout for the 240x160 LCD: 18px header, 118px content,
   11px context strip and 13px navigation. World drawing is clipped. */
enum { BG, PANEL, BORDER, CARD, DIM, MUTED, INK, WHITE, MINT, TEAL,
       GOLD, BROWN, CORAL, RED, ICE, BLUE };
uint8_t pixels[W*H] __attribute__((aligned(4)));
static int clip_left,clip_top,clip_right=W,clip_bottom=H;
static void clip(int x,int y,int w,int h){clip_left=x;clip_top=y;clip_right=x+w;clip_bottom=y+h;}
void putpixel(int x,int y,int c){
 if(x>=clip_left&&x<clip_right&&y>=clip_top&&y<clip_bottom)pixels[y*W+x]=(uint8_t)c;
}
static void span(int x,int y,int width,int c){
 if(y<clip_top||y>=clip_bottom)return;
 int end=x+width;if(end>clip_right)end=clip_right;if(x<clip_left)x=clip_left;
 if(x>=end)return;
 uint32_t word=(uint8_t)c;word|=word<<8;word|=word<<16;
 uint8_t *row=pixels+y*W;
 for(;x<end&&(x&3);x++)row[x]=(uint8_t)c;
 for(;x+4<=end;x+=4)*(uint32_t*)(void*)(row+x)=word;
 for(;x<end;x++)row[x]=(uint8_t)c;
}
void rect(int x,int y,int w,int h,int c){
 int end=y+h;if(end>clip_bottom)end=clip_bottom;if(y<clip_top)y=clip_top;
 for(;y<end;y++)span(x,y,w,c);
}
void line(int x,int y,int x1,int y1,int c){
 int dx=x1-x,dy=y1-y;
 for(int axis=0;axis<2;axis++){
  int lo=axis?clip_top:clip_left,hi=(axis?clip_bottom:clip_right)-1;
  int a=axis?y:x,b=axis?y1:x1;
  if((a<lo&&b<lo)||(a>hi&&b>hi))return;
  if(a<lo||a>hi){int bound=a<lo?lo:hi;
   if(axis){x+=dx*(bound-y)/dy;y=bound;}else{y+=dy*(bound-x)/dx;x=bound;}}
  b=axis?y1:x1;dx=x1-x;dy=y1-y;
  if(b<lo||b>hi){int bound=b<lo?lo:hi;
   if(axis){x1=x+dx*(bound-y)/dy;y1=bound;}else{y1=y+dy*(bound-x)/dx;x1=bound;}}
  dx=x1-x;dy=y1-y;
 }
 dx=x1-x;if(dx<0)dx=-dx;dy=y1-y;if(dy<0)dy=-dy;
 int sx=x<x1?1:-1,sy=y<y1?1:-1,err=dx-dy;
 for(;;){putpixel(x,y,c);if(x==x1&&y==y1)break;int e=err*2;
  if(e>-dy){err-=dy;x+=sx;}if(e<dx){err+=dx;y+=sy;}}
}
void text(int x,int y,const char *s,int c){
 for(;*s;s++,x+=6){unsigned ch=(unsigned char)*s;
  if(ch>=32&&ch<128)for(int j=0;j<7;j++)for(int i=0;i<5;i++)
   if(font[ch-32][j]&(1<<(4-i)))putpixel(x+i,y+j,c);}
}
static int digits(int value,char *buf){
 int n=0;unsigned v=value<0?(unsigned)(-(value+1))+1u:(unsigned)value;
 do{buf[n++]=(char)('0'+v%10);v/=10;}while(v);
 if(value<0)buf[n++]='-';
 for(int i=0;i<n/2;i++){char c=buf[i];buf[i]=buf[n-1-i];buf[n-1-i]=c;}buf[n]=0;return n;
}
void number(int x,int y,int value,int c){char s[12];digits(value,s);text(x,y,s,c);}
static void right_number(int right,int y,int value,int c){char s[12];int n=digits(value,s);text(right-n*6,y,s,c);}
static void large_number(int x,int y,int value,int c){
 char s[12];digits(value,s);
 for(int n=0;s[n];n++)for(int j=0;j<7;j++)for(int i=0;i<5;i++)
  if(font[(unsigned char)s[n]-32][j]&(1<<(4-i)))rect(x+n*12+i*2,y+j*2,2,2,c);
}
static void bar(int x,int y,int w,int value,int c){rect(x,y,w,4,BORDER);rect(x,y,clamp(value,0,100)*w/100,4,c);}
static const uint16_t circle[257]={256,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,254,254,254,254,254,253,253,253,253,253,253,253,253,252,252,252,252,252,252,251,251,251,251,251,250,250,250,250,250,249,249,249,249,248,248,248,248,247,247,247,247,246,246,246,245,245,245,245,244,244,244,243,243,243,242,242,242,241,241,241,240,240,240,239,239,238,238,238,237,237,236,236,236,235,235,234,234,233,233,233,232,232,231,231,230,230,229,229,228,228,227,227,226,226,225,225,224,223,223,222,222,221,221,220,219,219,218,218,217,216,216,215,214,214,213,213,212,211,210,210,209,208,208,207,206,205,205,204,203,202,202,201,200,199,199,198,197,196,195,194,194,193,192,191,190,189,188,187,186,185,184,183,183,182,181,180,179,177,176,175,174,173,172,171,170,169,168,167,165,164,163,162,161,159,158,157,155,154,153,151,150,149,147,146,144,143,142,140,138,137,135,134,132,130,129,127,125,123,122,120,118,116,114,112,110,108,106,103,101,99,96,94,91,89,86,83,80,77,74,70,67,63,59,55,50,45,39,31,22,0};
static void ellipse(int x,int y,int rx,int ry,int c){
 if(rx<1||ry<1)return;
 for(int j=-ry;j<=ry;j++){int a=j<0?-j:j;int width=rx*circle[a*256/ry]/256;span(x-width,y+j,width*2+1,c);}
}
/* The articulated flybody model is rasterized offline with a depth buffer.
   Sprites stay in ROM; row spans avoid a decompression buffer in EWRAM. */
#include "body_frames.h"
typedef struct{int x,y,z;} Vec;
typedef struct{int x,y,d;} Point;
static Point project(Vec v){
 if(fly.zoom==2)return (Point){88+v.x,78+v.z,300+v.z};
 int depth=v.z+330,scale=fly.zoom?400:350;depth=depth<80?80:depth;
 int camera_y=90+fly.height*scale/330;
 return (Point){88+v.x*scale/depth,camera_y+(v.z*210-v.y*scale)/depth,depth};
}
static void flysprite(void){
 const int first[6]={0,4,12,22,28,36},counts[6]={4,8,10,6,8,2};
 int state=clamp(fly.state,0,5),phase;
 if(state==WALK){phase=(int)(fly.distance>>8)&7;if(fly.speed<0)phase=(-phase)&7;}
 else if(state==FLIGHT)phase=fly.height<=8?8:fly.height<=20?9:(int)fly.ticks&7;
 else phase=(int)(fly.ticks/(state==GROOM?2:state==FEED?3:state==REST?30:12))%counts[state];
 int direction=((fly.heading+2)&255)>>2;
 int index=(fly.zoom*BODY_POSES+first[state]+phase)*BODY_DIRECTIONS+direction;
 const uint32_t *offsets=(const uint32_t*)(const void*)(fly_sprites+16);
 const uint8_t *data=fly_sprites+offsets[index];
 Point ground=project((Vec){0,fly.height,0});
 int x0=ground.x-data[0],y0=ground.y-data[1],first_y=data[2],rows=data[3];data+=4;
 for(int row=0;row<rows;row++){
  int y=y0+first_y+row,n=*data++;
  for(int i=0;i<n;i++){
   int x=x0+*data++,length=*data++;
   if(y>=clip_top&&y<clip_bottom){
    int lo=clamp(clip_left-x,0,length),hi=clamp(clip_right-x,0,length);
    for(int k=lo;k<hi;k++)pixels[y*W+x+k]=data[k];
   }
   data+=length;
  }
 }
}
static Point world(int x,int y,int z){return project((Vec){x-fly.x/256,y,z-fly.z/256});}
static void worldline(int x,int z,int x2,int z2,int c){Point a=world(x,0,z),b=world(x2,0,z2);line(a.x,a.y,b.x,b.y,c);}
static void dish_object(int i,int sugar){
 const Object *o=sugar?&food[i]:&obstacles[i];Point p=world(o->x,sugar?0:8,o->z);
 int r=clamp(o->r*(fly.zoom==2?330:350)/p.d,3,28),h=fly.zoom==2?r:r/2;
 ellipse(p.x+2,p.y+3,r+2,h,178);
 if(sugar){ellipse(p.x,p.y,r,h,192+8);ellipse(p.x-1,p.y-2,r-2,clamp(h-1,1,30),192+14);
  putpixel(p.x-2,p.y-3,WHITE);putpixel(p.x+2,p.y,192+6);
  if(i==fly.nearest){line(p.x-r-3,p.y-2,p.x-r-3,p.y+2,BROWN);line(p.x+r+3,p.y-2,p.x+r+3,p.y+2,BROWN);}
 }else{ellipse(p.x,p.y,r,h,80+6);ellipse(p.x-2,p.y-3,r-2,clamp(h-2,1,30),80+12);line(p.x-r/2,p.y-h/2,p.x+r/3,p.y-h/2,80+15);}
}
static __attribute__((noinline)) void specimen_scene(void){
 clip(0,18,178,118);
 for(int y=18;y<136;y++)span(0,y,178,160+clamp((y-18)/12,0,15));
 for(int i=-160;i<=160;i+=64){worldline(i,-160,i,160,177);worldline(-160,i,160,i,177);}
 worldline(-160,-160,160,-160,181);worldline(-160,160,160,160,181);
 worldline(-160,-160,-160,160,181);worldline(160,-160,160,160,181);
 for(int i=0;i<FOOD_COUNT;i++)dish_object(i,1);
 for(int pass=0;pass<2;pass++){
  /* Smaller world Z is farther from the camera. Draw distant rocks first;
     the previous reversed comparison painted them over the fly's head. */
  for(int i=0;i<OBSTACLE_COUNT;i++)if((obstacles[i].z>fly.z/256)==(pass==1))dish_object(i,0);
  if(pass==0){Point p=project((Vec){0,0,0});ellipse(p.x+3,p.y+2,fly.zoom==1?30:25,7,176);ellipse(p.x,p.y,18,4,178);flysprite();}
 }
 clip(0,0,W,H);
}
static void minimap(void){
 rect(187,91,44,40,BG);rect(188,92,42,38,CARD);
 for(int i=0;i<FOOD_COUNT;i++)rect(209+food[i].x/9-1,111+food[i].z/9-1,3,3,GOLD);
 for(int i=0;i<OBSTACLE_COUNT;i++)rect(209+obstacles[i].x/9-1,111+obstacles[i].z/9-1,3,3,DIM);
 int x=209+fly.x/(256*9),y=111+fly.z/(256*9);rect(x-1,y-1,3,3,WHITE);line(x,y,x+isin(fly.heading)/70,y+icos(fly.heading)/70,MINT);
}
static UI_PAGE void arena(void){
 specimen_scene();rect(178,18,62,118,PANEL);rect(178,18,1,118,BORDER);
 text(185,25,"ENERGY",MUTED);right_number(233,37,fly.energy/10,INK);text(233,37,"%",MUTED);bar(185,48,48,fly.energy/10,MINT);
 text(185,59,"HUNGER",MUTED);right_number(233,71,fly.hunger/10,INK);text(233,71,"%",MUTED);bar(185,82,48,fly.hunger/10,GOLD);
 minimap();
}
static PET_PAGE void pet_screen(void){
 clip(0,18,178,118);
 for(int y=18;y<136;y++)span(0,y,178,160+clamp((y-18)/12,0,15));
 for(int i=-96;i<=96;i+=48){worldline(i,-96,i,96,177);worldline(-96,i,96,i,177);}
 worldline(-96,-96,96,-96,181);worldline(-96,96,96,96,181);
 worldline(-96,-96,-96,96,181);worldline(96,-96,96,96,181);
 Point shadow=project((Vec){0,0,0});ellipse(shadow.x,shadow.y+2,22,5,178);
 if(fly.pet.food_timer){
  Point bowl=world(fly.x/256+isin(fly.heading)*22/256,0,fly.z/256+icos(fly.heading)*22/256);
  ellipse(bowl.x,bowl.y,10,5,192+8);ellipse(bowl.x,bowl.y-1,7,3,192+14);
 }
 flysprite();
 if(fly.pet.fear_timer){rect(0,18,178,1,CORAL);rect(0,135,178,1,CORAL);rect(0,18,1,118,CORAL);rect(177,18,1,118,CORAL);}
 clip(0,0,W,H);rect(178,18,62,118,PANEL);rect(178,18,1,118,BORDER);
 const char *labels[7]={"HEALTH","FOOD","WATER","ENERGY","CLEAN","STRESS","BOND"};
 const int values[7]={fly.pet.health,1000-fly.pet.hunger,fly.pet.water,fly.pet.energy,fly.pet.clean,fly.pet.stress,fly.pet.bond};
 for(int i=0;i<7;i++){int y=22+i*16;int c=i==5?CORAL:values[i]<250?GOLD:MINT;
  text(185,y,labels[i],MUTED);bar(185,y+10,26,values[i]/10,c);right_number(234,y+9,values[i]/10,c);
 }
 if(fly.pet.dead){rect(11,28,152,25,PANEL);text(20,32,"LIFE ENDED",CORAL);text(20,43,"HOLD A+B: NEW FLY",MUTED);}
 if(fly.pet.menu){
  rect(8,25,162,104,PANEL);text(14,30,"CARE / A TO APPLY",MINT);
  for(int i=0;i<CARE_COUNT;i++){int x=14+(i%2)*78,y=47+(i/2)*19;
   rect(x,y,72,15,i==fly.pet.action?TEAL:CARD);text(x+5,y+4,care_names[i],i==fly.pet.action?WHITE:MUTED);
  }
 }
}
static Point nodepoint(int i){
 /* Ordered by role, not anatomical soma coordinates. */
 int group=neuron_group[i],ordinal=0;for(int j=0;j<i;j++)if(neuron_group[j]==group)ordinal++;
 if(group==0)return (Point){12+(ordinal%2)*12,44+(ordinal/2)*8,0};
 if(group==2)return (Point){168,76+ordinal*12,0};
 return (Point){48+(ordinal%12)*10,44+(ordinal/12)*9,0};
}
static UI_PAGE void brain(void){
 rect(0,18,178,118,BG);rect(178,18,62,118,PANEL);rect(178,18,1,118,BORDER);
 text(7,25,"SENSE",MUTED);text(57,25,"NETWORK",MUTED);text(153,25,"OUT",GOLD);
 Point chosen=nodepoint(fly.selected);unsigned incoming=0,outgoing=0;
 /* Only the selected neuron's actual neighbours; activity is shown by dots. */
 for(unsigned e=0;e<edge_count;e++){
  int a=edges[e].pre,b=edges[e].post;
  incoming+=b==fly.selected;outgoing+=a==fly.selected;
  if(fly.show_links&&(a==fly.selected||b==fly.selected)){
   Point p=nodepoint(a),q=nodepoint(b);
   line(p.x,p.y,q.x,q.y,fly.spike[a]?(edges[e].weight>0?TEAL:RED):BORDER);
  }
 }
 for(int i=0;i<NEURONS;i++){Point p=nodepoint(i);
  int c=fly.glow[i]>160?WHITE:fly.glow[i]>0?MINT:neuron_group[i]==0?TEAL:neuron_group[i]==2?GOLD:DIM;
  rect(p.x-1,p.y-1,3,3,c);
 }
 rect(chosen.x-3,chosen.y-4,7,1,GOLD);rect(chosen.x-3,chosen.y+4,7,1,GOLD);
 rect(chosen.x-4,chosen.y-3,1,7,GOLD);rect(chosen.x+4,chosen.y-3,1,7,GOLD);
 text(185,25,"NODE",MUTED);number(185,37,fly.selected,GOLD);
 text(185,50,neuron_group[fly.selected]==0?"SENSE":neuron_group[fly.selected]==2?"MOTOR":"RELAY",INK);
 text(185,65,"VOLT",MUTED);right_number(234,77,fly.voltage[fly.selected],INK);
 text(185,94,"IN",MUTED);right_number(234,94,incoming,ICE);
 text(185,105,"OUT",MUTED);right_number(234,105,outgoing,MINT);
 text(185,122,fly.show_links?"LINKS ON":"LINKS OFF",MUTED);
 text(7,128,"128 N / 2048 LINKS",MUTED);
}
static void stat_row(int x,int y,const char *label,int value,int c){text(x,y,label,MUTED);right_number(x+104,y,value,c);}
static UI_PAGE void stats(void){
 rect(6,24,112,42,CARD);rect(122,24,112,42,CARD);
 if(fly.stats_detail){
  text(12,29,"ENERGY",MUTED);large_number(12,43,fly.energy/10,MINT);text(49,50,"%",MUTED);
  text(128,29,"HUNGER",MUTED);large_number(128,43,fly.hunger/10,GOLD);text(165,50,"%",MUTED);
  for(int i=0;i<6;i++){int y=74+i*10;text(8,y,state_names[i],MUTED);right_number(69,y,fly.state_ticks[i]/30,INK);text(74,y,"S",DIM);bar(88,y+1,25,fly.ticks?fly.state_ticks[i]*100/fly.ticks:0,i==fly.state?MINT:TEAL);}
  stat_row(126,74,"ODOR",fly.odor,MINT);stat_row(126,87,"HEIGHT",fly.height,ICE);
  stat_row(126,100,"MN9",fly.motor,GOLD);stat_row(126,113,"SUGAR",fly.stimulus,GOLD);
 }else{
  text(12,29,"FRAME RATE",MUTED);large_number(12,43,fly.fps,INK);text(49,50,"FPS",MUTED);text(77,42,"CPU",DIM);number(77,53,fly.cpu,MINT);
  text(128,29,"SPIKES / S",MUTED);large_number(128,43,fly.rate,MINT);
  stat_row(8,74,"TIME S",fly.ticks/30,INK);stat_row(126,74,"ACTIVE",fly.active,MINT);
  stat_row(8,85,"SPEED",(fly.speed<0?-fly.speed:fly.speed)*30/256,INK);stat_row(126,85,"FEED",fly.pet.mode?fly.pet.feeds:fly.meals,GOLD);
  stat_row(8,96,"PATH",fly.distance/256,INK);stat_row(126,96,"HITS",fly.pet.mode?fly.pet.hits:fly.collisions,CORAL);
  text(8,110,"SPIKES/TICK",MUTED);text(180,110,"LAST 4S",DIM);
  for(int y=121;y<=133;y+=6)line(8,y,231,y,BORDER);
  for(int i=1;i<HISTORY;i++){int a=fly.history[(fly.history_pos+i-1)%HISTORY],b=fly.history[(fly.history_pos+i)%HISTORY];line(8+(i-1)*223/119,133-clamp(a,0,128)*15/128,8+i*223/119,133-clamp(b,0,128)*15/128,MINT);}
 }
}
static UI_PAGE void help(void){
 text(8,26,"CONTROLS",MINT);
 const char *keys[7]={"DPAD","A","B","A+B","SELECT","START","L / R"};
 const char *actions[7]={"WALK / TURN","GROOM","FLY / LAND","3D / ZOOM / 2D","AUTO / MANUAL","PAUSE","CHANGE SCREEN"};
 for(int i=0;i<7;i++){int y=39+i*10;text(8,y,keys[i],GOLD);text(66,y,actions[i],INK);}
 text(8,115,"PET: DPAD PICK A USE B MENU",MINT);text(8,127,"PET: HOLD A+B 3S NEW FLY",MUTED);
}
static void chrome(void){
 rect(0,0,W,18,PANEL);rect(0,17,W,1,BORDER);
 text(7,5,"FLYWIRE",MINT);rect(57,5,1,7,BORDER);
 const char *title[5]={"3D ARENA","CONNECTOME","TELEMETRY","GUIDE","PET"};
 text(65,5,fly.page==0?(fly.zoom==2?"TOP VIEW":fly.zoom==1?"CLOSE 3D":title[0]):title[fly.page],INK);
 if(fly.page==4){text(91,5,pet_status(),fly.pet.dead?CORAL:MINT);right_number(180,5,fly.pet.age/1800,MUTED);text(182,5,"M",MUTED);}
 rect(200,4,33,10,fly.paused?RED:CARD);text(203,6,fly.paused?"PAUSE":fly.pet.mode?"PET":fly.auto_mode?"AUTO":"MAN",fly.paused?CORAL:MINT);
 rect(0,136,W,11,PANEL);
 if(fly.page==0){text(7,138,state_names[fly.state],MINT);text(50,138,"A GROOM B FLY A+B VIEW",MUTED);}
 else if(fly.page==1){extern const char root_labels[NEURONS][19];text(7,138,root_labels[fly.selected],INK);text(122,138,"DPAD NODE A LINKS",MUTED);}
 else if(fly.page==2)text(7,138,"A RESET  B BODY  START PAUSE",MUTED);
 else if(fly.page==4){
  if(fly.pet.reset_hold>0){text(7,138,"NEW FLY: HOLD A+B",GOLD);number(122,138,fly.pet.reset_hold*100/90,GOLD);}
  else if(fly.pet.feedback_timer){
   const char *msg[11]={"","FOOD OFFERED","WATER GIVEN","GENTLE TOUCH","PLAY TIME","ALL CLEAN","SLEEP TOGGLED","RECOVERING","OUCH!","TOO TIRED","MEDIC COOLDOWN"};
   text(7,138,msg[fly.pet.feedback],fly.pet.feedback==8?CORAL:MINT);text(122,138,"B CARE MENU",MUTED);
  }else{text(7,138,care_names[fly.pet.action],GOLD);text(50,138,"A USE B MENU DPAD PICK",MUTED);}
 }else text(7,138,"V783 SUBSET / DEMO DYNAMICS",MUTED);
 rect(0,147,W,13,BG);text(5,151,"<",MUTED);text(228,151,">",MUTED);
 const char *tabs[5]={"LAB","BRAIN","DATA","HELP","PET"};
 for(int i=0;i<5;i++){int x=18+i*42;if(i==fly.page){rect(x,148,39,11,TEAL);rect(x,148,39,1,MINT);}text(x+4,151,tabs[i],i==fly.page?WHITE:MUTED);}
}
void render(void){
 clip(0,0,W,H);
 if(fly.page!=0)rect(0,18,W,118,BG);
 if(fly.page==0)arena();else if(fly.page==1)brain();else if(fly.page==2)stats();else if(fly.page==4)pet_screen();else help();
 chrome();
}
