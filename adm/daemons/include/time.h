#ifndef __TIME_H__
#define __TIME_H__

private int last_midnight();
public varargs mixed *query_full_date(int t);
public varargs string query_date(int t);
public varargs int query_day(int t);
public varargs string query_full_time(int t);
public varargs int query_hour(int t);
public varargs int query_minute(int t);
public varargs int query_month(int t);
public varargs string query_month_name(int t);
public varargs string query_season(int t);
public varargs int query_second(int t);
public varargs int query_year(int t);
private varargs int since_start(int t);
public varargs string query_time(int t);

#endif // __TIME_H__
