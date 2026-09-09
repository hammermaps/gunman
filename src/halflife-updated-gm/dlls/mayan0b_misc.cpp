//=========================================================
// MAYAN0B's remaining classes with no SDK precedent: random_trigger
// (CRand_Trigger), monster_hatchetfish (CHatchetFish), monster_tank
// (CScriptedTank) + its runtime-only monster_tank_rocket projectile
// (CScriptedTankRocket). (monster_sentry_mini went into turret.cpp
// as CMiniSentry, alongside its CBaseTurret/CSentry relatives.)
//
// Decompiled fresh from gunman.dll this session:
//   random_trigger: LINK @0x10066b20, ctor helper @0x10066bd0
//     (vtable @0x100f65f0), Spawn @0x10066d70, KeyValue @0x10066c00,
//     CRand_Trigger::Rand_TriggerUse @0x10066dc0,
//     CRand_Trigger::Rand_TriggerThink @0x10066de0. Cross-checked
//     against findings/entity_review_list.csv's prior summary
//     (matches exactly: delay/random_min/random_max/start_state
//     keyvalues, Rand_TriggerThink/Use names) but re-verified by
//     decompiling all 4 functions again rather than trusting the doc.
//   monster_hatchetfish: LINK @0x10072200, vtable @0x100f82fc, Spawn
//     @0x10072410, Precache @0x10072800, Classify @0x100723a0
//     (returns 17 = CLASS_INSECT_CRICKET, matches
//     entity_review_list.csv's cross-reference to monster_cricket),
//     BloodColor @0x10072390 (returns -1 = DONT_BLEED),
//     CHatchetFish::SwimThink @0x10073380 (named via symbol table).
//   monster_tank: LINK @0x100a5a10, vtable @0x100ffb80, Spawn
//     @0x100a5a60, Precache @0x100a5be0. No demangled method names in
//     the binary for this class (Ghidra never resolved a class name
//     for it - matches findings' "Literal nicht 100%" caveat).
//     Confirmed: model "models/script_tank.mdl", bbox
//     (-16,-16,0)/(16,16,74), SOLID_SLIDEBOX, MOVETYPE_STEP, health
//     8.0, pev->takedamage explicitly cleared to DAMAGE_NO at the end
//     of Spawn (this tank is invulnerable - a scripted/cutscene prop,
//     not a real combat target, consistent with it being cheap
//     8.0 "health" that's never actually checked). The FGD/findings
//     doc's claimed "247 HP" does not match pev->health in the
//     decompile; the literal 247 (0xf7) does appear in Spawn, but at
//     a CScriptedTank-private member offset, not pev->health - likely
//     a default sequence/animation index instead, not reproduced
//     here (open question, not chased further).
//
// Simplified relative to the original (documented per-case):
// - CHatchetFish: Spawn bounds/FOV, 0.1-s SwimThink cadence, the
//   facing-gated bite event and the 25-unit TraceAttack knockback are
//   reproduced. The exact skill-cvar health and the complex DeadThink
//   schedule branch remain open.
// - CScriptedTank: event IDs 1/2 now launch the rocket exactly where
//   the animation requests it; no autonomous fire timer remains. The
//   rocket uses its retail 700-unit homing loop. The proprietary fire
//   event's additional area-effect payload remains undocumented.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "saverestore.h"
#include "effects.h"
#include "explode.h"
#include "weapons.h"

//=========================================================
// random_trigger - CRand_Trigger. A trigger_relay equivalent with
// randomized retrigger timing. Use() toggles the enabled/disabled
// state; while enabled, Think fires its targets on a
// wait + RANDOM(random_min, random_max) cycle.
//=========================================================
class CRand_Trigger : public CBaseEntity
{
public:
	void Spawn() override;
	bool KeyValue(KeyValueData* pkvd) override;
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT Rand_TriggerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT Rand_TriggerThink();

	bool m_bState = false;
	float m_fBaseTime = 5.0;
	float m_fMinAddTime = 5.0;
	float m_fMaxAddTime = 10.0;
};
LINK_ENTITY_TO_CLASS(random_trigger, CRand_Trigger);

TYPEDESCRIPTION CRand_Trigger::m_SaveData[] =
	{
		DEFINE_FIELD(CRand_Trigger, m_fMaxAddTime, FIELD_FLOAT),
		DEFINE_FIELD(CRand_Trigger, m_fMinAddTime, FIELD_FLOAT),
		DEFINE_FIELD(CRand_Trigger, m_fBaseTime, FIELD_FLOAT),
		DEFINE_FIELD(CRand_Trigger, m_bState, FIELD_BOOLEAN),
	};

IMPLEMENT_SAVERESTORE(CRand_Trigger, CBaseEntity);

bool CRand_Trigger::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "start_state"))
	{
		m_bState = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "wait"))
	{
		m_fBaseTime = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "random_max"))
	{
		m_fMaxAddTime = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "random_min"))
	{
		m_fMinAddTime = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CRand_Trigger::Spawn()
{
	SetUse(&CRand_Trigger::Rand_TriggerUse);
	SetThink(&CRand_Trigger::Rand_TriggerThink);
	pev->solid = SOLID_NOT;

	pev->nextthink = gpGlobals->time + m_fBaseTime + RANDOM_FLOAT(m_fMinAddTime, m_fMaxAddTime);
}

void CRand_Trigger::Rand_TriggerUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_bState = !m_bState;
}

void CRand_Trigger::Rand_TriggerThink()
{
	if (!m_bState)
	{
		pev->nextthink = gpGlobals->time + 1.0;
		return;
	}

	FireTargets(STRING(pev->target), this, this, USE_TOGGLE, 0);
	pev->nextthink = gpGlobals->time + m_fBaseTime + RANDOM_FLOAT(m_fMinAddTime, m_fMaxAddTime);
}

//=========================================================
// monster_hatchetfish - CHatchetFish (see file header re: SwimThink/
// TraceAttack/DeadThink simplification).
//=========================================================
class CHatchetFish : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_INSECT_CRICKET; }
	int BloodColor() override { return DONT_BLEED; }
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	void EXPORT SwimThink();
};
LINK_ENTITY_TO_CLASS(monster_hatchetfish, CHatchetFish);

void CHatchetFish::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hatchet.mdl");
	UTIL_SetSize(pev, Vector(-1, -1, 0), Vector(1, 1, 2));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->flags |= FL_SWIM;
	pev->health = 20; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = g_vecZero;
	m_flFieldOfView = -8.0f;

	MonsterInit();
	SetThink(&CHatchetFish::SwimThink);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1.0, 5.0);
}

void CHatchetFish::Precache()
{
	PrecacheModel("models/hatchet.mdl");

	PrecacheSound("hatchet/hatchet_bite1.wav");
	PrecacheSound("hatchet/hatchet_bite2.wav");
	PrecacheSound("hatchet/hatchet_splash1.wav");
	PrecacheSound("hatchet/hatchet_splash2.wav");
	PrecacheSound("hatchet/hatchet_sight1.wav");
	PrecacheSound("hatchet/hatchet_sight2.wav");
}

void CHatchetFish::SwimThink()
{
	// Retail's CHatchetFish::SwimThink runs at the standard 0.1-s AI
	// cadence; the old one-second timer made steering and attack state lag.
	pev->nextthink = gpGlobals->time + 0.1f;

	pev->angles.y += RANDOM_FLOAT(-15.0, 15.0);
	UTIL_MakeVectors(pev->angles);
	pev->velocity = gpGlobals->v_forward * RANDOM_FLOAT(40, 100);
}

void CHatchetFish::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	if (pEvent->event != 1)
	{
		if (pEvent->event != 2)
			CBaseMonster::HandleAnimEvent(pEvent);
		return;
	}

	if (!m_hEnemy)
		return;

	Vector toEnemy = m_hEnemy->pev->origin - pev->origin;
	toEnemy.z = 0;
	if (toEnemy.Length() == 0)
		return;
	toEnemy = toEnemy.Normalize();
	UTIL_MakeVectors(pev->angles);
	// The retail comparison is a forward-facing dot-product gate. Its
	// external constant is not recoverable from the static data image;
	// retain a strict forward arc rather than damaging behind the fish.
	if (DotProduct(gpGlobals->v_forward, toEnemy) > 0.5f)
		m_hEnemy->TakeDamage(pev, pev, 10, DMG_SLASH);
}

void CHatchetFish::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	pev->velocity = g_vecZero;
	if (pevAttacker)
	{
		Vector away = (pev->origin - pevAttacker->origin).Normalize();
		pev->velocity = away * 25.0f; // Retail constant at 0x100ea864.
	}
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================================
// monster_tank - CScriptedTank. An invulnerable scripted/cutscene tank;
// its fire animation dispatches the rocket through events 1 and 2.
//=========================================================
class CScriptedTank : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
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

private:
	void FireRocket();
	int m_iLastAttachment = 2;
};
LINK_ENTITY_TO_CLASS(monster_tank, CScriptedTank);

void CScriptedTank::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/script_tank.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 74));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = 8;
	pev->takedamage = DAMAGE_NO; // confirmed in decompiled Spawn - this tank cannot be damaged

	MonsterInit();
}

void CScriptedTank::Precache()
{
	PrecacheModel("models/script_tank.mdl");
	PrecacheSound("weapons/explode4.wav");
	// Nachtrag 2026-09-06 (In-Game-Test via monster_zoo_test): der
	// vorherige Pfad "tank/tank_engineidle.wav" existiert nicht -
	// echter retail-Pfad ist "Tank/engineidle.wav" (Ordner
	// gross geschrieben, kein "tank_"-Praefix beim Dateinamen).
	PrecacheSound("Tank/engineidle.wav");
}

void CScriptedTank::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	if (pEvent->event == 1 || pEvent->event == 2)
	{
		FireRocket();
		return;
	}
	CBaseMonster::HandleAnimEvent(pEvent);
}

void CScriptedTank::FireRocket()
{
	// The model owns the fire timing. Alternate its two verified cannon
	// attachments to preserve the safe, elevated launch origin.
	Vector vecSrc, vecAnglesUnused;
	GetAttachment(m_iLastAttachment, vecSrc, vecAnglesUnused);
	m_iLastAttachment = (m_iLastAttachment == 2) ? 3 : 2;

	CBaseEntity* pRocket = CBaseEntity::Create("monster_tank_rocket", vecSrc, pev->angles, edict());
	if (pRocket)
	{
		UTIL_MakeVectors(pev->angles);
		Vector vecDir = gpGlobals->v_forward;
		pRocket->pev->velocity = vecDir * 400;
		pRocket->pev->angles = UTIL_VecToAngles(vecDir);
	}
}

//=========================================================
// The runtime-only homing rocket fired by monster_tank. No LINK-time map
// placement; spawned only from the tank's fire animation events.
//=========================================================
class CScriptedTankRocket : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketThink();
	void EXPORT RocketTouch(CBaseEntity* pOther);

private:
	CSprite* m_pFlame = nullptr;
};
LINK_ENTITY_TO_CLASS(monster_tank_rocket, CScriptedTankRocket);

void CScriptedTankRocket::Precache()
{
	PrecacheModel("models/dmlrocket.mdl");
	PrecacheModel("sprites/firebeam.spr");
	PrecacheModel("sprites/white2.spr");
	PrecacheModel("sprites/part2.spr");
	PrecacheModel("sprites/explosion4.spr");
	PrecacheModel("sprites/smokering.spr");
	PrecacheSound("weapons/explode4.wav");
	PrecacheSound("tank/firering.wav");
	PrecacheSound("ambience/flameburst1.wav");
	// Nachtrag 2026-09-06 (Live-Report mit Retail-Referenzscreenshot,
	// siehe CAnimeRocket::Spawn() in rust4a_misc.cpp fuer die volle
	// Begruendung): monster_tank_rocket's reale Precache-Liste
	// (fgd_gap_closure.md) fuehrt "sprites/flame.spr" bereits explizit
	// auf, zusammen mit demselben FUN_10015400-Anhaenge-Helfer wie
	// CAnimeRocket - echter Mechanismus ist eine angehaengte additive
	// env_sprite, kein TE_BEAMFOLLOW-Rauchschweif.
	PrecacheModel("sprites/flame.spr");
}

void CScriptedTankRocket::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("monster_tank_rocket");
	SET_MODEL(ENT(pev), "models/dmlrocket.mdl");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.75f;
	pev->movetype = MOVETYPE_FLY;
	SetTouch(&CScriptedTankRocket::RocketTouch);
	SetThink(&CScriptedTankRocket::RocketThink);
	pev->nextthink = gpGlobals->time + 0.1f;

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "tank/firering.wav", 1.0f, 0.8f, 0, 100);

	m_pFlame = CSprite::SpriteCreate("sprites/flame.spr", pev->origin, true);
	if (m_pFlame)
	{
		m_pFlame->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation);
		m_pFlame->SetScale(1.0);
		m_pFlame->pev->framerate = 15.0f;
		m_pFlame->SetAttachment(edict(), 0);
	}
	pev->effects |= EF_LIGHT;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "ambience/flameburst1.wav", 1.0f, 0.5f, 0, 100);
}

void CScriptedTankRocket::RocketThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	if (pev->origin.x < -4096 || pev->origin.x > 4096 ||
		pev->origin.y < -4096 || pev->origin.y > 4096 ||
		pev->origin.z < -4096 || pev->origin.z > 4096)
	{
		UTIL_Remove(this);
		return;
	}

	if (!m_hEnemy || !m_hEnemy->IsAlive())
	{
		Look(700);
		m_hEnemy = BestVisibleEnemy();
	}

	if (!m_hEnemy)
		return;

	Vector targetDirection = (m_hEnemy->BodyTarget(pev->origin) - pev->origin).Normalize();
	float speed = pev->velocity.Length();
	Vector currentDirection = pev->velocity.Normalize();
	float dot = V_min(1.0f, V_max(-1.0f, DotProduct(currentDirection, targetDirection)));
	float angle = acos(dot) * (180.0f / 3.14159265f);
	if (angle > 20.0f)
		targetDirection = (currentDirection * (1.0f - 20.0f / angle) + targetDirection * (20.0f / angle)).Normalize();
	pev->velocity = targetDirection * speed;
	pev->angles = UTIL_VecToAngles(pev->velocity);
}

void CScriptedTankRocket::RocketTouch(CBaseEntity* pOther)
{
	if (m_pFlame)
	{
		UTIL_Remove(m_pFlame);
		m_pFlame = nullptr;
	}

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(g_sModelIndexFireball);
	WRITE_BYTE(30);
	WRITE_BYTE(15);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
	// Nachtrag 2026-09-06 (Nutzerwunsch: Erschuetterung+Lichtblitz bei
	// jeder Explosion, siehe util.h/UTIL_ExplosionEffects).
	UTIL_ExplosionEffects(pev->origin, 250.0f);

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/explode4.wav", 1.0, ATTN_NORM, 0, 100);
	RadiusDamage(pev->origin, pev, VARS(pev->owner), 60, 250, CLASS_NONE, DMG_BLAST);
	UTIL_Remove(this);
}
