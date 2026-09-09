//=========================================================
// rust2b's remaining classes with no SDK precedent: ammo_dmlclip
// (CDMLClipAmmo), monster_maggot (CMaggot) and monster_tube (CTube).
// func_tanklaserrust is implemented in func_tank.cpp instead,
// alongside its stock CFuncTankLaser base. weapon_beamgun and
// weapon_minigun, rust2b's other two gaps, are already-deferred
// player weapons (see STATUS.md/memory).
//
// Decompiled fresh from gunman.dll this session:
//   ammo_dmlclip: LINK @0x10084900, vtable @0x100fb160, Spawn
//     @0x10084960, Precache @0x10084990 -> same shared
//     CBasePlayerAmmo tail as ammo_dmlsingle/ammo_beamgunclip. Model
//     "models/dmlammo.mdl" (the multi-round clip, vs.
//     ammo_dmlsingle's "models/singlerocket.mdl" single round),
//     pickup sound "items/9mmclip1.wav".
//   monster_maggot: LINK @0x100c39d0, vtable @0x101037e4, Spawn
//     @0x100c3ec0, Precache @0x100c3e90, Classify @0x100c4e70
//     (constant 0xd = 13 = CLASS_ALIEN_BIOWEAPON, same as
//     monster_critter). Confirmed bbox (-12,-12,0)/(12,12,8),
//     SOLID_SLIDEBOX, `CMaggot::SquashTouch` wired directly in Spawn
//     (named via the vtable-adjacent literal `param_1[5] =
//     CMaggot::SquashTouch`) - same squash-to-death pattern as
//     CCricket/CScorpion elsewhere in this project. Precache also
//     loads "models/larva.mdl" (matches
//     findings/entity_review_list.csv's "Zwei-Stadien-Kreatur"
//     description), confirming the two-stage larva/maggot life cycle,
//     though the stage-transition logic itself wasn't decompiled this
//     session (see Simplified section).
//   monster_tube: LINK @0x100c4e90, vtable @0x10103a20, Spawn
//     @0x100c5a90, Precache @0x100c5c70, Classify @0x100c6420
//     (constant 0xc = 12 = CLASS_PLAYER_BIOWEAPON). Confirmed bbox
//     (-32,-32,0)/(32,32,64), SOLID_SLIDEBOX. Confirms
//     findings/entities/xenome_family.md's "Queen-Kind"-flag finding
//     exactly: Spawn compares pev->owner's classname against the
//     literal string "monster_tubequeen" and, on a match, sets
//     pev->spawnflags bit 0x80000000 - reproduced faithfully here.
//
// Simplified relative to the original (documented per-case): neither
// CMaggot's larva/maggot stage-transition logic nor CTube's actual
// homing-projectile-launcher attack behavior (per findings, "Tube
// Launcher" fires homing biological projectiles) were decompiled this
// session - out of scope for two secondary xenome creatures at this
// map-driven pass, same simplification level as CCritter earlier in
// this file family. Both rely on default CBaseMonster AI beyond the
// confirmed Spawn/Precache/Classify/SquashTouch/owner-flag behavior.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

//=========================================================
// ammo_dmlclip - CDMLClipAmmo.
//=========================================================
class CDMLClipAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/dmlammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PrecacheModel("models/dmlammo.mdl");
		PrecacheSound("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(4, "dml", 10) != -1) // plausible give amount - weapon_dml is not yet implemented, see file header
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_dmlclip, CDMLClipAmmo);

//=========================================================
// monster_maggot - CMaggot.
//=========================================================
class CMaggot : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_BIOWEAPON; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	void EXPORT SquashTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(monster_maggot, CMaggot);

void CMaggot::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/maggot.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 8));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = gSkillData.maggotHealth;
	pev->view_ofs = Vector(0, 0, 20);
	pev->flags |= FL_MONSTER;

	MonsterInit();
	SetTouch(&CMaggot::SquashTouch);
}

void CMaggot::Precache()
{
	PrecacheModel("models/maggot.mdl");
	PrecacheModel("models/larva.mdl");
	PrecacheModel("sprites/greensplash.spr");

	PrecacheSound("maggot/maggot_angry1.wav");
	PrecacheSound("maggot/maggot_angry2.wav");
	PrecacheSound("maggot/maggot_flinch1.wav");
	PrecacheSound("maggot/maggot_flinch2.wav");
	PrecacheSound("maggot/maggot_idle1.wav");
	PrecacheSound("maggot/maggot_idle2.wav");
	PrecacheSound("maggot/maggot_idle3.wav");
	PrecacheSound("maggot/maggot_run1.wav");
	PrecacheSound("maggot/maggot_walk1.wav");
}

void CMaggot::SquashTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->IsPlayer() && FBitSet(pev->flags, FL_ONGROUND))
	{
		TakeDamage(pOther->pev, pOther->pev, 100, DMG_CRUSH);
	}
}

//=========================================================
// monster_tube - CTube.
//=========================================================
class CTube : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_PLAYER_BIOWEAPON; }
	// BUG FIX (2026-09-05): scripted_sequence-driven "waiting" poses
	// get stomped mid-animation without this - see the
	// feedback-setactivity-act-idle-stomps-scripted-sequence memory
	// note (same fix as CFriendlyGunman etc).
	void SetActivity(Activity NewActivity) override
	{
		if ((NewActivity == ACT_IDLE || NewActivity == ACT_RESET) && m_pCine != nullptr)
		{
			m_Activity = NewActivity;
			m_IdealActivity = NewActivity;
			return;
		}
		CBaseMonster::SetActivity(NewActivity);
	}
};
LINK_ENTITY_TO_CLASS(monster_tube, CTube);

void CTube::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/tube.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 60; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = Vector(0, 0, 40);
	pev->flags |= FL_MONSTER;

	// Confirmed: a tube spawned by a monster_tubequeen owner gets a
	// "queen-child" spawnflag bit set, matching
	// findings/entities/xenome_family.md's Tube-Launcher/Queen
	// relationship finding exactly.
	if (pev->owner && FClassnameIs(pev->owner, "monster_tubequeen"))
		pev->spawnflags |= 0x80000000;

	MonsterInit();
}

void CTube::Precache()
{
	PrecacheModel("models/tube.mdl");
	PrecacheModel("sprites/gibtube.spr");
	PrecacheModel("sprites/goretube.spr");
	PrecacheModel("sprites/tubeguts.spr");
}
