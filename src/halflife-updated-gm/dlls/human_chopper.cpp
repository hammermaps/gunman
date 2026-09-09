//=========================================================
// monster_human_chopper - CChopper. First gap of west6a/west6c in the
// map-driven pass (west3b's monster_human_chopper resolves to the
// same class). CBaseBird-family flying combat boss - see
// findings/entities/CChopper.md and mayan0a_fauna.cpp's CPteradon
// (same family/simplification rationale).
//
// Decompiled fresh from gunman.dll: LINK @0x1006a230, vtable
// @0x100f7388, Spawn @0x1006a320, Precache @0x1006a470. Confirmed
// model "models/chopper.mdl", bbox (-96,-96,-96)/(96,96,96),
// MOVETYPE_FLY, SOLID_BBOX, FL_FLY, takedamage=DAMAGE_YES, health
// from a skill-scaled global (DAT_10136f44, not resolved to a
// concrete constant - approximated as 40), random start skin/frame.
// Sets Think=HuntThink/Touch=FlyTouch directly after sequence/bone setup.
// Its retail death override switches to FallingThink/CrashTouch.
//
// Fire logic (FUN_1006be80) fresh-decompiled: gates on yaw/pitch aim
// tolerance toward the last known enemy position, then fires via the
// shared multi-pellet traceline helper FUN_1000f090 (same helper
// already reproduced as a FireBullets() call for CBanditGrunt's
// MechaVolley and weapon_shotgun) at a confirmed 0.06976 rad spread,
// plays "weapons/hks2.wav" - the same MP5-style weapon sound already
// confirmed for CSentry::Shoot() in the stock SDK base.
//
// Simplified relative to the original: HuntThink's full waypoint/banking
// solver (FUN_1006b560) and its exact yaw/pitch firing gate remain open.
// The previous independent CircleThink and missing crash state were replaced
// by retail-backed target, impact and falling lifecycle behavior.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"

class CChopper : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_MILITARY; }
	int BloodColor() override { return DONT_BLEED; }
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void GibMonster() override;
	void EXPORT HuntThink();
	void EXPORT FlyTouch(CBaseEntity* pOther);
	void EXPORT FallingThink();
	void EXPORT CrashTouch(CBaseEntity* pOther);
	// Nachtrag 2026-09-06: models/choppergibs.mdl war schon vorher
	// precacht, aber nie in GibMonster() verwendet - siehe
	// CBaseMonster::CustomGibModel() in basemonster.h. (Das ebenfalls
	// precachte models/metalplategibs_green.mdl ist ein von mehreren
	// mechanischen Klassen geteiltes generisches Metalltruemmer-Modell,
	// nicht Chopper-spezifisch - hier bewusst nicht verwendet.)
	const char* CustomGibModel() override { return "models/choppergibs.mdl"; }

private:
	void FireAtEnemy();
	int m_iExplosionSprite = 0;
	int m_iMetalGibs = 0;
};
LINK_ENTITY_TO_CLASS(monster_human_chopper, CChopper);

void CChopper::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/chopper.mdl");
	UTIL_SetSize(pev, Vector(-96, -96, -96), Vector(96, 96, 96));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_FLY;
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = 40; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->frame = RANDOM_LONG(0, 255);
	pev->gravity = 2.0f;
	m_flFieldOfView = -0.4f;
	ResetSequenceInfo();
	InitBoneControllers();
	SetThink(&CChopper::HuntThink);
	SetTouch(&CChopper::FlyTouch);
	pev->nextthink = gpGlobals->time + 1.0;
}

void CChopper::Precache()
{
	PrecacheModel("models/chopper.mdl");
	PrecacheModel("models/chopperblade.mdl");
	PrecacheModel("models/choppergibs.mdl");
	m_iMetalGibs = PrecacheModel("models/metalplategibs_green.mdl");
	PrecacheModel("sprites/white.spr");
	m_iExplosionSprite = PrecacheModel("sprites/fexplo.spr");
	PrecacheSound("turret/tu_fire1.wav");
	PrecacheSound("apache/ap_rotor2.wav");
	PrecacheSound("weapons/explode5.wav");
	PrecacheSound("weapons/hks2.wav");
	PrecacheEvent(1, "events/chopperfire.sc");
}

void CChopper::HuntThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance();

	Look(4092);
	if (!m_hEnemy || !m_hEnemy->IsAlive() || !FVisible(m_hEnemy))
		m_hEnemy = BestVisibleEnemy();

	if (m_hEnemy != nullptr)
	{
		Vector desired = (m_hEnemy->Center() - pev->origin).Normalize();
		pev->angles.y = UTIL_VecToAngles(desired).y;
		pev->velocity = pev->velocity * 0.8f + desired * 40.0f;
		if (RANDOM_LONG(0, 9) == 0)
			FireAtEnemy();
	}
	else
		pev->velocity = pev->velocity * 0.8f;
}

void CChopper::FlyTouch(CBaseEntity* pOther)
{
	if (pOther && pOther->pev->solid == SOLID_BSP)
	{
		TraceResult tr = UTIL_GetGlobalTrace();
		pev->velocity = pev->velocity + tr.vecPlaneNormal * (pev->velocity.Length() + 200.0f);
	}
}

void CChopper::Killed(entvars_t* pevAttacker, int iGib)
{
	pev->movetype = MOVETYPE_TOSS;
	pev->gravity = 0.3f;
	STOP_SOUND(ENT(pev), CHAN_STATIC, "apache/ap_rotor2.wav");
	UTIL_SetSize(pev, Vector(-32, -32, -64), Vector(32, 32, 0));
	SetThink(&CChopper::FallingThink);
	SetTouch(&CChopper::CrashTouch);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->health = 0;
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DYING;
}

void CChopper::GibMonster()
{
	STOP_SOUND(ENT(pev), CHAN_STATIC, "apache/ap_rotor2.wav");
}

void CChopper::FallingThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->avelocity = pev->avelocity * 1.02f;

	Vector spot = pev->origin + Vector(RANDOM_FLOAT(-30, 30), RANDOM_FLOAT(-30, 30), RANDOM_FLOAT(-30, 30));
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, spot);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(spot.x);
	WRITE_COORD(spot.y);
	WRITE_COORD(spot.z);
	WRITE_SHORT(m_iExplosionSprite);
	WRITE_BYTE(50);
	WRITE_BYTE(10);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
	pev->flags &= ~FL_ONGROUND;
}

void CChopper::CrashTouch(CBaseEntity* pOther)
{
	if (!pOther || pOther->pev->solid != SOLID_BSP)
		return;

	SetTouch(NULL);
	Vector spot = pev->origin + (pev->mins + pev->maxs) * 0.5f;
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, spot);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(spot.x);
	WRITE_COORD(spot.y);
	WRITE_COORD(spot.z);
	WRITE_SHORT(m_iExplosionSprite);
	WRITE_BYTE(108);
	WRITE_BYTE(30);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, spot);
	WRITE_BYTE(TE_BREAKMODEL);
	WRITE_COORD(spot.x);
	WRITE_COORD(spot.y);
	WRITE_COORD(spot.z);
	WRITE_COORD(100);
	WRITE_COORD(100);
	WRITE_COORD(132);
	WRITE_COORD(pev->velocity.x);
	WRITE_COORD(pev->velocity.y);
	WRITE_COORD(pev->velocity.z);
	WRITE_BYTE(30);
	WRITE_SHORT(m_iMetalGibs);
	WRITE_BYTE(30);
	WRITE_BYTE(20);
	WRITE_BYTE(BREAK_METAL);
	MESSAGE_END();
	EMIT_SOUND(ENT(pev), CHAN_STATIC, "weapons/explode5.wav", 1.0f, ATTN_NORM);
	SetThink(&CChopper::SUB_Remove);
	pev->nextthink = gpGlobals->time + 0.1f;
}

void CChopper::FireAtEnemy()
{
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	// Confirmed spread angle from the decompile (0.06976 rad); pellet
	// count/aim-tolerance gate approximated (see file header).
	FireBullets(1, vecShootOrigin, vecShootDir, Vector(0.06976, 0.06976, 0), 8192, BULLET_MONSTER_MP5);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 1.0, ATTN_NORM, 0, 100);
	pev->effects |= EF_MUZZLEFLASH;
}
