// Private/Core/RadiantGameplayTags.cpp

#include "Core/RadiantGameplayTags.h"

// === NPC TYPE TAGS ===
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC, "NPC");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type, "NPC.Type");

// Humanoid NPCs
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid, "NPC.Type.Humanoid");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid_Villager, "NPC.Type.Humanoid.Villager");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid_Guard, "NPC.Type.Humanoid.Guard");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid_Merchant, "NPC.Type.Humanoid.Merchant");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid_Bandit, "NPC.Type.Humanoid.Bandit");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid_Noble, "NPC.Type.Humanoid.Noble");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid_Soldier, "NPC.Type.Humanoid.Soldier");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Humanoid_Civilian, "NPC.Type.Humanoid.Civilian");

// Creature NPCs
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Creature, "NPC.Type.Creature");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Creature_Animal, "NPC.Type.Creature.Animal");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Creature_Monster, "NPC.Type.Creature.Monster");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Creature_Undead, "NPC.Type.Creature.Undead");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Creature_Construct, "NPC.Type.Creature.Construct");

// Special NPCs
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Special, "NPC.Type.Special");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Special_QuestGiver, "NPC.Type.Special.QuestGiver");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Special_Boss, "NPC.Type.Special.Boss");
UE_DEFINE_GAMEPLAY_TAG(TAG_NPC_Type_Special_Companion, "NPC.Type.Special.Companion");

// === FACTION TAGS ===
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction, "Faction");

// Major Factions
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Kingdom, "Faction.Kingdom");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Kingdom_Guards, "Faction.Kingdom.Guards");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Kingdom_Military, "Faction.Kingdom.Military");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Kingdom_Nobility, "Faction.Kingdom.Nobility");

UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Merchants, "Faction.Merchants");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Merchants_TradeGuild, "Faction.Merchants.TradeGuild");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Merchants_Independent, "Faction.Merchants.Independent");

UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Bandits, "Faction.Bandits");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Bandits_RedCloaks, "Faction.Bandits.RedCloaks");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Bandits_IronWolves, "Faction.Bandits.IronWolves");

UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Villagers, "Faction.Villagers");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Villagers_Peaceful, "Faction.Villagers.Peaceful");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Villagers_Militia, "Faction.Villagers.Militia");

// Special Factions
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Neutral, "Faction.Neutral");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Wildlife, "Faction.Wildlife");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Hostile, "Faction.Hostile");
UE_DEFINE_GAMEPLAY_TAG(TAG_Faction_Player, "Faction.Player");

// === AI INTENT TAGS ===
UE_DEFINE_GAMEPLAY_TAG(TAG_AI, "AI");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent, "AI.Intent");

// Basic Intents
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Idle, "AI.Intent.Idle");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Wander, "AI.Intent.Wander");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Patrol, "AI.Intent.Patrol");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Guard, "AI.Intent.Guard");

// Survival Intents
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Survival, "AI.Intent.Survival");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Survival_Eat, "AI.Intent.Survival.Eat");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Survival_Sleep, "AI.Intent.Survival.Sleep");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Survival_Flee, "AI.Intent.Survival.Flee");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Survival_Hide, "AI.Intent.Survival.Hide");

// Social Intents
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Social, "AI.Intent.Social");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Social_Talk, "AI.Intent.Social.Talk");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Social_Trade, "AI.Intent.Social.Trade");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Social_Follow, "AI.Intent.Social.Follow");

// Combat Intents
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Combat, "AI.Intent.Combat");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Combat_Attack, "AI.Intent.Combat.Attack");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Combat_Defend, "AI.Intent.Combat.Defend");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Combat_Retreat, "AI.Intent.Combat.Retreat");

// Curiosity Intents
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Curiosity, "AI.Intent.Curiosity");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Curiosity_Explore, "AI.Intent.Curiosity.Explore");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Curiosity_Investigate, "AI.Intent.Curiosity.Investigate");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Intent_Curiosity_Watch, "AI.Intent.Curiosity.Watch");

// === BEHAVIOR TAGS ===
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior, "Behavior");
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior_Idle, "Behavior.Idle");
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior_Moving, "Behavior.Moving");
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior_Working, "Behavior.Working");
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior_Socializing, "Behavior.Socializing");
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior_Combat, "Behavior.Combat");
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior_Fleeing, "Behavior.Fleeing");
UE_DEFINE_GAMEPLAY_TAG(TAG_Behavior_Investigating, "Behavior.Investigating");

// === ZONE TAGS ===
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone, "Zone");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Settlement, "Zone.Settlement");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Settlement_Village, "Zone.Settlement.Village");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Settlement_Town, "Zone.Settlement.Town");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Settlement_City, "Zone.Settlement.City");

UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Wilderness, "Zone.Wilderness");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Wilderness_Forest, "Zone.Wilderness.Forest");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Wilderness_Plains, "Zone.Wilderness.Plains");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Wilderness_Mountains, "Zone.Wilderness.Mountains");

UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Dangerous, "Zone.Dangerous");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Dangerous_BanditCamp, "Zone.Dangerous.BanditCamp");
UE_DEFINE_GAMEPLAY_TAG(TAG_Zone_Dangerous_MonsterLair, "Zone.Dangerous.MonsterLair");

// === AI EVENT TAGS ===
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Event, "AI.Event");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Event_Combat, "AI.Event.Combat");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Event_Death, "AI.Event.Death");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Event_Social, "AI.Event.Social");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Event_Trade, "AI.Event.Trade");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Event_Time, "AI.Event.Time");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Event_Threat, "AI.Event.Threat");