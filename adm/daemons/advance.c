/**
 * @file /adm/daemons/advance.c
 * @description Daemon to handle XP and leveling
 *
 * @created 2024-07-28 - Gesslar
 * @last_modified 2024-07-28 - Gesslar
 *
 * @history
 * 2024-07-28 - Gesslar - Created
 */

inherit STD_DAEMON;

// Variables
public float baseTnl;
public float tnlRate;
public float overlevelThreshold, overlevelXpPunish;
public float underlevelThreshold, underlevelXpBonus;

// Functions
public int toNextLevel(float level);

void setup() {
  baseTnl = mudConfig("BASE_TNL");
  tnlRate = mudConfig("TNL_RATE");
  overlevelThreshold = mudConfig("OVERLEVEL_THRESHOLD");
  overlevelXpPunish = mudConfig("OVERLEVEL_XP_PUNISH");
  underlevelThreshold = mudConfig("UNDERLEVEL_THRESHOLD");
  underlevelXpBonus = mudConfig("UNDERLEVEL_XP_BONUS");
}

/**
 * Calculates the total XP needed to reach the next level from the current
 * level.
 *
 * @public
 * @param {float} level - The current level to calculate TNL for.
 * @returns {int} The total XP needed to reach the next level.
 */
public int toNextLevel(float level) {
  // Ensure level is at least 1.0
  if(level < 1.0)
    level = 1.0;

  // Calculate TNL
  return to_int(baseTnl * pow(tnlRate, level - 1.0));
}

/**
 * Checks if a character has enough XP to advance to the next level.
 *
 * @public
 * @param {int} xp - The current XP amount.
 * @param {float} level - The current level.
 * @returns {int} 1 if the character can advance, 0 otherwise.
 */
public int canAdvance(int xp, float level) {
  return xp >= toNextLevel(level);
}

/**
 * Advances a character to the next level if they have enough XP. Deducts the
 * required XP, increments the level, and emits a signal.
 *
 * @public
 * @param {STD_PLAYER} tp - The player object to advance.
 * @returns {int} 1 if the character advanced, 0 if they did not have enough XP.
 */
public int advance(object tp) {
  int xp = tp->queryXp();
  float level = tp->queryLevel();
  int toNextLevel = toNextLevel(level);

  if(!canAdvance(xp, level))
    return 0;

  xp -= toNextLevel;
  level += 1.0;

  tp->setLevel(level);
  tp->setXp(xp);

  emit(SIG_PLAYER_ADVANCED, tp, level);

  return 1;
}

/**
 * Awards XP to a player and optionally auto-levels them if enabled in the mud
 * configuration.
 *
 * @public
 * @param {STD_PLAYER} tp - The player object to award XP to.
 * @param {int} amount - The amount of XP to award.
 * @returns {int} Always returns 1.
 */
public int earnXp(object tp, int amount) {
  tp->adjustXp(amount);

  if(mudConfig("PLAYER_AUTOLEVEL"))
    advance(tp);

  return 1;
}

/**
 * Calculates and awards XP to a killer for defeating an opponent. The XP
 * amount is based on the killed opponent's level with variance, and adjusted
 * based on level difference between killer and killed.
 *
 * @public
 * @param {STD_PLAYER} killer - The object that killed the opponent.
 * @param {STD_NPC | STD_PLAYER} killed - The object that was killed.
 * @returns {int} - The amount of XP awarded, or 0 if either object is null.
 */
int killXp(object killer, object killed) {
  if(nullp(killer) || nullp(killed))
    return 0;

  float killerLevel = killer->query_effective_level();
  float killedLevel = killed->query_effective_level();

  float toNextLevel = toNextLevel(killedLevel);
  float xp = (toNextLevel / 10);
  float variance = xp / 10;
  xp = xp - variance + random(variance);

  // Calculate the level difference
  float levelDifference = killerLevel - killedLevel;

  // Initialize the adjustment factor
  float adjustmentFactor = 1.0;

  // Apply punishments if the player is higher level by more than the monster
  if(levelDifference > overlevelThreshold)
    adjustmentFactor -= overlevelXpPunish * (levelDifference - overlevelThreshold);
  // Apply bonuses if the player is lower level by more than the monster
  else if(levelDifference < underlevelThreshold)
    adjustmentFactor += underlevelXpBonus * (-levelDifference);

  // Apply the adjustment factor to the XP
  xp = to_int(xp * adjustmentFactor);

  earnXp(killer, xp);

  return xp;
}
