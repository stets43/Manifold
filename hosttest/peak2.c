#define _POSIX_C_SOURCE 199309L
#include "synth.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
static Synth S;
static uint32_t clk(void){ struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t); return (uint32_t)(t.tv_sec*1000000000ull+t.tv_nsec); }
#define NB (44100*40/128)
static uint32_t best[NB][7];
int main(int argc,char**argv){
  uint32_t seed=argc>1?atoi(argv[1]):501; int style=argc>2?atoi(argv[2]):STYLE_COSMIC; int runs=argc>3?atoi(argv[3]):7;
  memset(best,0xff,sizeof best);
  for(int r=0;r<runs;r++){
    memset(&S,0,sizeof S);
    synth_init(&S,seed); synth_set_style(&S,style); synth_randomize(&S,seed,0); engine_build(&S); S.rebuild_pending=0; S.duck=S.duck_tgt=1;
    S.clock=clk; S.build_defer=1;
    static float L[128],R[128]; uint32_t last[8];
    for(int b=0;b<NB;b++){
      if(b%3000==1500) synth_roll_sections(&S,seed+b,1.5f,0xFF);
      if(b%3000==2500) synth_mutate(&S,.35f);
      if(S.build_req) synth_build_now(&S);
      memcpy(last,S.prof,sizeof last);
      uint32_t t0=clk(); synth_process(&S,L,R,128); uint32_t d=clk()-t0;
      uint32_t v[7]={d,S.prof[PROF_CTRL]-last[PROF_CTRL],S.prof[PROF_SEQ]-last[PROF_SEQ],S.prof[PROF_PADWORK]-last[PROF_PADWORK],S.prof[PROF_BANK]-last[PROF_BANK],S.prof[PROF_FX]-last[PROF_FX],0};
      if(v[0]<best[b][0]) memcpy(best[b],v,sizeof v);
    }
  }
  double sum=0; uint32_t worst=0; int wb=0; int n=0;
  static uint32_t sorted[NB];
  for(int b=10;b<NB;b++){ sum+=best[b][0]; sorted[n++]=best[b][0]; if(best[b][0]>worst){worst=best[b][0];wb=b;} }
  for(int i=0;i<n;i++) for(int j=i+1;j<n && j<i+1;j++);
  /* percentiles by counting */
  uint32_t p999=0; { int cnt=0; for(uint32_t th=worst; th>0; th=th*97/100){ cnt=0; for(int i=0;i<n;i++) if(sorted[i]>th) cnt++; if(cnt>n/1000){p999=th;break;} } }
  double avg=sum/n;
  printf("seed %u style %d: avg %.0f ns, p99.9 %u (x%.2f), worst %u at %d (x%.2f) ctrl %u seq %u pad %u bank %u fx %u\n",seed,style,avg,p999,p999/avg,worst,wb,worst/avg,best[wb][1],best[wb][2],best[wb][3],best[wb][4],best[wb][5]);
  return 0;}
