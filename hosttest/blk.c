#include "synth.h"
#include <valgrind/callgrind.h>
#include <stdlib.h>
static Synth S;
int main(int argc,char**argv){
  uint32_t seed=atoi(argv[1]); int style=atoi(argv[2]); int target=atoi(argv[3]); int avg=argc>4;
  synth_init(&S,seed); synth_set_style(&S,style); synth_randomize(&S,seed,0); engine_build(&S); S.rebuild_pending=0; S.duck=S.duck_tgt=1; S.build_defer=1;
  static float L[128],R[128];
  for(int b=0;b<=target+(avg?400:0);b++){
    if(b%3000==1500) synth_roll_sections(&S,seed+b,1.5f,0xFF);
    if(b%3000==2500) synth_mutate(&S,.35f);
    if(S.build_req) synth_build_now(&S);
    int on = avg ? (b>target && b<=target+400) : b==target;
    if(on) CALLGRIND_TOGGLE_COLLECT;
    synth_process(&S,L,R,128);
    if(on) CALLGRIND_TOGGLE_COLLECT;
  }
  return 0;}
