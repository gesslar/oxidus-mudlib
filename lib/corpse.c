/**
 * @file /std/area/corpse/corpse.c
 * Corpses oooOooOooOooO
 *
 * @created 2024-07-27 - Gesslar
 * @last_modified 2024-07-27 - Gesslar
 *
 * @history
 * 2024-07-27 - Gesslar - Created
 */

inherit STD_ITEM;
inherit STD_CONTAINER;

// Forward declarations
private void decay(int it);
public void setup_corpse(object dead, object killer);
public string query_killer_name();
public string query_dead_name();
public int is_pc_corpse();
public int is_npc_corpse();
public int is_corpse();

// Variables
private string dead_name, killer_name;
private nosave int pc_corpse, npc_corpse;
private nosave string *_shorts = ({
  "the dead body of {NAME}",
  "the bloated body of {NAME}",
  "the decaying body of {NAME}",
  "the rotting body of {NAME}",
  "the putrid body of {NAME}",
  "the decomposing body of {NAME}",
  "the skeletal remains of {NAME}",
});

public void setup_corpse(object dead, object killer) {
  string *corpse_shorts = copy(_shorts);
  string short = shift(ref corpse_shorts);

  killer_name = killer ? killer->query_name() : "unknown";
  dead_name = dead->query_name();

  set_id(({ "body", "dead body", "corpse" }));
  set_name(dead_name+"'s body");

  short = replace_string(short, "{NAME}", dead_name);

  set_short(short);
  set_long("This is the dead body of "+dead_name+".");
  set_mass(1);
  set_capacity(1000);

  pc_corpse = dead->is_pc();
  npc_corpse = dead->is_npc();

  call_out_walltime((: decay, corpse_shorts :), 5.0+random_float(5.0));
}

public string query_killer_name() { return killer_name; }
public string query_dead_name() { return dead_name; }

public int is_pc_corpse() { return pc_corpse; }
public int is_npc_corpse() { return npc_corpse; }

private void decay(string *corpse_shorts) {
  string short;

  if(!environment())
    return;

  short = shift(ref corpse_shorts);
  short = replace_string(short, "{NAME}", dead_name);
  set_short(short);

  if(sizeof(corpse_shorts) == 0) {
    call_out_walltime(function() {
      // clean_contents();
      remove();
    }, 1.0);
    return;
  }

  call_out_walltime((: decay, corpse_shorts :), 5.0+random_float(5.0));
}

public int is_corpse() { return 1; }
