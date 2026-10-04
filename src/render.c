#include "fly.h"
#include "font.h"
#ifdef __arm__
#define COLD __attribute__((section(".romcode"),noinline))
#else
#define COLD __attribute__((noinline))
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
/* High colour bit selects a checker pattern for translucent wings. */
static void triangle(int x0,int y0,int x1,int y1,int x2,int y2,int c){
 if(y0>y1){int t=x0;x0=x1;x1=t;t=y0;y0=y1;y1=t;}
 if(y1>y2){int t=x1;x1=x2;x2=t;t=y1;y1=y2;y2=t;}
 if(y0>y1){int t=x0;x0=x1;x1=t;t=y0;y0=y1;y1=t;}
 if(y0==y2)return;
 int slope=(x2-x0)*256/(y2-y0),s0=y1>y0?(x1-x0)*256/(y1-y0):0,s1=y2>y1?(x2-x1)*256/(y2-y1):0;
 int start=clamp(y0,clip_top,clip_bottom),end=clamp(y2,clip_top,clip_bottom);
 for(int y=start;y<end;y++){
  int a=x0*256+(y-y0)*slope,b=y<y1?x0*256+(y-y0)*s0:x1*256+(y-y1)*s1;
  if(a>b){int t=a;a=b;b=t;}
  int lo=clamp(a/256,clip_left,clip_right),hi=clamp(b/256,clip_left-1,clip_right-1);
  if(c&256){for(int x=lo+((lo+y)&1);x<=hi;x+=2)putpixel(x,y,c&255);}
  else span(lo,y,hi-lo+1,c);
 }
}

typedef struct{int x,y,z;} Vec;
typedef struct{int x,y,d;} Point;
typedef struct{Point a,b,c;int depth,color;} Face;
static Face faces[384];static int face_count;
static Point project(Vec v){
 if(fly.zoom==2)return (Point){88+v.x,83+v.z-v.y/5,300+v.z};
 int depth=v.z+330,scale=fly.zoom?400:350;depth=depth<80?80:depth;
 int camera_y=fly.zoom?118+fly.height*3/2:108+fly.height*4/5;
 return (Point){88+v.x*scale/depth,camera_y+(v.z*210-v.y*scale)/depth,depth};
}
static Vec local(int x,int y,int z){
 /* Enlarged specimen geometry, separate from the navigation body radius. */
 x=x*3/2;y=y*3/2;z=z*3/2;
 return (Vec){(x*icos(fly.heading)+z*isin(fly.heading))/256,y+fly.height,(z*icos(fly.heading)-x*isin(fly.heading))/256};
}
static void meshface(Point a,Point b,Point c,int color){
 if(face_count>=384||(b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x)>=0)return;
 faces[face_count++]=(Face){a,b,c,a.d+b.d+c.d,color};
}
static void sphere(int cx,int cy,int cz,int rx,int ry,int rz,int base){
 Point v[5][8];const int lat[5]={-256,-181,0,181,256},rad[5]={0,181,256,181,0};
 for(int r=0;r<5;r++)for(int s=0;s<8;s++)
  v[r][s]=project(local(cx+rx*icos(s*32)*rad[r]/65536,cy+ry*lat[r]/256,cz+rz*isin(s*32)*rad[r]/65536));
 for(int r=0;r<4;r++)for(int s=0;s<8;s++){
  int t=(s+1)%8,shade=clamp(5+r*2+(icos(s*32+fly.heading+40)>>6),0,15);
  meshface(v[r][s],v[r][t],v[r+1][s],base+shade);
  meshface(v[r][t],v[r+1][t],v[r+1][s],base+shade);
 }
}
static void abdomen(void){
 const int z[7]={-33,-29,-23,-17,-11,-5,0},radius[7]={0,140,225,256,240,180,0};
 Point p[7][8];
 for(int r=0;r<7;r++)for(int s=0;s<8;s++)
  p[r][s]=project(local(9*icos(s*32)*radius[r]/65536,12+7*isin(s*32)*radius[r]/65536,z[r]));
 for(int r=0;r<6;r++)for(int s=0;s<8;s++){
  int t=(s+1)%8,shade=clamp(9+isin(s*32)/64,0,15),base=(r&1)?128:144;
  meshface(p[r][s],p[r][t],p[r+1][s],base+shade);
  meshface(p[r][t],p[r+1][t],p[r+1][s],base+shade);
 }
}
static void limb(int x,int y,int z,int x2,int y2,int z2,int c){
 Point a=project(local(x,y,z)),b=project(local(x2,y2,z2));line(a.x,a.y,b.x,b.y,c);
}
static void wings(void){
 for(int side=-1;side<=1;side+=2){
  int airborne=fly.state==FLIGHT;
  int flap=airborne?isin((int)fly.ticks*83)/14:0;
  const int rest_x[6]={4,10,14,13,8,4},open_x[6]={4,20,31,29,18,4};
  const int z[6]={5,-2,-16,-29,-28,-5};Point p[6];
  for(int i=0;i<6;i++)p[i]=project(local(side*(airborne?open_x[i]:rest_x[i]),24+flap*(i>0&&i<5),z[i]));
  for(int i=1;i<5;i++)triangle(p[0].x,p[0].y,p[i].x,p[i].y,p[i+1].x,p[i+1].y,256+61);
  for(int i=0;i<6;i++)line(p[i].x,p[i].y,p[(i+1)%6].x,p[(i+1)%6].y,BLUE);
  line(p[0].x,p[0].y,p[3].x,p[3].y,57);line(p[1].x,p[1].y,p[4].x,p[4].y,57);
  line(p[2].x,p[2].y,p[4].x,p[4].y,57);
 }
}
static void flylegs(void){
 for(int side=-1;side<=1;side+=2)for(int leg=0;leg<3;leg++){
  int phase=(int)fly.ticks*18+(leg==1?128:0)+(side>0?128:0);
  int stride=fly.state==WALK?isin(phase)/48:0;
  int lift=fly.state==WALK?clamp(isin(phase)/64,0,4):0;
  int root_z=13-leg*11,knee_z=root_z+(1-leg)*7,foot_z=knee_z+(1-leg)*6+stride;
  int knee_x=18,foot_x=26,knee_y=6+lift,foot_y=lift;
  if(fly.state==FLIGHT){knee_x=12;foot_x=16;knee_y=8;foot_y=5;foot_z-=8;}
  if(fly.state==GROOM&&leg==0){knee_x=12;foot_x=6;knee_z=24;foot_z=29;foot_y=20+isin((int)fly.ticks*22)/80;knee_y=18;}
  limb(side*7,15,root_z,side*knee_x,knee_y,knee_z,144+10);
  limb(side*knee_x,knee_y,knee_z,side*foot_x,foot_y,foot_z,144+8);
  limb(side*foot_x,foot_y,foot_z,side*(foot_x+3),foot_y,foot_z-2,144+5);
 }
}
static void flymesh(void){
 face_count=0;int bob=fly.state==WALK?isin((int)fly.ticks*20)/180:0;
 abdomen();sphere(0,17+bob,4,10,9,12,16);sphere(0,20+bob,21,8,7,8,16);
 sphere(-7,21+bob,24,5,6,5,32);sphere(7,21+bob,24,5,6,5,32);
 for(int i=1;i<face_count;i++){Face f=faces[i];int j=i;
  while(j>0&&faces[j-1].depth<f.depth){faces[j]=faces[j-1];j--;}faces[j]=f;}
 flylegs();
 for(int i=0;i<face_count;i++){Face *f=&faces[i];triangle(f->a.x,f->a.y,f->b.x,f->b.y,f->c.x,f->c.y,f->color);}
 wings();
 for(int side=-1;side<=1;side+=2){
  if(side*isin(fly.heading)>-48){Point e=project(local(side*8,24+bob,27));ellipse(e.x,e.y,2,1,220);putpixel(e.x,e.y,WHITE);}
  limb(side*3,24,26,side*5,27,31,144+6);limb(side*5,27,31,side*7,27,34,144+6);
 }
 if(fly.state==FEED){int pulse=fly.motor?2:0;
  limb(0,15,28,0,4,34+pulse,32+10);Point tip=project(local(0,4,34+pulse));ellipse(tip.x,tip.y,2,1,32+12);}
}
static void flatoval(int cx,int cy,int cz,int rx,int rz,int base){
 Point center=project(local(cx,cy,cz)),p[12];
 for(int i=0;i<12;i++)p[i]=project(local(cx+rx*icos(i*256/12)/256,cy,cz+rz*isin(i*256/12)/256));
 for(int i=0;i<12;i++){int j=(i+1)%12;
  triangle(center.x,center.y,p[i].x,p[i].y,p[j].x,p[j].y,base+clamp(10+icos(i*256/12+fly.heading)/80,0,15));
  line(p[i].x,p[i].y,p[j].x,p[j].y,base+5);
 }
}
static void flatfly(void){
 flylegs();
 const int z[7]={-33,-29,-23,-17,-11,-5,0},r[7]={0,5,8,9,8,6,0};
 for(int i=0;i<6;i++){
  Point a=project(local(-r[i],17,z[i])),b=project(local(r[i],17,z[i]));
  Point c=project(local(-r[i+1],17,z[i+1])),d=project(local(r[i+1],17,z[i+1]));
  int color=(i&1)?128+13:144+7;
  triangle(a.x,a.y,b.x,b.y,c.x,c.y,color);triangle(b.x,b.y,d.x,d.y,c.x,c.y,color);
  line(a.x,a.y,c.x,c.y,144+4);line(b.x,b.y,d.x,d.y,144+4);
 }
 flatoval(0,24,4,10,12,16);flatoval(0,25,21,8,8,16);
 for(int side=-1;side<=1;side+=2){Point e=project(local(side*7,28,24));ellipse(e.x,e.y,6,7,32+7);ellipse(e.x-1,e.y-1,5,6,32+13);ellipse(e.x-2,e.y-3,2,1,220);putpixel(e.x-2,e.y-3,WHITE);}
 wings();
 for(int side=-1;side<=1;side+=2){limb(side*3,26,27,side*7,27,34,144+5);}
 if(fly.state==FEED)limb(0,24,28,0,24,35+(fly.motor?2:0),32+10);
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
  for(int i=0;i<OBSTACLE_COUNT;i++)if((obstacles[i].z>fly.z/256)==(pass==0))dish_object(i,0);
  if(pass==0){Point p=project((Vec){0,0,0});ellipse(p.x+3,p.y+2,fly.zoom==1?30:25,7,176);ellipse(p.x,p.y,18,4,178);if(fly.zoom==2)flatfly();else flymesh();}
 }
 clip(0,0,W,H);
 rect(7,24,47,12,PANEL);text(11,27,state_names[fly.state],MINT);
 rect(116,24,55,12,165);text(120,27,fly.zoom==2?"TOP VIEW":fly.zoom==1?"CLOSE UP":"3D VIEW",144+4);
}
static void minimap(void){
 rect(187,91,44,40,BG);rect(188,92,42,38,CARD);
 for(int i=0;i<FOOD_COUNT;i++)rect(209+food[i].x/9-1,111+food[i].z/9-1,3,3,GOLD);
 for(int i=0;i<OBSTACLE_COUNT;i++)rect(209+obstacles[i].x/9-1,111+obstacles[i].z/9-1,3,3,DIM);
 int x=209+fly.x/(256*9),y=111+fly.z/(256*9);rect(x-1,y-1,3,3,WHITE);line(x,y,x+isin(fly.heading)/70,y+icos(fly.heading)/70,MINT);
}
static COLD void arena(void){
 specimen_scene();rect(178,18,62,118,PANEL);rect(178,18,1,118,BORDER);
 text(185,25,"ENERGY",MUTED);right_number(233,37,fly.energy/10,INK);text(233,37,"%",MUTED);bar(185,48,48,fly.energy/10,MINT);
 text(185,59,"HUNGER",MUTED);right_number(233,71,fly.hunger/10,INK);text(233,71,"%",MUTED);bar(185,82,48,fly.hunger/10,GOLD);
 minimap();
}
static Point nodepoint(int i){
 /* Ordered by role, not anatomical soma coordinates. */
 int group=neuron_group[i],ordinal=0;for(int j=0;j<i;j++)if(neuron_group[j]==group)ordinal++;
 if(group==0)return (Point){12+(ordinal%2)*12,44+(ordinal/2)*8,0};
 if(group==2)return (Point){168,76+ordinal*12,0};
 return (Point){48+(ordinal%12)*10,44+(ordinal/12)*9,0};
}
static COLD void brain(void){
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
static COLD void stats(void){
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
  stat_row(8,85,"SPEED",(fly.speed<0?-fly.speed:fly.speed)*30/256,INK);stat_row(126,85,"FEED",fly.meals,GOLD);
  stat_row(8,96,"PATH",fly.distance/256,INK);stat_row(126,96,"HITS",fly.collisions,CORAL);
  text(8,110,"SPIKES/TICK",MUTED);text(180,110,"LAST 4S",DIM);
  for(int y=121;y<=133;y+=6)line(8,y,231,y,BORDER);
  for(int i=1;i<HISTORY;i++){int a=fly.history[(fly.history_pos+i-1)%HISTORY],b=fly.history[(fly.history_pos+i)%HISTORY];line(8+(i-1)*223/119,133-clamp(a,0,128)*15/128,8+i*223/119,133-clamp(b,0,128)*15/128,MINT);}
 }
}
static COLD void help(void){
 text(8,26,"CONTROLS",MINT);
 const char *keys[7]={"DPAD","A","B","A+B","SELECT","START","L / R"};
 const char *actions[7]={"WALK / TURN","GROOM","FLY / LAND","3D / ZOOM / 2D","AUTO / MANUAL","PAUSE","CHANGE SCREEN"};
 for(int i=0;i<7;i++){int y=42+i*12;text(8,y,keys[i],GOLD);text(66,y,actions[i],INK);}
}
static void chrome(void){
 rect(0,0,W,18,PANEL);rect(0,17,W,1,BORDER);
 text(7,5,"FLYWIRE",MINT);rect(57,5,1,7,BORDER);
 const char *title[4]={"ARENA","CONNECTOME","TELEMETRY","GUIDE"};text(65,5,title[fly.page],INK);
 rect(200,4,33,10,fly.paused?RED:CARD);text(203,6,fly.paused?"PAUSE":fly.auto_mode?"AUTO":"MAN",fly.paused?CORAL:MINT);
 rect(0,136,W,11,PANEL);
 if(fly.page==0)text(7,138,"A GROOM  B FLY  A+B VIEW",MUTED);
 else if(fly.page==1){extern const char root_labels[NEURONS][19];text(7,138,root_labels[fly.selected],INK);text(122,138,"DPAD NODE A LINKS",MUTED);}
 else if(fly.page==2)text(7,138,"A RESET  B BODY  START PAUSE",MUTED);
 else text(7,138,"V783 SUBSET / DEMO DYNAMICS",MUTED);
 rect(0,147,W,13,BG);text(5,151,"<",MUTED);text(228,151,">",MUTED);
 const char *tabs[4]={"LAB","BRAIN","DATA","HELP"};
 for(int i=0;i<4;i++){int x=22+i*48;if(i==fly.page){rect(x,148,44,11,TEAL);rect(x,148,44,1,MINT);}text(x+7,151,tabs[i],i==fly.page?WHITE:MUTED);}
}
void render(void){
 clip(0,0,W,H);
 if(fly.page!=0)rect(0,18,W,118,BG);
 if(fly.page==0)arena();else if(fly.page==1)brain();else if(fly.page==2)stats();else help();
 chrome();
 if(fly.paused&&fly.page==0){rect(48,69,82,21,PANEL);text(60,76,"PAUSED",GOLD);}
}
