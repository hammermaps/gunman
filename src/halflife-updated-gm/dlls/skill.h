/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//=========================================================
// skill.h - skill level concerns
//=========================================================

#pragma once

struct skilldata_t
{

	int iSkillLevel; // game skill level

	// Monster Health & Damage
	float agruntHealth;
	float agruntDmgPunch;

	float apacheHealth;

	float barneyHealth;

	float bigmommaHealthFactor; // Multiply each node's health by this
	float bigmommaDmgSlash;		// melee attack damage
	float bigmommaDmgBlast;		// mortar attack damage
	float bigmommaRadiusBlast;	// mortar attack radius

	float bullsquidHealth;
	float bullsquidDmgBite;
	float bullsquidDmgWhip;
	float bullsquidDmgSpit;

	float gargantuaHealth;
	float gargantuaDmgSlash;
	float gargantuaDmgFire;
	float gargantuaDmgStomp;

	float hassassinHealth;

	float headcrabHealth;
	float headcrabDmgBite;

	float hgruntHealth;
	float hgruntDmgKick;
	float hgruntShotgunPellets;
	float hgruntGrenadeSpeed;

	float houndeyeHealth;
	float houndeyeDmgBlast;

	float slaveHealth;
	float slaveDmgClaw;
	float slaveDmgClawrake;
	float slaveDmgZap;

	float ichthyosaurHealth;
	float ichthyosaurDmgShake;

	float leechHealth;
	float leechDmgBite;

	float controllerHealth;
	float controllerDmgZap;
	float controllerSpeedBall;
	float controllerDmgBall;

	float nihilanthHealth;
	float nihilanthZap;

	float scientistHealth;

	float snarkHealth;
	float snarkDmgBite;
	float snarkDmgPop;

	float zombieHealth;
	float zombieDmgOneSlash;
	float zombieDmgBothSlash;

	float turretHealth;
	float miniturretHealth;
	float sentryHealth;

	// Nachtrag 2026-09-06 (RE-Nachtrag "Fauna/Cricket", monster_ourano):
	// echte Retail-Skill.cfg (Gunman-ENG-GER/rewolf/Skill.cfg) hat eigene
	// Gunman-Felder ohne SDK-Vorbild - ouranoHealth per Skill-Level
	// (sk_ourano_h1/h2/h3 = 70/70/80) sowie ein globaler
	// Zufallsvarianz-Prozentsatz (sk_percent_random_h1/h2/h3 = 6/12/18),
	// der laut Decompile von COurano::Spawn (FUN_100608d0 @0x100608d0)
	// auf die Basis-Health addiert wird: health = base +
	// RANDOM_FLOAT(0,base) * (percentRandomHealth * 0.01). Siehe
	// findings/entities/ourano_health_and_fauna_re.md.
	float ouranoHealth;
	float percentRandomHealth;

	// Nachtrag 2026-09-06 (Cut-Content-Rekonstruktion, monster_penta):
	// keine Retail-Skill.cfg-Werte mehr vorhanden (Cvars dort entfernt) -
	// diese Werte stammen aus der E3-2000-Beta-Skill.cfg
	// (sk_penta_h1-3=300/400/500, sk_penta_bite_d1-3=30/40/50), siehe
	// findings/entities/cutcontent_penta_batterybot.md. monster_battery
	// (CBatteryBot) hat keine bekannten Skill-Werte - siehe dortiger
	// Klassenkopf-Kommentar fuer den verwendeten Platzhalter.
	float pentaHealth;
	float pentaBiteDamage;

	// Retail Gunman: monster_maggot uses sk_maggot_h<level>. The three
	// shipped values are all 10, but the value still belongs in Skill.cfg.
	float maggotHealth;


	// Player Weapons
	float plrDmgCrowbar;
	float plrDmg9MM;
	float plrDmg357;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgCrossbowClient;
	float plrDmgCrossbowMonster;
	float plrDmgRPG;
	float plrDmgGauss;
	float plrDmgEgonNarrow;
	float plrDmgEgonWide;
	float plrDmgHornet;
	float plrDmgHandGrenade;
	float plrDmgSatchel;
	float plrDmgTripmine;

	// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float monDmg12MM;
	float monDmgHornet;

	// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
	float scientistHeal;

	// monster damage adj
	float monHead;
	float monChest;
	float monStomach;
	float monLeg;
	float monArm;

	// player damage adj
	float plrHead;
	float plrChest;
	float plrStomach;
	float plrLeg;
	float plrArm;
};

inline DLL_GLOBAL skilldata_t gSkillData;
float GetSkillCvar(const char* pName);

inline DLL_GLOBAL int g_iSkillLevel;

#define SKILL_EASY 1
#define SKILL_MEDIUM 2
#define SKILL_HARD 3
