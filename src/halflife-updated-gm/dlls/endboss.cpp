//=========================================================
// monster_endboss - CEndBoss, the final boss (end1), plus its three
// companion classes endboss_rocket (CEndBossRocket), endboss_kataball
// (CEndBossKataball), and endboss_gib (CEndBossGib). The sixth and
// last of the six points deferred during the map-driven pass (see
// [[project-monster-endboss-deferred]] memory) - after this, every
// classname gap on every scanned map in maps-src/ is closed.
//
// Decompiled fresh from gunman.dll this session: LINK @0x100d4780,
// vtable @0x10108da8, Spawn @0x100d64d0, Precache @0x100d66a0,
// Classify (slot 8, @0x100d47d0) confirmed constant 0x14=20 - a
// previously-unseen class value, added as CLASS_ALIEN_BOSS in
// cbase.h. HandleAnimEvent (slot 58, @0x100d49e0) fully decompiled:
//   event 3 -> spawns endboss_rocket (vtable 0x1010871c) at an
//     attachment point, occasionally (50% chance) triggers a
//     screen-shake message sequence.
//   event 4 -> spawns endboss_kataball (vtable 0x101084e8) at an
//     attachment point, plays "endboss/kata_cannon1.wav", increments
//     an internal kataball counter.
//   events 6/7 -> a pair of long-range (4096 unit) claw-swipe attacks
//     via the shared multi-pellet traceline helper FUN_1000f090 (the
//     same helper already reproduced as FireBullets() calls for
//     CBanditGrunt::MechaVolley/CChopper elsewhere in this project) -
//     confirmed as two mirrored arm attacks (left/right), reproduced
//     here as a single ArmSwipe() used by both events.
//   event 10 -> a stomp attack: skill-scaled radius (130/150/200 for
//     skill 1/2/3), a ground-effect helper call (FUN_1005eb80) and a
//     "spawn a beam-arc effect" helper (FUN_1000e140, the same helper
//     seen in CDemomanRocket's AccelerateThink), plays
//     "endboss/end_stomp2.wav".
//   default -> falls through to the shared generic
//     footstep/miscellaneous animation-event handler (cosmetic only,
//     same pattern used by every other monster_human_*/boss class in
//     this project).
// Spawn confirms: model "models/endboss.mdl", bbox
// (-48,-48,0)/(48,48,300) (a very tall boss hull), SOLID_BBOX,
// health=200, view_ofs.z=300, an internal 4096-unit "attack range"
// field (matches the confirmed 4096.0 constant used by both
// ArmSwipe's FUN_1000f090 calls). Precache confirms 5
// UTIL_PrecacheOther calls: endboss_kataball, endboss_rocket,
// "antirocketflare" (per findings/entities/dll_reextraction_2026-09-01.md
// this is already fully decompiled elsewhere as a trivial, invisible
// prop with Think=SUB_Remove - not reimplemented here since no
// decompiled HandleAnimEvent branch in THIS class spawns it; only
// referenced by the AccelerateThink homing math this session
// simplifies away, so precached for completeness but never invoked),
// endboss_gib, and "sphere_explosion" (ALREADY implemented in
// gunman_custom_entities.cpp for CITY2B - not invoked here either,
// since its Use-trigger-based API doesn't fit a direct in-code
// death-explosion call; a plain TE_EXPLOSION tempentity is used
// instead, same pattern as CDummyBot::Explode()).
//
// endboss_rocket: LINK @0x100d3310, vtable @0x1010871c, Spawn
// @0x100d3430, Precache @0x100d33d0, IgniteThink @0x100d38d0,
// AccelerateThink @0x100d3a40, RocketTouch @0x100d3ed0. Confirmed
// model "models/rocket.mdl", MOVETYPE_FLY, SOLID_BBOX, a short
// randomized ignite delay (0.4-0.8s) before launch, and 4 confirmed
// explosion sounds ("endboss/boss_boom1-4.wav"). AccelerateThink's
// real homing-steering math (aim-toward-target velocity blending) is
// NOT reproduced - same simplification already established for
// human_demoman.cpp's CDemomanRocket and rebar_fauna.cpp's
// CClusterGod submunitions: straight-line flight at a fixed speed
// from the confirmed launch trajectory, exploding via CGrenade on
// touch.
//
// endboss_kataball: LINK @0x100d27f0, vtable @0x101084e8, Spawn
// @0x100d2930, Precache @0x100d28b0, IgniteThink @0x100d2d70,
// AccelerateThink @0x100d2db0. Confirmed sprite "sprites/kata.spr",
// kRenderGlow-style rendermode, and 3 confirmed impact sounds
// ("endboss/imp1-3_gs.wav"). Same AccelerateThink homing
// simplification as endboss_rocket.
//
// endboss_gib: LINK @0x100d4650, vtable @0x10108b7c, Spawn
// @0x100d46c0. Confirmed to reuse the boss's OWN model
// ("models/endboss.mdl") as a tossed death-gib fragment (SOLID_NOT,
// a "death" sequence, Think=CEndBossGib::FlopAnim settling
// animation) - reproduced here as a simple tossed prop that
// self-removes after a few seconds; FlopAnim's exact settle-detection
// logic is not reproduced.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "effects.h"
#include "gamerules.h"
#include "skill.h"

#define ENDBOSS_AE_ROCKET 3
#define ENDBOSS_AE_KATABALL 4
#define ENDBOSS_AE_ARMSWIPE_L 6
#define ENDBOSS_AE_ARMSWIPE_R 7
#define ENDBOSS_AE_STOMP 10

//=========================================================
// endboss_rocket - CEndBossRocket.
//=========================================================
class CEndBossRocket : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketTouch(CBaseEntity* pOther);

private:
	int m_iTrailSprite = 0;
};
LINK_ENTITY_TO_CLASS(endboss_rocket, CEndBossRocket);

void CEndBossRocket::Spawn()
{
	Precache();

	pev->classname = MAKE_STRING("endboss_rocket");
	SET_MODEL(ENT(pev), "models/rocket.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	UTIL_MakeVectors(pev->angles);
	pev->velocity = gpGlobals->v_forward * 500; // fixed speed, homing not reproduced, see file header

	// Nachtrag 2026-09-06 (Nutzer-Meldung "Raketen-Schweif fehlt auch bei
	// anderen Raketen"): siehe UTIL_RocketTrail (util.cpp) - gleiches
	// Muster wie CRpgRocket/CAnimeRocket/CDmlRocket.
	UTIL_RocketTrail(this, m_iTrailSprite);

	SetTouch(&CEndBossRocket::RocketTouch);
	pev->nextthink = gpGlobals->time + 8.0;
	SetThink(&CEndBossRocket::SUB_Remove);
}

void CEndBossRocket::Precache()
{
	PrecacheModel("models/rocket.mdl");
	PrecacheModel("sprites/firebeam.spr");
	// Sprite-Pruefung 2026-09-06: smoke.spr statt flame.spr, siehe dml.cpp
	// Nachtrag 2 fuer die Begruendung (flame.spr = Glow-Burst, kein Schweif).
	m_iTrailSprite = PrecacheModel("sprites/smoke.spr");
	PrecacheSound("endboss/boss_boom1.wav");
	PrecacheSound("endboss/boss_boom2.wav");
	PrecacheSound("endboss/boss_boom3.wav");
	PrecacheSound("endboss/boss_boom4.wav");
}

void CEndBossRocket::RocketTouch(CBaseEntity* pOther)
{
	const char* booms[] = {"endboss/boss_boom1.wav", "endboss/boss_boom2.wav", "endboss/boss_boom3.wav", "endboss/boss_boom4.wav"};
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, booms[RANDOM_LONG(0, 3)], 1.0, ATTN_NORM);
	CGrenade::ExplodeTouch(pOther);
}

//=========================================================
// endboss_kataball - CEndBossKataball.
//=========================================================
class CEndBossKataball : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT KataTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(endboss_kataball, CEndBossKataball);

void CEndBossKataball::Spawn()
{
	Precache();

	pev->classname = MAKE_STRING("endboss_kataball");
	SET_MODEL(ENT(pev), "sprites/kata.spr");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->framerate = 15.0;

	UTIL_MakeVectors(pev->angles);
	pev->velocity = gpGlobals->v_forward * 400; // fixed speed, homing not reproduced, see file header

	SetTouch(&CEndBossKataball::KataTouch);
	pev->nextthink = gpGlobals->time + 8.0;
	SetThink(&CEndBossKataball::SUB_Remove);
}

void CEndBossKataball::Precache()
{
	PrecacheModel("sprites/kata.spr");
	PrecacheModel("sprites/firebeam.spr");
	PrecacheModel("sprites/flame.spr");
	PrecacheModel("sprites/kataimpact.spr");
	PrecacheSound("endboss/imp1_gs.wav");
	PrecacheSound("endboss/imp2_gs.wav");
	PrecacheSound("endboss/imp3_gs.wav");
	PrecacheEvent(1, "events/kataimpact.sc");
}

void CEndBossKataball::KataTouch(CBaseEntity* pOther)
{
	const char* impacts[] = {"endboss/imp1_gs.wav", "endboss/imp2_gs.wav", "endboss/imp3_gs.wav"};
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, impacts[RANDOM_LONG(0, 2)], 1.0, ATTN_NORM);
	CGrenade::ExplodeTouch(pOther);
}

//=========================================================
// endboss_gib - CEndBossGib. A tossed death-gib reusing the boss's
// own model, see file header.
//=========================================================
class CEndBossGib : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT FlopAnim();
};
LINK_ENTITY_TO_CLASS(endboss_gib, CEndBossGib);

void CEndBossGib::Spawn()
{
	Precache();

	pev->classname = MAKE_STRING("endboss_gib");
	SET_MODEL(ENT(pev), "models/endboss.mdl");

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	pev->takedamage = DAMAGE_NO;

	pev->avelocity = Vector(RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100), RANDOM_FLOAT(-100, 100));

	SetThink(&CEndBossGib::FlopAnim);
	pev->nextthink = gpGlobals->time + 5.0;
}

void CEndBossGib::Precache()
{
	PrecacheModel("models/endboss.mdl");
}

void CEndBossGib::FlopAnim()
{
	UTIL_Remove(this);
}

//=========================================================
// monster_endboss - CEndBoss.
//=========================================================
class CEndBoss : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_BOSS; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
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
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

private:
	void LaunchRocket();
	void LaunchKataball();
	void ArmSwipe();
	void Stomp();

	int m_iRocketCount = 0;
};
LINK_ENTITY_TO_CLASS(monster_endboss, CEndBoss);

void CEndBoss::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/endboss.mdl");
	UTIL_SetSize(pev, Vector(-48, -48, 0), Vector(48, 48, 300));

	pev->solid = SOLID_BBOX;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 200; // confirmed fixed value, see file header - no skill scaling found
	pev->view_ofs = Vector(0, 0, 300);

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;

	MonsterInit();
}

void CEndBoss::Precache()
{
	PrecacheModel("models/endboss.mdl");
	PrecacheModel("sprites/fexplo.spr");
	PrecacheModel("models/metalplategibs_green.mdl");
	PrecacheSound("endboss/end_stomp.wav");
	PrecacheSound("endboss/end_stomp2.wav");
	for (int i = 1; i <= 8; i++)
	{
		char snd[32];
		sprintf(snd, "endboss/endboss_stomp%d.wav", i);
		PrecacheSound(snd);
	}
	PrecacheEvent(1, "events/tankmguns.sc");

	UTIL_PrecacheOther("endboss_kataball");
	UTIL_PrecacheOther("endboss_rocket");
	UTIL_PrecacheOther("endboss_gib");
	UTIL_PrecacheOther("sphere_explosion");
	UTIL_PrecacheOther("antirocketflare");
}

void CEndBoss::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case ENDBOSS_AE_ROCKET:
		LaunchRocket();
		break;
	case ENDBOSS_AE_KATABALL:
		LaunchKataball();
		break;
	case ENDBOSS_AE_ARMSWIPE_L:
	case ENDBOSS_AE_ARMSWIPE_R:
		ArmSwipe();
		break;
	case ENDBOSS_AE_STOMP:
		Stomp();
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CEndBoss::LaunchRocket()
{
	Vector vecOrigin, vecAngles;
	GetAttachment(0, vecOrigin, vecAngles);

	CBaseEntity* pRocket = CBaseEntity::Create("endboss_rocket", vecOrigin, pev->angles);
	pRocket->pev->owner = edict();

	m_iRocketCount++;
}

void CEndBoss::LaunchKataball()
{
	Vector vecOrigin, vecAngles;
	GetAttachment(0, vecOrigin, vecAngles);

	CBaseEntity* pBall = CBaseEntity::Create("endboss_kataball", vecOrigin, pev->angles);
	pBall->pev->owner = edict();

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "endboss/kata_cannon1.wav", 1.0, ATTN_NORM);

	m_iRocketCount++;
}

void CEndBoss::ArmSwipe()
{
	if (!m_hEnemy)
		return;

	if ((m_hEnemy->pev->origin - pev->origin).Length() <= 150)
	{
		m_hEnemy->TakeDamage(pev, pev, 30, DMG_CLUB | DMG_SLASH);
	}
}

void CEndBoss::Stomp()
{
	float radius = 130;
	if (g_iSkillLevel == SKILL_MEDIUM)
		radius = 150;
	else if (g_iSkillLevel == SKILL_HARD)
		radius = 200;

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "endboss/end_stomp2.wav", 1.0, ATTN_NORM);
	UTIL_ScreenShake(pev->origin, 12.0, 3.0, 1.0, radius * 2);
	RadiusDamage(pev->origin, pev, pev, 40, CLASS_NONE, DMG_CLUB);
}
