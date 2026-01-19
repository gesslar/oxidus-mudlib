/* chan_d.c

 Tacitus @ LPUniversity
 30-SEPT-05
 Modular Channel Daemon

 Version 2.5
*/

#include <gmcp_defines.h>

inherit STD_DAEMON;

private nosave mapping channels;
private nosave mapping modules;

int register_module(string name, string path);
int register_channel(string module_name, string channel_name);
int remove_module(string module_name);
int remove_channel(string channel_name);
int tune(string channel, string user, int flag);
int valid_channel(string channel_name);
int valid_module(string module_name);
int chat(string channel, string user, string msg);
void grapevine_chat(mapping data);
varargs int filter_listing(string element, string name);
string *get_channels(string module_name);
string *get_modules();
string *get_tuned(string argument);
void rec_msg(string channel, string user, string msg);

void setup() {
  string err, *arr;
  int i;
  float time;

  set_no_clean();

  channels = ([]);
  modules = ([]);

  arr = explode_file(__DIR__ "modules/channel/installed_modules");

  if(!sizeof(arr))
    return;

  for(i = 0; i < sizeof(arr); i++) {
    /** @type {STD_DAEMON} */
    object ob = find_object(arr[i]);

    if(ob)
      ob->remove();

    debug("> Loading channel module: %s", arr[i]);
    time = time_frac();
    err = catch(load_object(arr[i]));

    if(err != 0)
      debug("< Error %s when loading %s", err, arr[i]);
    else
      debug("< Done (%.2fs)", time_frac() - time);
  }

  set_no_clean(1);
}

int register_module(string name, string path) {
  string *keys;

  keys = keys(modules);
  if(!name || !path)
    return 0;

  if(member_array(name, keys) != -1) {
    if(modules[name] == path)  {
      debug("  > Module %s already registered to path %s", name, path);
      return 1;
    } else {
      debug("  > Module %s already registered to path %s", name, modules[name]);
      return -1;
    }
  }

  modules[name] = path;

  debug("  > Module %s registered to path %s", name, path);

  return 1;
}

int unregister_module(string module_name) {
  string *keys = keys(modules);

  if(!module_name || member_array(module_name, keys) == -1)
    return 0;

  if(modules[module_name] != file_name(previous_object()))
    return 0;

  foreach(mixed key, mixed value in channels)
    if(value["module"] == module_name)
      map_delete(channels, key);

  return 1;
}

int register_channel(string module_name, string channel_name) {
  string *keys;
  string new_name = channel_name;

  if(!valid_module(module_name))
    return -1;

  keys = keys(modules);
  if(member_array(module_name, keys) == -1)
    return -1;

  keys = keys(channels);

  if(modules[module_name] != file_name(previous_object()))
    return 0;

  if(member_array(channel_name, keys) != -1) {
    if(channels[channel_name]["module"] == module_name) {
      channels[channel_name]["listeners"] = ({});
      return 1;
    }
    else
      new_name = module_name[0..3] + channel_name;
  }

  channels[new_name] = (["module" : module_name, "real_name" : channel_name, "listeners" : ({})]);

  debug("   > Channel %s registered to module %s", new_name, module_name);

  return 1;
}

int remove_channel(string channel_name) {
  string *keys;

  keys = keys(channels);
  if(member_array(channel_name, keys) == -1)
    return 0;

  map_delete(channels, channel_name);

  return 1;
}

int tune(string channel, string user, int flag) {
  string *keys;
  object mod_obj;

  keys = keys(channels);
  if(member_array(channel, keys) == -1)
    return 0;

  if(!valid_module(channels[channel]["module"])) {
    map_delete(channels, channel);
    return 0;
  }

  mod_obj = find_object(modules[channels[channel]["module"]]);

  if(!mod_obj->is_allowed(channels[channel]["real_name"], user, flag))
    return 0;

  if(flag == 1 && member_array(user, channels[channel]["listeners"]) == -1)
    channels[channel]["listeners"] += ({user});

  if(flag == 0 && member_array(user, channels[channel]["listeners"]) != -1)
    channels[channel]["listeners"] -= ({user});

  return 1;
}

int valid_channel(string channel_name) {
  return !nullp(channels[channel_name]);
}

int valid_module(string module_name) {
  return !nullp(modules[module_name]);
}

int chat(string channel, string user, string msg) {
  object mod_obj;
  string *keys;

  keys = keys(channels);
  if(member_array(channel, keys) == -1)
    return 0;

  if(!valid_module(channels[channel]["module"])) {
    map_delete(channels, channel);
    return 0;
  }

  mod_obj = find_object(modules[channels[channel]["module"]]);

  if(member_array(user, channels[channel]["listeners"]) == -1) return 0;
  if(!mod_obj->is_allowed(channels[channel]["real_name"], user)) return 0;
  if(!msg) return(notify_fail("Syntax: <channel> <msg>\n"));
  if(mod_obj->rec_msg(channels[channel]["real_name"], user, msg)) return 1;
  else return 0;
}

void grapevine_chat(mapping payload) {
  object mod_obj;
  string *keys;
  string channel, user, msg, game;

  channel = payload["channel"];
  msg = payload["message"];
  game = payload["game"];
  user = payload["name"];

  keys = keys(channels);
  if(member_array(channel, keys) == -1) return 0;

  if(!valid_module(channels[channel]["module"])) {
    map_delete(channels, channel);
    return 0;
  }

  mod_obj = find_object(modules[channels[channel]["module"]]);
  return call_if(mod_obj, "rec_grapevine_msg", channel, user, msg, game);
}

string *get_channels(string module_name, string name) {
  string *ret = ({}), *keys;
  int i;

  keys = keys(channels);

  if(module_name == "all")
    ret = keys;
  else {
    for(i = 0; i < sizeof(keys); i++)
    if(channels[keys[i]]["module"] == module_name) ret += ({keys[i]});
  }

  ret = filter_array(ret, "filter_listing", this_object(), name);
  ret = sort_array(ret, 1);

  return ret;
}

string *get_tuned(string argument) {
  string *ret = ({});

  if(!argument)
    return ret;

  if(sizeof(channels[argument]["listeners"]) <= 0)
    return ret;

  ret = channels[argument]["listeners"];

  foreach(string name in ret)
    if(find_living(name))
      if(!interactive(find_living(name)))
        ret -= ({ name });

  return ret;
}

varargs int filter_listing(string element, string name) {
  object mod_obj;

  if(!element)
    return 0;

  if(!name) {
    if(!this_body())
      return 0;
    else
      name = query_privs(this_body());
  }

  catch(mod_obj = load_object(modules[channels[element]["module"]]));

  if(!mod_obj)
    return 0;
  if(mod_obj->is_allowed(element, name))
    return 1;

  return 0;
}

string *get_modules() {
  string *keys = keys(modules);

  return keys;
}

void rec_msg(string channel, string user, string msg) {
  string *listeners;
  string *keys;
  int i;
  object ob;
  mapping payload;

  keys = keys(channels);
  if(member_array(channel, keys) == -1)
    return 0;

  if(!valid_module(channels[channel]["module"])) {
    map_delete(channels, channel);
    return 0;
  }

  payload = ([
    "channel" : channel,
    "talker" : user,
    "text" : msg,
  ]);

  listeners = channels[channel]["listeners"];
  listeners -= ({ 0 });
  if(listeners) {
    for(i = 0; i < sizeof(listeners); i ++) {
      ob = find_living(listeners[i]);

      if(!objectp(ob)) {
        channels[channel]["listeners"] -= ({ listeners[i] });
        continue;
      }

      tell(ob, msg);
      GMCP_D->send_gmcp(ob, GMCP_PKG_COMM_CHANNEL_TEXT, payload);
    }
  }
}
