#ifndef __ADVANCEMENT_H__
#define __ADVANCEMENT_H__

int queryXp() ;
float query_tnl() ;
float queryLevel() ;
float query_effective_level() ;
float setLevel(float l) ;
float add_level(float l) ;
float query_level_mod() ;
float set_level_mod(float l) ;
float adjust_level_mod(float l) ;
int adjustXp(int amount) ;
int setXp(int amount) ;
void on_advance(object tp, float l) ;

#endif // __ADVANCEMENT_H__
