//=========================================================
// cinematic2's targ_speaker (CTargSpeaker) - the third of Gunman's
// three sound-entity variants (alongside stock speaker/CSpeaker and
// this project's own player_speaker/random_speaker in
// gunman_custom_entities.cpp). FGD title "targeted Time Wav Player".
//
// Decompiled fresh from gunman.dll: LINK @0x10066910, ctor helper
// @0x100669c0 (vtable @0x100f6508), Spawn @0x10066a70,
// CTargSpeaker::Use @0x100669e0, KeyValue @0x10066a00. Single
// keyvalue "tsnoise" (the sound file to play). Spawn removes itself
// with an ALERT if no sound was assigned (confirmed error string
// "NO_SOUND_ASSIGNED_TO_TARG_SPEAKER" in the binary). Use() is a
// simple one-shot EMIT_SOUND, no toggle/loop state unlike stock
// CSpeaker.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"

class CTargSpeaker : public CBaseEntity
{
public:
	bool KeyValue(KeyValueData* pkvd) override;
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	int m_iszNoise = 0;
};
LINK_ENTITY_TO_CLASS(targ_speaker, CTargSpeaker);

TYPEDESCRIPTION CTargSpeaker::m_SaveData[] =
	{
		DEFINE_FIELD(CTargSpeaker, m_iszNoise, FIELD_STRING),
	};

IMPLEMENT_SAVERESTORE(CTargSpeaker, CBaseEntity);

bool CTargSpeaker::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "tsnoise"))
	{
		m_iszNoise = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CTargSpeaker::Spawn()
{
	if (FStringNull(m_iszNoise))
	{
		ALERT(at_error, "NO SOUND ASSIGNED TO TARG_SPEAKER at %f %f %f\n", pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove(this);
		return;
	}

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
}

void CTargSpeaker::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, STRING(m_iszNoise), 1.0, ATTN_NORM);
}

//=========================================================
// monster_rustbot - CRustbot. A hostile combat robot with a melee arm
// swing and a continuous, tracking damage-over-time eye beam
// ("weapon_egon-like", per the user description that
// findings/entities/rustbot_combat.md confirmed against the
// decompile).
//
// Decompiled fresh from gunman.dll this session: LINK @0x100b7e30,
// vtable @0x10101d9c, Spawn @0x100b91c0, Precache @0x100b8010,
// Classify @0x100b7e90 (constant 7 = CLASS_ALIEN_MONSTER),
// HandleAnimEvent @0x100b8e50 (vtable slot 58, confirms
// findings/entities/rustbot_combat.md's event table exactly - this
// session independently re-derived event 1's melee traceline
// structure from the decompile rather than trusting the doc alone).
// Confirmed: model "models/rustbot.mdl", bbox (-32,-32,0)/(32,32,64),
// MOVETYPE_STEP, SOLID_SLIDEBOX, health is 100 with 50% probability
// or RANDOM(109,116) otherwise (matches the doc's "identisches
// Zufallsmuster wie monster_human_demoman" note), view_ofs=(0,0,40).
// The "borgRustbot" debug spawnflag/log (pev->effects & 0x40) is a
// cut-content developer flag with no findings-documented gameplay
// effect - not reproduced.
//
// Simplified relative to the original (documented per findings'
// still-open points and this session's own scope decision): the
// real eye beam (events 2/3) is a hand-built two-sprite +
// dynamic-light construction, live-repositioned and damage-ticked
// every frame via a dedicated per-tick function
// (FUN_100b8400, not fully decompiled this session). Reproduced here
// instead with the SDK's existing CBeam class (same approach
// egon.cpp already uses for a very similar continuous tracking
// laser) - a single beam entity, retargeted and damage-ticked each
// think, instead of the original's 2-sprite+dlight rig. Event 1's
// melee still uses the confirmed 128-unit traceline range and deals
// approximated (not byte-verified) damage. Spark-particle bursts on
// both events are approximated with a generic TE_SPARKS effect
// rather than the original's dedicated debris-spawner helper
// (FUN_100b9c40, shared with CGunner, not decompiled this session).
//=========================================================

#define RUSTBOT_AE_MELEE 1
#define RUSTBOT_AE_BEAM_OFF 2
#define RUSTBOT_AE_BEAM_ON 3

class CRustbot : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_MONSTER; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	// BUG FIX (2026-09-05): scripted_sequence-driven "waiting" poses
	// get stomped mid-animation without this - see the
	// feedback-setactivity-act-idle-stomps-scripted-sequence memory
	// note (same fix as CFriendlyGunman etc; covers CRustbotFriendly
	// too via inheritance).
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
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	void Melee();
	void BeamOn();
	void BeamOff();
	void EXPORT BeamThink();

	CBeam* m_pBeam = nullptr;
};
LINK_ENTITY_TO_CLASS(monster_rustbot, CRustbot);

TYPEDESCRIPTION CRustbot::m_SaveData[] =
	{
		DEFINE_FIELD(CRustbot, m_pBeam, FIELD_CLASSPTR),
};

IMPLEMENT_SAVERESTORE(CRustbot, CBaseMonster);

void CRustbot::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/rustbot.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 64));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = RANDOM_LONG(0, 1) == 0 ? 100 : RANDOM_LONG(109, 116);
	pev->view_ofs = Vector(0, 0, 40);

	MonsterInit();
}

void CRustbot::Precache()
{
	PrecacheModel("models/rustbot.mdl");
	PrecacheModel("sprites/xbeam1.spr");
	PrecacheModel("sprites/rustbeam_end.spr");
}

void CRustbot::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case RUSTBOT_AE_MELEE:
		Melee();
		break;
	case RUSTBOT_AE_BEAM_OFF:
		BeamOff();
		break;
	case RUSTBOT_AE_BEAM_ON:
		BeamOn();
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CRustbot::Melee()
{
	if (!m_hEnemy)
		return;

	if ((m_hEnemy->pev->origin - pev->origin).Length() <= 128)
	{
		UTIL_MakeVectors(pev->angles);
		Vector vecSrc = pev->origin + gpGlobals->v_forward * 16 + Vector(0, 0, 40);

		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSrc);
		WRITE_BYTE(TE_SPARKS);
		WRITE_COORD(vecSrc.x);
		WRITE_COORD(vecSrc.y);
		WRITE_COORD(vecSrc.z);
		MESSAGE_END();

		m_hEnemy->TakeDamage(pev, pev, 15, DMG_CLUB);
	}
}

void CRustbot::BeamOn()
{
	BeamOff();

	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPARKS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z + 40);
	MESSAGE_END();

	m_pBeam = CBeam::BeamCreate("sprites/xbeam1.spr", 30);
	if (m_pBeam)
	{
		m_pBeam->PointsInit(pev->origin, pev->origin);
		m_pBeam->SetColor(80, 160, 255);
		m_pBeam->SetBrightness(200);
		m_pBeam->SetNoise(0);
		m_pBeam->pev->owner = edict();

		SetThink(&CRustbot::BeamThink);
		pev->nextthink = gpGlobals->time;
	}
}

void CRustbot::BeamOff()
{
	if (m_pBeam)
	{
		UTIL_Remove(m_pBeam);
		m_pBeam = nullptr;
	}

	// BeamOn temporarily replaces the regular monster think callback.  Always
	// restore it here; otherwise a finished beam leaves the rustbot running an
	// inert BeamThink every 100 ms and prevents its normal AI from resuming.
	SetThink(&CBaseMonster::CallMonsterThink);
	pev->nextthink = gpGlobals->time;
}

void CRustbot::BeamThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (!m_pBeam || !m_hEnemy)
	{
		BeamOff();
		return;
	}

	Vector vecSrc = pev->origin + Vector(0, 0, 40);
	Vector vecEnd = m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

	m_pBeam->SetStartPos(vecSrc);
	m_pBeam->SetEndPos(tr.vecEndPos);

	if (tr.flFraction < 1.0 && tr.pHit)
	{
		CBaseEntity* pHit = CBaseEntity::Instance(tr.pHit);
		if (pHit && pHit->pev->takedamage != DAMAGE_NO)
			pHit->TakeDamage(pev, pev, 2, DMG_ENERGYBEAM);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "buttons/spark6.wav", 0.5, ATTN_NORM, 0, 100);
	}
}

void CRustbot::Killed(entvars_t* pevAttacker, int iGib)
{
	BeamOff();
	CBaseMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// monster_rustbot_friendly - CRustbotFriendly (rust7d+). Confirmed
// fresh this session (LINK @0x100b9d30, vtable @0x10101fd0): Spawn
// and Precache are the exact same addresses as monster_rustbot's own
// (0x100b91c0/0x100b8010). Classify (slot 8, @0x100b9db0) is the
// only override and returns the literal constant 3
// (CLASS_HUMAN_PASSIVE), matching monster_rustbit_friendly's
// identical pattern (see rust1_misc.cpp).
//=========================================================
class CRustbotFriendly : public CRustbot
{
public:
	int Classify() override { return CLASS_HUMAN_PASSIVE; }
};
LINK_ENTITY_TO_CLASS(monster_rustbot_friendly, CRustbotFriendly);

//=========================================================
// lightning_bug - CLightningBug. Homing sub-projectile spawned
// periodically by ball_lightning's own MoveThink (see the
// ball_lightning section below for the full decompile evidence).
// Reuses this project's established CDmlRocket::TrackTarget-style
// simplified homing rather than the original's exact charge-tier
// damage table (see file header note below).
//=========================================================
class CLightningBug : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT TrackTargetBug();
	void EXPORT DieTouchBug(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(lightning_bug, CLightningBug);

void CLightningBug::Precache()
{
	PrecacheModel("sprites/flyball.spr");
}

void CLightningBug::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("lightning_bug");
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 180;
	pev->scale = 0.5;
	SET_MODEL(ENT(pev), "sprites/flyball.spr");
	UTIL_SetSize(pev, Vector(-2, -2, -2), Vector(2, 2, 2));
	pev->dmg = 25; // approximated mid-tier of the confirmed 25/75/100/~130 charge table, see file header

	SetTouch(&CLightningBug::DieTouchBug);
	SetThink(&CLightningBug::TrackTargetBug);
	pev->nextthink = gpGlobals->time + 0.1;

	pev->dmgtime = gpGlobals->time + 4.0; // self-expiry, no original timing decompiled
}

void CLightningBug::TrackTargetBug()
{
	if (gpGlobals->time >= pev->dmgtime)
	{
		UTIL_Remove(this);
		return;
	}
	pev->nextthink = gpGlobals->time + 0.1;

	if (!m_hEnemy || !m_hEnemy->IsAlive())
		return;

	Vector vecToTarget = (m_hEnemy->pev->origin - pev->origin).Normalize();
	float flSpeed = pev->velocity.Length();
	if (flSpeed < 1.0f)
		flSpeed = 400.0f;

	Vector vecNewDir = (pev->velocity.Normalize() * 0.85 + vecToTarget * 0.15).Normalize();
	pev->velocity = vecNewDir * flSpeed;
	pev->angles = UTIL_VecToAngles(pev->velocity);
}

void CLightningBug::DieTouchBug(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->takedamage != DAMAGE_NO)
		pOther->TakeDamage(pev, pev, pev->dmg, DMG_SHOCK);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPARKS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	MESSAGE_END();

	UTIL_Remove(this);
}

//=========================================================
// Shared target-picker for ball_lightning's lightning_bug spawns and
// weapon_beamgun's tracer volley (see both sections below and
// beamgun.cpp): if the firer is a CBaseMonster (e.g.
// monster_rustflier), reuse its own m_hEnemy; if it's the player
// (weapon_beamgun has no tracked "enemy" field in this SDK),
// approximate by picking the nearest living monster within range - a
// documented simplification, not decompiled. Not declared static
// since beamgun.cpp reuses it (extern-declared there).
//=========================================================
CBaseEntity* FindLightningBugTarget(CBaseEntity* pOwner, const Vector& vecOrigin)
{
	if (pOwner)
	{
		CBaseMonster* pMonster = pOwner->MyMonsterPointer();
		if (pMonster && pMonster->m_hEnemy && pMonster->m_hEnemy->IsAlive())
			return pMonster->m_hEnemy;
	}

	CBaseEntity* pList[8];
	int iCount = UTIL_MonstersInSphere(pList, 8, vecOrigin, 750.0f);
	CBaseEntity* pBest = nullptr;
	float flBestDist = 750.0f * 750.0f;
	for (int i = 0; i < iCount; i++)
	{
		if (pList[i] == pOwner || !pList[i]->IsAlive())
			continue;
		float flDist = (pList[i]->pev->origin - vecOrigin).LengthSquared();
		if (flDist < flBestDist)
		{
			flBestDist = flDist;
			pBest = pList[i];
		}
	}
	return pBest;
}

//=========================================================
// ball_lightning - CBallLightning. The runtime-only ranged attack
// projectile fired by monster_rustflier's HuntThink (per
// findings/entities/rustflier_family.md, also independently fired by
// the player weapon_beamgun's overheat mode - out of scope here, not
// yet implemented). No LINK-time map placement; spawned only via
// CBaseEntity::Create. Not individually re-decompiled this session
// (the doc's confirmation that CRustFlier::HuntThink calls a generic
// ball_lightning spawn helper was enough to justify a standard
// runtime-projectile implementation, same pattern as
// vehicle_tank_rocket/monster_targetrocket_proj elsewhere in this
// project) - straight-line flight, no homing.
//
// CORRECTION (2026-09-05): findings/entities/code_annahme_final_batch.md's
// "tracer/lightning_bug-Verhältnis" open point resolved by fresh
// decompile this session. `FUN_100812d0` (the "lightning_bug"
// constructor, vtable 0x100fa728) is called from
// **`CBallLightning::MoveThink`** (confirmed via xref) - i.e.
// ball_lightning periodically spawns homing "lightning_bug"
// sub-projectiles during its own flight, not a separate/unrelated
// system. `lightning_bug`'s own ctor (`FUN_10080740`) confirms
// Think=`CLightningBug::TrackTargetBug`, Touch=`CLightningBug::DieTouchBug`,
// and a 4-tier charge-scaled damage/speed table (param_1[0xa4] 1-4,
// confirmed float damage constants 25/75/100/~130 across the
// branches). Reproduced here as: `CBallLightning::MoveThink` (new,
// replaces the previous plain `SUB_Remove`-after-5s Think) fires one
// `CLightningBug` every ~0.5s toward the ball's current target (see
// `FindLightningBugTarget()` below) for as long as it has a valid
// target and hasn't yet hit anything; `CLightningBug` reuses this
// project's established `CDmlRocket::TrackTarget`-style simplified
// homing (steer a fraction toward the target's current position each
// tick, not the exact original lead/blend math) rather than the exact
// charge-tier table, which is approximated with a flat mid-tier
// damage/speed instead of the 4-way branch.
//=========================================================
class CBallLightning : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT BoltTouch(CBaseEntity* pOther);
	void EXPORT MoveThink();
};
LINK_ENTITY_TO_CLASS(ball_lightning, CBallLightning);

void CBallLightning::Precache()
{
	PrecacheModel("sprites/flyball.spr");
	UTIL_PrecacheOther("lightning_bug");
}

void CBallLightning::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("ball_lightning");
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 200;
	SET_MODEL(ENT(pev), "sprites/flyball.spr");
	UTIL_SetSize(pev, Vector(-4, -4, -4), Vector(4, 4, 4));
	SetTouch(&CBallLightning::BoltTouch);

	pev->dmgtime = gpGlobals->time + 5.0; // reused as this ball's own expiry deadline
	SetThink(&CBallLightning::MoveThink);
	pev->nextthink = gpGlobals->time + 0.5;
}

void CBallLightning::BoltTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->takedamage != DAMAGE_NO)
		pOther->TakeDamage(pev, pev, 15, DMG_SHOCK);

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPARKS);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	MESSAGE_END();

	UTIL_Remove(this);
}

void CBallLightning::MoveThink()
{
	if (gpGlobals->time >= pev->dmgtime)
	{
		UTIL_Remove(this);
		return;
	}

	pev->nextthink = gpGlobals->time + 0.5;

	CBaseEntity* pOwner = pev->owner ? CBaseEntity::Instance(pev->owner) : nullptr;
	CBaseEntity* pTarget = FindLightningBugTarget(pOwner, pev->origin);
	if (!pTarget)
		return;

	CBaseEntity* pBug = CBaseEntity::Create("lightning_bug", pev->origin, UTIL_VecToAngles(pev->velocity), pev->owner);
	if (pBug)
	{
		pBug->pev->velocity = pev->velocity.Normalize() * 400;
		static_cast<CLightningBug*>(pBug)->m_hEnemy = pTarget;
	}
}

//=========================================================
// monster_rustflier - CRustFlier. A flying, turbine-driven robot with
// a shield-buffer damage mechanic and a scripted crash-death sequence
// (see findings/entities/rustflier_family.md - a thorough, multi-
// session doc whose Spawn/Precache/Classify/KeyValue/TakeDamage/
// FlierUse/CrashTouch/FlyTouch claims were all independently
// re-confirmed via fresh decompile this session, not taken on faith).
//
// Decompiled fresh: LINK @0x100b2b50, ctor helper @0x100b2c00 (vtable
// @0x101012fc - matches the doc's independently-found vtable value
// exactly), Spawn @0x100b2fa0, Precache @0x100b30f0, Classify
// @0x100b2c80 (constant 8 = CLASS_ALIEN_PREY), KeyValue @0x100b2d70
// (confirms all 6 FGD fields: startshieldstate, targetdamage,
// deathpath, deathangle, timetillcrash, triggerondeath - and their
// FGD-documented defaults, independently reconstructed from the
// ctor's field-init values: shield on, targetdamage 10000, deathangle
// (0,0,0), timetillcrash 10), TakeDamage @0x100b5270 (vtable slot 10),
// FlierUse @0x100b4fe0, CrashTouch @0x100b3290, FlyTouch @0x100b31b0
// (all confirmed byte-for-byte matching the doc's descriptions).
//
// Confirmed bbox (-64,-64,0)/(64,64,64) (matches the FGD exactly -
// the doc's "nicht verifiziert" bbox open point is now resolved),
// MOVETYPE_FLY, SOLID_BBOX, pev->takedamage forced to DAMAGE_YES.
// TakeDamage's real logic (re-derived from the fresh decompile, not
// just summarized from the doc): pev->health itself doubles as the
// "still alive" gate, NOT a depleting shield counter - any single hit
// below the targetdamage threshold is pure cosmetic absorption (a
// random orange renderfx flash + one of 3 impact sounds, no HP loss
// at all), while a single hit AT OR ABOVE the targetdamage threshold
// (while pev->health > 0) immediately triggers the crash sequence.
// This is a "must land one huge hit" boss mechanic, not a drainable
// shield bar. FlierUse is confirmed as a two-press debug/cheat
// toggle (first press logs "shield down", second deletes the
// entity), not a real gameplay mechanic - reproduced faithfully
// anyway since it's cheap to.
//
// Simplified relative to the original (documented per-case):
// HuntThink's real target-acquisition/line-of-sight math
// (FUN_100b4d70) and CrashingThink's exact spark-particle timing are
// not reproduced - replaced with a straightforward "if enemy visible,
// fire ball_lightning periodically" Think and a simplified crash
// sequence (fly toward the deathpath path_corner over timetillcrash
// seconds, switch to the crashed model, fire triggerondeath, explode
// and remove) that preserves every FGD-documented behavior (deathpath
// navigation, timetillcrash timing, deathangle final orientation,
// triggerondeath firing) without the original's exact per-tick spark
// choreography.
//=========================================================

#define SF_RUSTFLIER_START_SHIELD_OFF 1 // not FGD-documented as a spawnflag; shield state comes from startshieldstate

class CRustFlier : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	int Classify() override { return CLASS_ALIEN_PREY; }
	bool TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT FlyTouch(CBaseEntity* pOther);
	void EXPORT CrashTouch(CBaseEntity* pOther);
	void EXPORT HuntThink();
	void EXPORT CrashingThink();
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

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	bool m_bShieldUp = true;			  // "startshieldstate"
	float m_flTargetDamage = 10000;	  // "targetdamage"
	int m_iszDeathPath = 0;			  // "deathpath"
	Vector m_vecDeathAngle = g_vecZero;  // "deathangle"
	float m_flTimeTillCrash = 10;		  // "timetillcrash"
	int m_iszTriggerOnDeath = 0;		  // "triggerondeath"

	float m_flNextAttack = 0;
	bool m_bCrashing = false;
	Vector m_vecCrashGoal = g_vecZero;
	float m_flCrashStartTime = 0;
};
LINK_ENTITY_TO_CLASS(monster_rustflier, CRustFlier);

TYPEDESCRIPTION CRustFlier::m_SaveData[] =
	{
		DEFINE_FIELD(CRustFlier, m_bShieldUp, FIELD_BOOLEAN),
		DEFINE_FIELD(CRustFlier, m_bCrashing, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CRustFlier, CBaseMonster);

bool CRustFlier::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "startshieldstate"))
	{
		m_bShieldUp = atoi(pkvd->szValue) != 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "targetdamage"))
	{
		m_flTargetDamage = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "deathpath"))
	{
		m_iszDeathPath = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "deathangle"))
	{
		UTIL_StringToVector((float*)m_vecDeathAngle, pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "timetillcrash"))
	{
		m_flTimeTillCrash = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "triggerondeath"))
	{
		m_iszTriggerOnDeath = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

void CRustFlier::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/rustflyer.mdl");
	UTIL_SetSize(pev, Vector(-64, -64, 0), Vector(64, 64, 64));

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->takedamage = DAMAGE_YES;
	pev->health = 1; // doubles as the "not yet crashed" gate, see file header
	pev->view_ofs = g_vecZero;

	MonsterInit();
	SetThink(&CRustFlier::HuntThink);
	SetTouch(&CRustFlier::FlyTouch);
	pev->nextthink = gpGlobals->time + 1.0;
}

void CRustFlier::Precache()
{
	PrecacheModel("models/rustflyer.mdl");
	PrecacheModel("models/rustflyer_crashed.mdl");
	PrecacheModel("sprites/flyball.spr");
	PrecacheModel("sprites/white.spr");
	PrecacheModel("sprites/fexplo.spr");
	PrecacheModel("models/metalplategibs_green.mdl");
	UTIL_PrecacheOther("ball_lightning");

	PrecacheSound("flier/ap_rotor2.wav");
	PrecacheSound("flier/ap_whine1.wav");
	PrecacheSound("flier/flyer_sick.wav");
	PrecacheSound("flier/field_impact1.wav");
	PrecacheSound("flier/field_impact2.wav");
	PrecacheSound("flier/field_impact3.wav");
}

bool CRustFlier::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	if (flDamage >= m_flTargetDamage && pev->health > 0)
	{
		m_bCrashing = true;
		pev->health = 0;

		CBaseEntity* pGoal = m_iszDeathPath ? UTIL_FindEntityByTargetname(nullptr, STRING(m_iszDeathPath)) : nullptr;
		m_vecCrashGoal = pGoal ? pGoal->pev->origin : pev->origin;
		m_flCrashStartTime = gpGlobals->time;

		SetTouch(&CRustFlier::CrashTouch);
		SetThink(&CRustFlier::CrashingThink);
		pev->nextthink = gpGlobals->time;
		return false;
	}

	if (m_bShieldUp)
	{
		pev->renderfx = kRenderFxDistort;
		pev->rendercolor = Vector(255, 150, 0);

		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "flier/field_impact1.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG(80, 120));
			break;
		case 1:
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "flier/field_impact2.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG(80, 120));
			break;
		default:
			EMIT_SOUND_DYN(ENT(pev), CHAN_STATIC, "flier/field_impact3.wav", 1.0, ATTN_NORM, 0, RANDOM_LONG(80, 120));
			break;
		}
	}

	return false;
}

void CRustFlier::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Confirmed debug/cheat toggle, not a real gameplay mechanic (see
	// file header): first press drops the shield, second deletes the
	// flier outright.
	if (m_bShieldUp)
	{
		m_bShieldUp = false;
		ALERT(at_console, "Flier shield down!\n");
		return;
	}

	ALERT(at_console, "Deleting Flier!\n");
	UTIL_Remove(this);
}

void CRustFlier::FlyTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->movetype == MOVETYPE_FLY)
	{
		Vector vecPush = (pev->origin - pOther->pev->origin).Normalize();
		float flSpeed = pev->velocity.Length() + 10;
		pev->velocity = pev->velocity + vecPush * flSpeed;
	}
}

void CRustFlier::CrashTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->movetype == MOVETYPE_FLY)
	{
		pev->nextthink = gpGlobals->time;
	}
}

void CRustFlier::HuntThink()
{
	pev->nextthink = gpGlobals->time + 0.2;

	if (!m_hEnemy)
		return;

	if (m_flNextAttack > gpGlobals->time)
		return;
	m_flNextAttack = gpGlobals->time + 2.0;

	UTIL_MakeVectors(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_forward * 32;
	Vector vecDir = (m_hEnemy->pev->origin - vecSrc).Normalize();

	CBaseEntity* pBolt = CBaseEntity::Create("ball_lightning", vecSrc, UTIL_VecToAngles(vecDir), edict());
	if (pBolt)
		pBolt->pev->velocity = vecDir * 500;
}

void CRustFlier::CrashingThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	Vector vecDir = (m_vecCrashGoal - pev->origin);
	float flDist = vecDir.Length();
	if (flDist > 8)
	{
		float flTimeLeft = m_flTimeTillCrash - (gpGlobals->time - m_flCrashStartTime);
		if (flTimeLeft < 0.1)
			flTimeLeft = 0.1;
		pev->velocity = vecDir.Normalize() * (flDist / flTimeLeft);
	}

	if (RANDOM_LONG(0, 4) == 0)
	{
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_SPARKS);
		WRITE_COORD(pev->origin.x + RANDOM_FLOAT(-16, 16));
		WRITE_COORD(pev->origin.y + RANDOM_FLOAT(-16, 16));
		WRITE_COORD(pev->origin.z + RANDOM_FLOAT(-16, 16));
		MESSAGE_END();
	}

	if (flDist <= 8 || gpGlobals->time - m_flCrashStartTime >= m_flTimeTillCrash)
	{
		pev->velocity = g_vecZero;
		pev->angles = m_vecDeathAngle;
		SET_MODEL(ENT(pev), "models/rustflyer_crashed.mdl");

		MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
		WRITE_BYTE(TE_EXPLOSION);
		WRITE_COORD(pev->origin.x);
		WRITE_COORD(pev->origin.y);
		WRITE_COORD(pev->origin.z);
		WRITE_SHORT(g_sModelIndexFireball);
		WRITE_BYTE(40);
		WRITE_BYTE(15);
		WRITE_BYTE(TE_EXPLFLAG_NONE);
		MESSAGE_END();
		// Nachtrag 2026-09-06 (Nutzerwunsch: Erschuetterung+Lichtblitz bei
		// jeder Explosion, siehe util.h/UTIL_ExplosionEffects).
		UTIL_ExplosionEffects(pev->origin, 250.0f);

		RadiusDamage(pev->origin, pev, pev, 60, CLASS_NONE, DMG_BLAST);

		if (m_iszTriggerOnDeath)
			FireTargets(STRING(m_iszTriggerOnDeath), this, this, USE_TOGGLE, 0);

		SetThink(&CBaseEntity::SUB_Remove);
		pev->nextthink = gpGlobals->time + 2.0;
	}
}
