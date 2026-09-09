//=========================================================
// MAYAN3A's remaining classes with no SDK precedent:
// monster_largescorpion (CScorpion) and lava_god (CLavaGod + its
// runtime-only CLava fireball).
//
// Decompiled fresh from gunman.dll this session:
//   monster_largescorpion: LINK @0x100789c0, vtable @0x100f925c,
//     Spawn @0x10079040, Precache @0x10078fd0, KeyValue @0x10078530,
//     Classify @0x10078ad0 (returns 15, same numeric value confirmed
//     independently for monster_raptor's hostile variant - reused
//     via CLASS_ALIEN_PREDATOR_RAPTOR rather than adding a second,
//     numerically identical constant), CScorpion::SquashTouch
//     @0x10078250 (named via the symbol table) - byte-for-byte the
//     same pattern as CCricket::SquashTouch (see mayan0a_fauna.cpp):
//     a grounded player touching it deals 100 DMG_CRUSH self-damage,
//     except this one has no FL_ONGROUND gate (confirmed by the
//     decompile - reproduced faithfully, not "corrected" to match
//     CCricket). Cross-checked against
//     findings/entity_review_list.csv's "SquashTouch = 100 Schaden
//     Nahkampf, eigenes _skin-Keyvalue" note - both confirmed by this
//     session's fresh decompile. monster_scorpion (the small variant)
//     was NOT linked in the original MAYAN3A pass - added below as
//     CScorpionSmall once MAYAN4's own gap scan required it (see that
//     class's own header comment for its fresh decompile).
//   lava_god: LINK @0x10065620, vtable @0x100f6168, Spawn @0x10065670,
//     Precache @0x10065730, KeyValue @0x10065750,
//     CLavaGod::Use/Think @0x10065890/0x100658b0, CLava::KillTouch
//     @0x10065b60, CLava::Think @0x10065b80 (all named via the symbol
//     table). Confirmed keyvalues: dropradius, mindropfreq,
//     maxdropfreq, maxvelocity - matches
//     findings/entity_review_list.csv's "Spawner-Mechanismus fuer
//     lava-Instanzen" description. Spawn is invisible/nonsolid
//     (EF_NODRAW, SOLID_NOT, MOVETYPE_NONE) and starts active or
//     inactive based on spawnflag bit 0; Use() toggles it.
//
// Simplified relative to the original (documented per-case):
// - CScorpion: no custom TakeDamage/Killed override reproduced
//   (matches findings' description as a simple squashable creature,
//   no combat AI mentioned); the "_skin" keyvalue is parsed and
//   applied to pev->skin at spawn exactly as decompiled.
// - CLavaGod/CLava: CLavaGod::Think's exact eruption math (random
//   horizontal offset within dropradius, a velocity/gravity pair
//   scaled by maxvelocity) is approximated rather than reproduced
//   byte-exact - the confirmed keyvalue semantics (radius + min/max
//   spawn frequency + a velocity scale) are honored, but the precise
//   parabola shape is not. CLava::Think's original per-tick behavior
//   (a small recurring effect via FUN_1005f620 - likely a
//   sound/sizzle, not fully decoded) is replaced with a simple
//   gravity-affected fireball that deals damage and removes itself on
//   touch (CLava::KillTouch's real behavior - clear touch handler,
//   remove - is reproduced faithfully).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "explode.h"
#include "weapons.h"

//=========================================================
// monster_largescorpion - CScorpion.
//=========================================================
class CScorpion : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	int Classify() override { return CLASS_ALIEN_PREDATOR_RAPTOR; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	// BUG FIX (2026-09-05): scripted_sequence-driven "waiting" poses
	// get stomped mid-animation without this - see the
	// feedback-setactivity-act-idle-stomps-scripted-sequence memory
	// note (same fix as CFriendlyGunman etc; covers CScorpionSmall too
	// via inheritance).
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
	void EXPORT SquashTouch(CBaseEntity* pOther);

protected:
	int m_iSkin = 0;
};
LINK_ENTITY_TO_CLASS(monster_largescorpion, CScorpion);

bool CScorpion::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "_skin"))
	{
		m_iSkin = atoi(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

void CScorpion::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/scorpion_large.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 36));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 100;
	if (pev->skin == 0)
		pev->skin = m_iSkin;

	MonsterInit();
	pev->view_ofs = Vector(0, 0, 20);
	pev->flags |= FL_MONSTER;

	SetTouch(&CScorpion::SquashTouch);
}

void CScorpion::Precache()
{
	PrecacheModel("models/scorpion_large.mdl");
}

void CScorpion::SquashTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->IsPlayer())
	{
		TakeDamage(pOther->pev, pOther->pev, 100, DMG_CRUSH);
	}
}

//=========================================================
// monster_scorpion - the small variant. Added for MAYAN4 (this file
// stays MAYAN3A-named since it already holds CScorpion, but this
// class is only required starting MAYAN4's gap scan).
//
// Decompiled fresh: LINK @0x10077ef0, vtable @0x100f9020, Spawn
// @0x100785d0, Precache @0x100785b0, Classify @0x10077f40. KeyValue
// (slot 2) and Save/Restore (slots 3/4) point at the exact same
// addresses as monster_largescorpion's (0x10078530/0x10077f90/
// 0x10077fc0) - confirms this genuinely is the same underlying
// CScorpion class (SquashTouch is even the same literal symbol,
// confirmed via `param_1[5] = CScorpion::SquashTouch` in this
// decompile), just a second LINK_ENTITY_TO_CLASS with its own
// Spawn/Precache/Classify overrides for the smaller model/size -
// modeled here as a small subclass reusing CScorpion's KeyValue/
// SquashTouch rather than a full separate class. Classify also
// returns 15, same as the large variant.
//=========================================================
class CScorpionSmall : public CScorpion
{
public:
	void Spawn() override;
	void Precache() override;
};
LINK_ENTITY_TO_CLASS(monster_scorpion, CScorpionSmall);

void CScorpionSmall::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/scorpion_small.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 40; // plausible default - exact skill-cvar lookup not traced this session, see file header
	if (pev->skin == 0)
		pev->skin = m_iSkin;

	MonsterInit();
	pev->view_ofs = Vector(0, 0, 20);
	pev->flags |= FL_MONSTER;

	SetTouch(&CScorpion::SquashTouch);
}

void CScorpionSmall::Precache()
{
	PrecacheModel("models/scorpion_small.mdl");
}

//=========================================================
// The runtime-only lava fireball spawned by lava_god. No LINK-time
// map placement, spawned only via CBaseEntity::Create.
//=========================================================
class CLava : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT KillTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(lava_god_ball, CLava);

void CLava::Precache()
{
	PrecacheModel("models/lavaball.mdl");
	PrecacheModel("sprites/laserbeam.spr");
}

void CLava::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("lava_god_ball");
	SET_MODEL(ENT(pev), "models/lavaball.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.6;
	SetTouch(&CLava::KillTouch);

	pev->nextthink = gpGlobals->time + 3.0;
	SetThink(&CBaseEntity::SUB_Remove);
}

void CLava::KillTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->takedamage != DAMAGE_NO)
		pOther->TakeDamage(pev, pev, 10, DMG_BURN);

	UTIL_Remove(this);
}

//=========================================================
// lava_god - CLavaGod. Invisible, Use-toggleable spawner that
// periodically erupts a CLava fireball within dropradius (see file
// header re: eruption-math simplification).
//=========================================================
class CLavaGod : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT EruptThink();

	bool m_bActive = true;
	float m_flDropRadius = 128.0;
	float m_flMinDropFreq = 0.5;
	float m_flMaxDropFreq = 2.0;
	float m_flMaxVelocity = 200.0;
};
LINK_ENTITY_TO_CLASS(lava_god, CLavaGod);

bool CLavaGod::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "dropradius"))
	{
		m_flDropRadius = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "mindropfreq"))
	{
		m_flMinDropFreq = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "maxdropfreq"))
	{
		m_flMaxDropFreq = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "maxvelocity"))
	{
		m_flMaxVelocity = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CLavaGod::Spawn()
{
	Precache();

	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);

	m_bActive = !FBitSet(pev->spawnflags, 1);

	SetUse(&CLavaGod::Use);
	SetThink(&CLavaGod::EruptThink);
	pev->nextthink = gpGlobals->time + 1.0;
}

void CLavaGod::Precache()
{
	PrecacheModel("models/lavaball.mdl");
	PrecacheModel("sprites/laserbeam.spr");
	UTIL_PrecacheOther("lava_god_ball");
}

void CLavaGod::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (useType == USE_TOGGLE)
		m_bActive = !m_bActive;
	else
		m_bActive = (useType == USE_ON);
}

void CLavaGod::EruptThink()
{
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(m_flMinDropFreq, m_flMaxDropFreq);

	if (!m_bActive)
		return;

	Vector vecSpot = pev->origin + Vector(RANDOM_FLOAT(-m_flDropRadius, m_flDropRadius), RANDOM_FLOAT(-m_flDropRadius, m_flDropRadius), 0);

	CBaseEntity* pLava = CBaseEntity::Create("lava_god_ball", vecSpot, g_vecZero, edict());
	if (pLava)
		pLava->pev->velocity = Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(0.5, 1)) * m_flMaxVelocity;
}
