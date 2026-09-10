#ifndef __BODY_H__
#define __BODY_H__

void rehash_capacity();
protected void die();
public varargs int move_living(mixed dest, string dir, string depart_message, string arrive_message);
public int query_log_level();
public int is_able();
protected string *query_body_slots();
public string *query_weapon_slots();
public void set_su_body(object source);
public object query_su_body();
public void clear_su_body();
protected void display_health_bar();
private void display_plain_health_bar();
private void display_auto_health_bar();
private int *get_vital_ratio(float curr, float max, int constraint);
protected varargs void display_enemy_health_bar(object who);

#endif // __BODY_H__
