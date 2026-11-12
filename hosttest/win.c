#define _POSIX_C_SOURCE 199309L
#include "synth.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
static Synth S;
static uint64_t ns(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return t.tv_sec*1000000000ull+t.tv_nsec; }
#define NB (44100*30/128)
static double best[NB]; static double padt[NB];
static uint32_t c32(void){ return (uint32_t)ns(); }
int main(int argc,char**argv){
  uint32_t seed=atoi(argv[1]); int style=atoi(argv[2]); int runs=3; int W=50;
  for(int b=0;b<NB;b++) best[b]=1e18;
  for(int r=0;r<runs;r++){
    memset(&S,0,sizeof S);
    synth_init(&S,seed); synth_set_style(&S,style); synth_randomize(&S,seed,0); engine_build(&S); S.rebuild_pending=0; S.duck=S.duck_tgt=1; S.build_defer=1; S.clock=c32;
    static float L[128],R[128];
    for(int b=0;b<NB;b++){
      if(b%3000==1500) synth_roll_sections(&S,seed+b,1.5f,0xFF);
      if(b%3000==2500) synth_mutate(&S,.35f);
      if(S.build_req) synth_build_now(&S);
      uint32_t p0=S.prof[PROF_PADWORK];
      uint64_t t0=ns(); synth_process(&S,L,R,128); double d=(double)(ns()-t0);
      if(d<best[b]){best[b]=d; padt[b]=(double)(S.prof[PROF_PADWORK]-p0);}
    }
  }
  double tot=0, ptot=0; int n=0; for(int b=20;b<NB;b++){tot+=best[b]; ptot+=padt[b]; n++;}
  double mean=tot/n, worst=0, wpad=0; int wb=0;
  for(int b=20;b+W<NB;b++){ double s=0,p=0; for(int k=0;k<W;k++){s+=best[b+k];p+=padt[b+k];} s/=W; p/=W; if(s>worst){worst=s;wb=b;wpad=p;} }
  printf("%u %d mean_ns_per_sample %.1f worstwin %.1f (x%.2f) at %d pad_share_mean %.2f pad_share_worst %.2f\n",seed,style,mean/128,worst/128,worst/mean,wb,ptot/tot,wpad/worst);
  return 0;}
