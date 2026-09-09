//=========================================================
// RUST5A's remaining classes with no SDK precedent: trigger_kill
// (CGunmanTriggerKill) and monster_beakbirther (CBeakBirther, the
// "Beak Queen"). weapon_beamgun, RUST5A's third gap, is an
// already-deferred player weapon.
//
// Decompiled fresh from gunman.dll this session:
//   trigger_kill: LINK @0x1005a050, ctor helper @0x1005a100 (vtable
//     @0x100f4c34), Spawn @0x1005a190 (just wires
//     SetUse(&KillUse) - Think/Touch untouched), KeyValue
//     @0x1005a110, CGunmanTriggerKill::KillUse @0x1005a1a0 (found via
//     the binary's own demangled symbol table after an initial wrong
//     guess at "vtable slot 58" turned out to read past the real
//     vtable's end into unrelated neighboring data - a reminder that
//     slot-position assumptions need per-class verification, not
//     copy-paste from a similarly-sized class). Confirms
//     findings/entity_review_list.csv's description exactly: Use-
//     triggered (no Touch), finds its target by the stock "target"
//     keyvalue, checks the target isn't already dead
//     (pev->takedamage != DAMAGE_NO), then calls TakeDamage on it
//     with damage = the target's own current health + 1 (guaranteed
//     lethal regardless of the target's actual health) and
//     bitsDamageType = DMG_CRUSH | (shouldgib ? DMG_ALWAYSGIB : 0) -
//     the confirmed "shouldgib" keyvalue maps directly onto the
//     stock DMG_ALWAYSGIB bit (0x2000), not a Gunman-custom flag.
//   monster_beakbirther: LINK @0x100c1d80, vtable @0x10102f34, Spawn
//     @0x100c21c0, Precache @0x100c1ed0, Classify @0x100c21b0
//     (constant 5 = CLASS_ALIEN_MILITARY). Confirmed bbox
//     (-10,-10,-10)/(10,10,10), MOVETYPE_FLY, SOLID_SLIDEBOX, health
//     150. Matches findings/entity_review_list.csv's "Nicht in FGD
//     platzierbar, aber in 3 Retail-Maps hart verdrahtet" note (no
//     custom KeyValue found here either - falls through to the
//     shared generic CBaseMonster::KeyValue).
//
// Simplified relative to the original: CBeakBirther's actual
// Beak-spawning behavior (per findings, the in-game "Hopper Cocoon"
// that spawns monster_beak instances) was not decompiled this
// session - out of scope for a secondary/setpiece creature at this
// map-driven pass (same simplification level as CTube's undecompiled
// homing-launcher attack elsewhere in this project); relies on
// default CBaseMonster AI beyond the confirmed Spawn/Precache/
// Classify behavior.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

//=========================================================
// trigger_kill - CGunmanTriggerKill.
//=========================================================
#define SF_TRIGGERKILL_SHOULDGIB DMG_ALWAYSGIB

class CGunmanTriggerKill : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT KillUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
};
LINK_ENTITY_TO_CLASS(trigger_kill, CGunmanTriggerKill);

bool CGunmanTriggerKill::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "shouldgib"))
	{
		if (atoi(pkvd->szValue) != 0)
			pev->spawnflags |= SF_TRIGGERKILL_SHOULDGIB;
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CGunmanTriggerKill::Spawn()
{
	SetUse(&CGunmanTriggerKill::KillUse);
}

void CGunmanTriggerKill::KillUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	CBaseEntity* pTarget = UTIL_FindEntityByTargetname(nullptr, STRING(pev->target));
	if (!pTarget || pTarget->pev->takedamage == DAMAGE_NO)
		return;

	CBaseEntity* pAttacker = pActivator ? pActivator : this;
	// Confirmed via decompile: damage = target's own current health + 1
	// (guaranteed lethal regardless of magnitude), bitsDamageType =
	// DMG_CRUSH | the confirmed shouldgib->DMG_ALWAYSGIB mapping.
	pTarget->TakeDamage(pAttacker->pev, pAttacker->pev, pTarget->pev->health + 1, DMG_CRUSH | (pev->spawnflags & SF_TRIGGERKILL_SHOULDGIB));
}

//=========================================================
// monster_beakbirther - CBeakBirther ("Beak Queen").
//=========================================================
class CBeakBirther : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_MILITARY; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	// Nachtrag 2026-09-06: models/beakqueengib.mdl war schon vorher
	// precacht, aber nie in GibMonster() verwendet - siehe
	// CBaseMonster::CustomGibModel() in basemonster.h.
	const char* CustomGibModel() override { return "models/beakqueengib.mdl"; }
};
LINK_ENTITY_TO_CLASS(monster_beakbirther, CBeakBirther);

void CBeakBirther::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/beakqueen.mdl");
	UTIL_SetSize(pev, Vector(-10, -10, -10), Vector(10, 10, 10));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->health = 150;

	MonsterInit();
}

void CBeakBirther::Precache()
{
	PrecacheModel("models/beakqueen.mdl");
	PrecacheModel("models/beakqueengib.mdl");
	PrecacheModel("models/beakbryo.mdl");
	PrecacheModel("sprites/gorebirth.spr");
	PrecacheModel("sprites/gibbirth.spr");

	PrecacheSound("beak/qbroar1.wav");
	PrecacheSound("beak/qbroar2.wav");
	PrecacheSound("beak/qbroar3.wav");
	// Nachtrag 2026-09-06 (In-Game-Test via monster_zoo_test): "beak/
	// qbdie.wav" existiert nicht im retail-Sound-Ordner - vorhanden
	// sind nur die generischen "beak/die1.wav"/"die2.wav" (auch von
	// monster_beak genutzt), kein eigener "qb"-Todessound fuer die
	// Beak-Birther/-Queen. Auf die naechstliegende echte Datei
	// umgestellt (dokumentierte Annahme, kein RE-Fund).
	PrecacheSound("beak/die1.wav");
}
