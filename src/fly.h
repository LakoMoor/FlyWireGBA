#ifndef FLY_H
#define FLY_H
#include <stdint.h>
#define W 240
#define H 160
#define KEY_A 1
#define KEY_B 2
#define KEY_SELECT 4
#define KEY_START 8
#define KEY_RIGHT 16
#define KEY_LEFT 32
#define KEY_UP 64
#define KEY_DOWN 128
#define KEY_R 256
#define KEY_L 512
#define NEURONS 128
#define MAX_EDGES 2048
#define HISTORY 120
#define FOOD_COUNT 4
#define OBSTACLE_COUNT 3
#define STEP_HZ 30

typedef struct { uint8_t pre, post; int16_t weight; } Edge;
typedef struct { int x,z,r; } Object;
enum {CARE_FEED,CARE_WATER,CARE_PET,CARE_PLAY,CARE_CLEAN,CARE_SLEEP,CARE_MEDIC,CARE_HIT,CARE_COUNT};
typedef struct {
 int mode,health,hunger,energy,water,clean,stress,bond;
 int sleeping,dead,action,menu,food_timer,fear_timer,play_timer,cooldown,medicine;
 int feedback,feedback_timer,reset_hold,dirty;
 uint32_t age,feeds,hits;
} Pet;
typedef struct {
 int x,z,heading,height,speed,energy,hunger,odor,touch,food_dist,nearest;
 int state,auto_mode,paused,page,selected,zoom; uint32_t ticks,distance,meals,collisions;
 uint16_t last_keys; uint32_t rng,spikes_total;
 int16_t voltage[NEURONS],input[NEURONS]; uint8_t spike[NEURONS],glow[NEURONS],refractory[NEURONS];
 uint16_t spike_count,rate,active,history[HISTORY],history_pos,second_spikes;
 uint32_t state_ticks[6]; int motor,stimulus,show_links;
 int fps,cpu,render_cycles,frame_cycles,stats_detail;
 Pet pet;
} Fly;
enum {IDLE,WALK,FLIGHT,FEED,GROOM,REST};
extern Fly fly;
extern uint8_t pixels[W*H];
extern const uint16_t palette[256];
extern const Object food[FOOD_COUNT],obstacles[OBSTACLE_COUNT];
extern const Edge edges[];
extern const uint64_t root_ids[];
extern const unsigned edge_count;
extern const uint8_t neuron_group[];
extern const char *const state_names[6];
void sim_reset(void);
void sim_step(uint16_t keys);
void render(void);
int isin(int angle);
int icos(int angle);
int clamp(int v,int lo,int hi);
void putpixel(int x,int y,int c);
void line(int x0,int y0,int x1,int y1,int c);
void rect(int x,int y,int w,int h,int c);
void text(int x,int y,const char *str,int c);
void number(int x,int y,int value,int c);
void neural_step(void);
extern const char *const care_names[CARE_COUNT];
void pet_reset(void);
void pet_enter(void);
void pet_leave(void);
void pet_step(uint16_t keys,uint16_t hit);
const char *pet_status(void);
#define PET_SAVE_SIZE 128
void pet_encode(uint8_t out[PET_SAVE_SIZE]);
int pet_decode(const uint8_t in[PET_SAVE_SIZE]);
#endif
