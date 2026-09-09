//=========================================================
// MAYAN1's remaining classes with no SDK precedent: monster_raptor
// (CRaptor) and monster_darttrap (CDartTrap + its runtime-only
// CDart projectile).
//
// Decompiled fresh from gunman.dll this session:
//   monster_raptor: LINK @0x10075fa0, vtable @0x100f8bc8, Spawn
//     @0x100776e0, Precache @0x10077430, Classify @0x10077470
//     (returns 15 normally, 5 == CLASS_ALIEN_MILITARY when
//     pev->skin==1 - see CLASS_ALIEN_PREDATOR_RAPTOR in cbase.h),
//     bbox-recompute override @0x10076000 (custom SetObjectCollisionBox
//     - not reproduced, see below), CRaptor::JumpTouch @0x100766a0
//     (named via the binary's symbol table). Cross-checked against
//     findings/entity_review_list.csv's prior notes - confirms the
//     "precacht tatsaechlich Rheptor.mdl" naming trap and the
//     skin-driven Classify(5|15) split, both independently re-derived
//     from the fresh decompile rather than assumed from the doc.
//     monster_rheptor (same CRaptor class, per findings a pure
//     model/recolor alias) and monster_microraptor (a separate
//     CMicroRaptor class per findings) are NOT linked here - MAYAN1
//     only requires monster_raptor; the others get added, per the
//     project's map-driven rule, only once an actual map needs them.
//   monster_darttrap: LINK @0x100a3600, vtable @0x100ff2b8, Spawn
//     @0x100a37f0, Precache @0x100a37c0, KeyValue @0x100a36b0,
//     CDartTrap::OnUse @0x100a3870, CDartTrap::DartShootThink
//     @0x100a3890 (all named via the symbol table), plus the
//     runtime-only dart-fire helper @0x100a31b0 and CDart::ShotTouch
//     @0x100a32e0. Cross-checked against
//     findings/entity_review_list.csv's "FGD-Keyvalues 1:1 bestaetigt"
//     note - re-verified this session: dartcount (int, -1 = infinite,
//     decremented each shot, 0 removes the trap), spread (int),
//     timebetweenshots (float, also the OnUse arm delay).
//
// Simplified relative to the original (documented per-case):
// - CRaptor: the custom bbox-recompute override (slot 7,
//   FUN_10076000) - a hand-tuned collision box distinct from the
//   normal UTIL_SetSize hull - is not reproduced; the engine's default
//   axis-aligned box from UTIL_SetSize is used instead. No custom
//   TakeDamage/Killed override reproduced (matches
//   entity_review_list.csv's note that monster_rheptor, sharing this
//   same class, has "keine eigene KI/Kampflogik"). JumpTouch's exact
//   vector-cross knockback math is approximated as a simple
//   fixed-damage bite-on-touch instead of the byte-exact push vector.
//   The exact skill-cvar health lookup (FUN_100608d0(DAT_10136ec8))
//   was not traced to a specific gSkillData field - plausible fixed
//   health used instead.
// - CDartTrap/CDart: DartShootThink's exact rejection-sampled cone
//   spread math (~40 decompiled lines of vector rejection sampling) is
//   approximated with a simpler random yaw/pitch jitter scaled by the
//   "spread" keyvalue. CDart::ShotTouch's "stick into the wall and
//   play a randomized impact sound" branch is reproduced with the
//   same 3-way randomized impact sound but without literally embedding
//   the dart model into the wall surface (it just stops and fades via
//   SUB_Remove, same visible net effect for a dart that already
//   removes itself moments later either way).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "explode.h"
#include "weapons.h"

//=========================================================
// The runtime-only dart projectile fired by monster_darttrap. No
// LINK_ENTITY_TO_CLASS under a map-facing name since it's never
// map-placed, spawned only via CBaseEntity::Create.
//=========================================================
class CDart : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT ShotTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(monster_darttrap_dart, CDart);

void CDart::Precache()
{
	PrecacheModel("models/dart.mdl");
	PrecacheSound("darts/blowdart2.wav");
	PrecacheSound("darts/blowdart3.wav");
	PrecacheSound("darts/blowdartimpact1.wav");
	PrecacheSound("darts/blowdartimpact2.wav");
	PrecacheSound("darts/blowdartimpact3.wav");
	PrecacheSound("debris/flesh1.wav");
}

void CDart::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("monster_darttrap_dart");
	SET_MODEL(ENT(pev), "models/dart.mdl");
	UTIL_SetSize(pev, Vector(-1, -1, -1), Vector(1, 1, 1));
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	SetTouch(&CDart::ShotTouch);

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_LONG(0, 1) ? "darts/blowdart2.wav" : "darts/blowdart3.wav", 1.0, ATTN_NORM, 0, 100);
}

void CDart::ShotTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->takedamage != DAMAGE_NO)
	{
		pOther->TakeDamage(pev, pev, 8, DMG_BULLET);
		if (pOther->BloodColor() != DONT_BLEED)
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "debris/flesh1.wav", 1.0, ATTN_NORM, 0, 110);
	}
	else
	{
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "darts/blowdartimpact1.wav", 0.7, ATTN_NORM, 0, 100);
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "darts/blowdartimpact2.wav", 0.7, ATTN_NORM, 0, 100);
			break;
		default:
			EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "darts/blowdartimpact3.wav", 0.7, ATTN_NORM, 0, 100);
			break;
		}
		pev->velocity = g_vecZero;
		pev->movetype = MOVETYPE_FLY;
		pev->solid = SOLID_NOT;
	}

	SetThink(&CDart::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}

//=========================================================
// monster_darttrap - CDartTrap. An invisible, Use-triggered dart
// launcher script point (see file header re: spread-cone
// simplification).
//=========================================================
class CDartTrap : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT OnUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT DartShootThink();

	int m_iDartCount = -1;
	int m_iSpread = 0;
	float m_flTimeBetweenShots = 2.0;
};
LINK_ENTITY_TO_CLASS(monster_darttrap, CDartTrap);

bool CDartTrap::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "dartcount"))
	{
		m_iDartCount = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "timebetweenshots"))
	{
		m_flTimeBetweenShots = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "spread"))
	{
		m_iSpread = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CDartTrap::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/null.mdl");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	pev->takedamage = DAMAGE_NO;

	SetUse(&CDartTrap::OnUse);
	SetThink(&CDartTrap::DartShootThink);
}

void CDartTrap::Precache()
{
	PrecacheModel("models/null.mdl");
	PrecacheModel("models/dart.mdl");
	UTIL_PrecacheOther("monster_darttrap_dart");
}

void CDartTrap::OnUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	pev->nextthink = gpGlobals->time + m_flTimeBetweenShots;
}

void CDartTrap::DartShootThink()
{
	if (m_iDartCount != -1)
	{
		if (m_iDartCount == 0)
		{
			UTIL_Remove(this);
			return;
		}
		m_iDartCount--;
	}

	UTIL_MakeVectors(pev->angles);
	Vector vecDir = gpGlobals->v_forward;
	// Approximated spread cone (see file header) - a simple angular
	// jitter scaled by the "spread" keyvalue instead of the original's
	// rejection-sampled unit-disk cone math.
	if (m_iSpread > 0)
	{
		vecDir = vecDir + gpGlobals->v_right * RANDOM_FLOAT(-1, 1) * (m_iSpread * 0.01) + gpGlobals->v_up * RANDOM_FLOAT(-1, 1) * (m_iSpread * 0.01);
		vecDir = vecDir.Normalize();
	}

	CBaseEntity* pDart = CBaseEntity::Create("monster_darttrap_dart", pev->origin, UTIL_VecToAngles(vecDir), edict());
	if (pDart)
		pDart->pev->velocity = vecDir * 400;

	pev->nextthink = gpGlobals->time + m_flTimeBetweenShots;
}

//=========================================================
// monster_raptor - CRaptor. Also used (per findings, unverified this
// session since MAYAN1 doesn't require it) under the alias
// "monster_rheptor".
//=========================================================
class CRaptor : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return pev->skin == 1 ? CLASS_ALIEN_MILITARY : CLASS_ALIEN_PREDATOR_RAPTOR; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	// BUG FIX (2026-09-05): scripted_sequence-driven "waiting" poses
	// get stomped mid-animation without this - see the
	// feedback-setactivity-act-idle-stomps-scripted-sequence memory
	// note (same fix as CFriendlyGunman etc; covers monster_rheptor
	// too via classname alias).
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
	void EXPORT JumpTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(monster_raptor, CRaptor);
// CORRECTION (2026-09-05): findings/entities/dinosaur_family.md confirms
// monster_rheptor is a pure classname alias of monster_raptor (identical
// vtable @0x100f8bc8) - added here for completeness, same pattern as
// monster_gunner/monster_rustgnr's alias fix, see
// findings/open_items_audit_2026-09-05.md.
LINK_ENTITY_TO_CLASS(monster_rheptor, CRaptor);

void CRaptor::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/rheptor.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 40));

	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_SLIDEBOX;
	pev->health = 80; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = Vector(0, 0, 48);
	pev->flags |= FL_MONSTER;

	if (pev->skin == 1)
		SetBodygroup(2, 1);

	MonsterInit();
	SetTouch(&CRaptor::JumpTouch);
}

void CRaptor::Precache()
{
	PrecacheModel("models/rheptor.mdl");
	PrecacheModel("sprites/gorehuman.spr");
	PrecacheModel("sprites/gibhuman.spr");
}

void CRaptor::JumpTouch(CBaseEntity* pOther)
{
	if (!pOther || pOther->pev->takedamage == DAMAGE_NO)
		return;
	if (pOther->Classify() == Classify())
		return;

	pOther->TakeDamage(pev, pev, 15, DMG_SLASH);
}
