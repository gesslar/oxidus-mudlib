//lockdown_d.c

//Parthenon @ LPUniversity
//15-JUL-06
//Lockdown daemon

//Last edited on July 17th, 2006 by Parthenon

inherit STD_DAEMON;

#define DEV_LOCK_MSG mud_name() + " is currently locked to developers right now.\nPlease try again later."
#define PLAYER_LOCK_MSG mud_name() + " is currently locked to players right now.\nPlease try again later."
#define VIP_LOCK_MSG mud_name() + " is currently locked to all but VIP players.\nPlease try again later."
#define GUEST_LOCK_MSG "We are sorry but the guest account has been deactivated temporarily.\nPlease try again later."
#define PLAYER_CREATION_MSG "We are sorry but new player creation is temporarily disabled.\nPleast try again later."

string *users_banned, *ips_banned, *play_testers;
int guest_lock = 0, player_creation = 0;
int dev_lock = 0, player_lock = 0, vip_lock = 0;
string dev_lock_msg = "", vip_lock_msg = "", player_lock_msg = "";
string guest_lock_msg = "", player_creation_msg = "";

/**************/
/* PROTOTYPES */
/**************/

string *query_banned_users();
mixed add_banned_user(string name);
mixed remove_banned_user(string name);
mixed add_play_tester(string name);
mixed remove_play_tester(string name);
string *query_play_testers();
int remove_all_banned_users();
int remove_all_play_testers();
mixed add_ip_ban(string ip);
mixed remove_ip_ban(string ip);
int remove_all_ip_bans();
string *query_banned_ips();
void toggle_guest_account();
void toggle_new_player_creation();
int query_guest_locked();
int query_creation_locked();
int query_dev_lock();
int query_vip_lock();
void set_dev_lock_msg(string arg);
void set_vip_lock_msg(string arg);
void set_guest_lock_msg(string arg);
void set_player_creation_msg(string arg);
void set_player_lock_msg(string arg);
string query_dev_lock_msg();
string query_vip_lock_msg();
string query_guest_lock_msg();
string query_player_creation_msg();
string query_player_lock_msg();
void unlock_mud();
int is_ip_banned(string ip);
int is_user_banned(string user);

int setup() {
    setPersistent();
}

void post_setup_1() {
    if(!users_banned)
        users_banned = ({});
    if(!ips_banned)
        ips_banned = ({});
    if(!play_testers)
        play_testers = ({});
}

/*******************/
/* QUERY FUNCTIONS */
/*******************/

int is_user_banned(string user) {
    int i;
    string *banned_users;

    banned_users = query_banned_users();

    if(!banned_users || sizeof(banned_users) < 1)
    return 0;

    for(i = 0; i < sizeof(banned_users); i++) {
    if(user == banned_users[i] || regexp(user, banned_users[i]) == 1)
        return 1;
    }

    return 0;
}

int is_ip_banned(string ip) {
    int i;
    string *banned_ips, curr_ip, new_ip, first, second, third, fourth;

    banned_ips = query_banned_ips();

    if(!banned_ips || sizeof(banned_ips) < 1)
    return 0;

    for(i = 0; i < sizeof(banned_ips); i++) {
    curr_ip = banned_ips[i];

    sscanf(curr_ip, "%s.%s.%s.%s", first, second, third, fourth);

    if(first = "*")
        first = "^[0-9]?[0-9]?[0-9]?";
    if(second = "*")
        second = "[0-9]?[0-9]?[0-9]?";
    if(third = "*")
        third = "[0-9]?[0-9]?[0-9]?";
    if(fourth = "*")
        fourth = "[0-9]?[0-9]?[0-9]?$";

    new_ip = "^" + first + "." + second + "." + third + "." + fourth + "$";

    if(ip == curr_ip || regexp(ip, new_ip) == 1)
        return 1;
    }//END FOR

    return 0;
}

int query_player_lock() {
    int tmp = player_lock;
    return tmp;
}

int query_dev_lock() {
    int tmp = dev_lock;
    return tmp;
}

string query_vip_lock_msg() {
    string tmp = vip_lock_msg;

    if(tmp == "")
        return VIP_LOCK_MSG;
    else
        return tmp;
}

string query_dev_lock_msg() {
    string tmp = dev_lock_msg;

    if(tmp == "")
        return DEV_LOCK_MSG;
    else
        return tmp;
}

string query_guest_lock_msg() {
    string tmp = guest_lock_msg;

    if(tmp == "")
        return GUEST_LOCK_MSG;
    else
        return tmp;
}

string query_player_creation_msg() {
    string tmp = player_creation_msg;

    if(tmp == "")
        return PLAYER_CREATION_MSG;
    else
        return tmp;
}

string query_player_lock_msg() {
    string tmp = player_lock_msg;

    if(tmp == "")
        return PLAYER_LOCK_MSG;
    else
        return tmp;
}

int query_vip_lock() {
    int tmp;

    tmp = vip_lock;
    return tmp;
}

int query_creation_locked() {
    int tmp;

    tmp = player_creation;
    return tmp;
}

int query_guest_locked() {
    int tmp;

    tmp = guest_lock;
    return guest_lock;
}

string *query_banned_ips() {
    string *tmp;

    tmp = ips_banned;
    return tmp;
}

string *query_banned_users() {
    string *tmp;

    tmp = users_banned;
    return tmp;
}

string *query_play_testers() {
    string *tmp;

    tmp = play_testers;
    return tmp;
}

/**********************/
/* USER BAN FUNCTIONS */
/**********************/

mixed add_banned_user(string name) {
    if((member_array(name, users_banned) != -1))
        return "That user has already been banned.";

    if(!users_banned)
        users_banned = ({ name });
    else
        users_banned += ({ name });

    saveData();

    return 1;
}

mixed remove_banned_user(string name) {
    if((member_array(name, users_banned) == -1))
        return "That user is not currently banned.";

    users_banned -= ({ name });

    saveData();

    return 1;
}

int remove_all_banned_users() {
    if(users_banned)
        users_banned = ({});

    saveData();

    return 1;
}

/*****************/
/* VIP FUNCTIONS */
/*****************/

mixed add_play_tester(string name) {
    if((member_array(name, play_testers) != -1))
    return "That user has already been banned.";

    if(!play_testers)
        play_testers = ({ name });
    else
        play_testers += ({ name });

    saveData();

    return 1;
}

mixed remove_play_tester(string name) {
    if((member_array(name, play_testers) == -1))
        return "That user is not currently banned.";

    play_testers -= ({ name });

    saveData();

    return 1;
}

int remove_all_play_testers() {
    if(play_testers)
        play_testers = ({});

    saveData();

    return 1;
}

/********************/
/* IP BAN FUNCTIONS */
/********************/

mixed add_ip_ban(string ip) {
    if((member_array(ip, ips_banned) != -1))
        return "That IP address has already been banned.";

    if(!ips_banned)
        ips_banned = ({ ip });
    else
        ips_banned += ({ ip });

    saveData();

    return 1;
}

mixed remove_ip_ban(string ip) {
    if((member_array(ip, ips_banned) == -1))
        return "That IP address is not currently banned.";

    ips_banned -= ({ ip });

    saveData();

    return 1;
}

int remove_all_ip_bans() {
    if(ips_banned)
        ips_banned = ({});

    saveData();

    return 1;
}

/******************/
/* LOCK FUNCTIONS */
/******************/

void unlock_mud() {
    player_lock = 0;
    dev_lock = 0;
    vip_lock = 0;
}

void toggle_guest_account() {
    guest_lock = !guest_lock;
    saveData();
}

void toggle_new_player_creation() {
    player_creation = !player_creation;
    saveData();
}

void set_dev_lock() {
    dev_lock = 1;
    saveData();
}

void set_player_lock() {
    player_lock = 1;
    saveData();
}

void set_vip_lock() {
    vip_lock = 1;
    saveData();
}

void set_player_lock_msg(string arg) {
    player_lock_msg = arg;
    saveData();
}

void set_dev_lock_msg(string arg) {
    dev_lock_msg = arg;
    saveData();
}

void set_vip_lock_msg(string arg) {
    vip_lock_msg = arg;
    saveData();
}

void set_guest_lock_msg(string arg) {
    guest_lock_msg = arg;
    saveData();
}

void set_player_creation_msg(string arg) {
    player_creation_msg = arg;
    saveData();
}
