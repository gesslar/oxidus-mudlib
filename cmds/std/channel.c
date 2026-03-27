/**
 * @file /cmds/std/channel.c
 *
 * Command to interact with channel networks, including listing,
 * tuning, and displaying channel members.
 *
 * @created 2005-10-02 - Tacitus @ LPUniversity
 * @last_modified 2006-07-14 - Tacitus
 *
 * @history
 * 2005-10-02 - Tacitus - Created
 * 2006-01-15 - Tacitus - Rewrote
 * 2006-07-14 - Tacitus - Last edited
 */

inherit STD_CMD;

private string formatChannelList(string *channels);
private void displayChannelsByModule(object tp);
private void displayChannelMembers(object tp, string channel);
private void displayTunedChannels(object tp, string name);
public varargs int tune(string channel, string name,
    int inOut, int silent);

mixed main(object tp, string args) {
  string cmd, arg;
  string name;

  name = query_privs(tp);

  if(!args) {
    displayTunedChannels(tp, name);
    return 1;
  }

  if(sscanf(args, "%s %s", cmd, arg) != 2)
    cmd = args;

  switch(cmd) {
  case "list":
    displayChannelsByModule(tp);
    return 1;

  case "show":
    if(!arg)
      return "Syntax: channel show <channel_name>\n";
    displayChannelMembers(tp, arg);
    return 1;

  case "tune": {
    string direction, channel;
    if(!arg)
      return "Syntax: channel tune <in/out> "
        "<channel/network/all>\n";
    if(sscanf(arg, "%s %s", direction, channel) != 2)
      return "Syntax: channel tune <in/out> "
        "<channel/network/all>\n";
    tune(channel, name, direction == "in", 0);
    return 1;
  }
  case "tuned":
    displayTunedChannels(tp, arg || name);
    return 1;

  default:
    return "Syntax: channel "
      "[list/show/tune/tuned] [argument]\n";
  }
}

private string formatChannelList(string *channels) {
  if(sizeof(channels) == 1)
    return channels[0];
  else if(sizeof(channels) > 7)
    return sprintf("%s,\n\t%s, and %s",
      implode(channels[0..6], ", "),
      implode(channels[7..<2], ", "),
      channels[<1]);
  else
    return sprintf("%s, and %s",
      implode(channels[0..<2], ", "),
      channels[<1]);
}

private void displayChannelsByModule(object tp) {
  string *modules;
  string output;
  int i;
  string name = query_privs(tp);

  modules = sort_array(CHAN_D->get_modules(), 1);
  output = "Channels by module:\n";

  for(i = 0; i < sizeof(modules); i++) {
    string *channels =
      sort_array(CHAN_D->get_channels(modules[i], name), 1);
    if(sizeof(channels) > 0)
      output += sprintf("%s - %s\n",
        modules[i], implode(channels, ", "));
  }

  tell(tp, output);
}

private void displayChannelMembers(object tp, string channel) {
  string *members;

  if(!CHAN_D->valid_channel(channel)) {
    tell(tp, "Channel: Channel " + channel +
      " does not exist.\n");
    return;
  }

  members = CHAN_D->get_tuned(channel);
  if(sizeof(members) > 0) {
    tell(tp, sprintf("Users tuned into '%s':\n\t%s\n",
      channel,
      formatChannelList(
        map(members, (: capitalize :)))));
  } else {
    tell(tp, "Channel: No users tuned into channel " +
      channel + "\n");
  }
}

private void displayTunedChannels(object tp, string name) {
  string *allChannels, *tunedChannels;
  int i;

  if(name != query_privs(tp) && !wizardp(tp))
    name = query_privs(tp);

  allChannels = CHAN_D->get_channels("all", name);
  tunedChannels = ({});

  for(i = 0; i < sizeof(allChannels); i++) {
    string *members = CHAN_D->get_tuned(allChannels[i]);
    if(member_array(name, members) != -1)
      tunedChannels += ({ allChannels[i] });
  }

  if(sizeof(tunedChannels) > 0) {
    tell(tp, sprintf("%s currently tuned into:\n\t%s\n",
      (name == query_privs(tp) ?
        "You are" : capitalize(name) + " is"),
      formatChannelList(tunedChannels)));
  } else {
    tell(tp, sprintf(
      "%s not currently tuned into any channels.\n",
      (name == query_privs(tp) ?
        "You are" : capitalize(name) + " is")));
  }
}

public varargs int tune(string channel, string name,
    int inOut, int silent) {
  string *channels;
  int result, i;

  if(nullp(channel) || !stringp(channel)
  || nullp(name) || !stringp(name))
    return 0;

  result = 1;

  if(channel == "all") {
    channels = CHAN_D->get_channels("all", name);
    if(sizeof(channels) == 0) {
      if(!silent)
        tell(this_player(),
          "No channels available to tune.\n");
      return 0;
    }
  } else {
    channels = ({ channel });
  }

  for(i = 0; i < sizeof(channels); i++) {
    int tuneResult;

    tuneResult = CHAN_D->tune(channels[i], name, inOut);
    if(!silent) {
      if(tuneResult)
        tell(this_player(),
          sprintf("%s channel tuned %s.\n",
            capitalize(channels[i]),
            inOut ? "in" : "out"));
      else
        tell(this_player(),
          sprintf("Channel %s does not exist.\n",
            channels[i]));
    }
    result = result && tuneResult;
  }

  return result;
}

string query_help(object _caller) {
  string *modList = CHAN_D->get_modules(), mods;

  if(!sizeof(modList))
    mods = "There is no networks currently installed.";
  if(sizeof(modList) == 1)
    mods = modList[0];
  else
    mods = implode(modList[0..(sizeof(modList) - 2)],
      ", ") + ", " + modList[sizeof(modList) - 1];

  return
" SYNTAX: channel <list/show/tune> <argument>\n\n"
"This command allows you to interact with the different "
"channel networks\navailable here on " +
capitalize(mud_name()) + ". More specifically,\n"
"it currently allows you to list the different channels "
"that you can\ntune into, display who is tuned into a "
"channel, and the ability to\ntune in and out of "
"channels/networks. If you are wondering what a\n"
"network is, it is a group of channels. You may notice "
"that different\nnetworks have different channels, "
"different features, and different\nusers/muds "
"communicating through it. Currently, the following "
"networks\nare installed on " +
capitalize(mud_name()) + ":\n"
"\t" + mods + "\n";
}
