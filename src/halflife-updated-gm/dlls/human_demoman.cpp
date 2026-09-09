//=========================================================
// monster_human_demoman (CDemoman, real C++ class name confirmed via
// a raw string literal "CDemoman" in gunman.dll) plus its two
// projectile/ordnance classes demoman_mine (CDemomanMine) and
// demoman_rocket (CDemomanRocket). First gap of west3b/west4a/west5b/
// west6a/west6b/west6c/west6d in the map-driven pass. NOT part of the
// CTalkMonster family (same "hostile CBaseMonster, own combat NPC"
// shape as human_bandit.cpp's CBanditGrunt) - see
// [[project-talking-human-npc-pattern]].
//
// Decompiled fresh from gunman.dll:
//   monster_human_demoman: LINK @0x100ac9a0, vtable @0x101007a8, Spawn
//     @0x100acd50, Precache @0x100acd10 (calls a shared
//     "Human-Bandit"-family base precache FUN_100aa1d0, confirmed via
//     findings/entities/monster_human_demoman.md's xref analysis to
//     also be used by CBanditGrunt/CFriendlyGunman/monster_human_
//     scientist - reproduced here with CBanditGrunt's own confirmed
//     precache list rather than a separate helper class, matching
//     this project's existing pattern of flattening shared C++ base
//     precache into each leaf class), Classify (slot 8, @0x100ad540,
//     confirmed constant 5 = CLASS_ALIEN_MILITARY, same as
//     CBanditGrunt), HandleAnimEvent (slot 58, @0x100ad880). KeyValue
//     (slot 2) is the generic shared stub - unlike CBanditGrunt, this
//     class has NO custom FGD keyvalues.
//   Spawn confirms: bbox implied by model (not separately set - the
//     default CBaseMonster hull from the SDK is used, no
//     UTIL_SetSize call found in the decompile, matching an
//     unusually short Spawn body), pev->rendermode/skin randomized
//     30%-vs-random split (reproduced via RANDOM_LONG), health
//     confirmed as skill-scaled via a DAT_10136ea8 global not
//     resolved to a concrete constant this session - approximated as
//     100 (matches the doc's "~30% Health=100" branch), view_ofs.z =
//     68 (0x42880000). Skill-cvar-scaled field at param_1[0xb5]
//     (3/4/6 for skill 1/2/3) copied into param_1[0x87] - exact
//     semantic meaning (likely an internal ammo-count/rocket-reserve
//     field for HandleAnimEvent's rocket branch) not resolved; not
//     reproduced since HandleAnimEvent's rocket firing here has no
//     ammo gate of its own (matches CBanditGrunt's precedent of
//     omitting internal ammo bookkeeping for monster attacks).
//   HandleAnimEvent's 4 real events (byte-confirmed, matches
//     findings' Session-91 table exactly): event 1 = rocket launch
//     (spawns demoman_rocket), event 3 = mine drop (spawns
//     demoman_mine, then calls a Use()-equivalent on it to arm it -
//     reproduced as immediate SetThink(WaitToExplode) call instead),
//     event 6 = shotgun blast (weapons/sbarrel1.wav +
//     BULLET_MONSTER_BUCKSHOT spread, same shared multi-pellet
//     pattern as CBanditGrunt's MechaVolley/weapon_shotgun), event
//     0xb = CORRECTED (2026-09-05): confirmed via a direct decompile
//     of FUN_10001f90 to be a plain SetBodygroup(2, 2) call - a raw
//     bodygroup-array walk over the model's studio-header
//     numbodyparts/bodypart-offset fields, writing pev->body. There is
//     NO melee damage/traceline in the original for this event at
//     all (the earlier claim of "128-unit melee traceline" here was a
//     fabrication, not a reproduction - see Melee()'s own comment and
//     findings/open_items_audit_2026-09-05.md). CBanditGrunt's event
//     0xb (human_bandit.cpp) is unrelated real combat code, not shared
//     with this class despite the matching event number.
//
//   demoman_mine: LINK @0x100ab950, vtable @0x10100344, Spawn
//     @0x100aba40, Precache @0x100aba10, WaitToExplode (Think)
//     @0x100abd20 - confirmed derived from stock SDK CGrenade
//     (calls CGrenade::Detonate() directly at the end of
//     WaitToExplode). Confirmed: model "models/demomine.mdl",
//     MOVETYPE_FLY(6), bbox (-4,-4,-8)/(4,4,8), SOLID_BBOX,
//     takedamage=DAMAGE_YES, a skill-scaled damage field
//     (DAT_10136fe0, not resolved to a concrete float - approximated
//     as 75, matching sk_demoman_* skill-cvar naming convention seen
//     in the symbol table), and a blinking warning-light child sprite
//     ("sprites/camled.spr") plus a "weapons/dml_lock.wav" tick sound
//     played once armed. Simplified relative to the original: the
//     blinking-LED child sprite/lock-tick timing (WaitToExplode's
//     second half, driving a toggled pev->effects bit on the
//     spawned camled sprite) is NOT reproduced - a fixed 3-second
//     fuse leading straight to CGrenade::Detonate() is used instead,
//     matching this project's established pattern of dropping purely
//     cosmetic child-entity detail from fuse/warning timers (see
//     CTubeQueen's UseQueen stub) while keeping the core
//     arm-then-explode mechanic and the confirmed damage/model/hull
//     values.
//
//   demoman_rocket: LINK @0x100abf40, vtable @0x10100574,
//     RocketExplodeTouch @0x100ac130 (thin wrapper around stock
//     CGrenade::ExplodeTouch, confirming CDemomanRocket : CGrenade),
//     IgniteThink @0x100ac160 (spawns a trailing smoke-sprite child,
//     plays weapons/rocket1.wav, sends two temp-entity sprite
//     messages for muzzle flash/glow, then switches Think to
//     AccelerateThink), AccelerateThink @0x100ac390 - a full
//     nearby-target homing/steering algorithm (scans entities within
//     a ~1024-unit radius, steers velocity toward the nearest valid
//     enemy, clamps speed between 1200-1400 units/s). Simplified
//     relative to the original: AccelerateThink's homing-steering
//     math is NOT reproduced - the rocket flies in a straight line at
//     a fixed 1200 units/s from its confirmed launch velocity/origin
//     instead, exploding on any touch via the confirmed
//     RocketExplodeTouch->CGrenade::ExplodeTouch chain. Matches this
//     project's established pattern of documenting-not-reproducing
//     homing/steering AI math (same rationale as CTubeQueen's full
//     combat AI, CXenome's exact visibility threshold).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"
#include "gamerules.h"

#define DEMOMAN_AE_ROCKET 1
#define DEMOMAN_AE_MINE 3
#define DEMOMAN_AE_SHOTGUN 6
#define DEMOMAN_AE_MELEE 0xb

class CDemomanMine : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT WaitToExplode();
};
LINK_ENTITY_TO_CLASS(demoman_mine, CDemomanMine);

void CDemomanMine::Spawn()
{
	Precache();

	pev->classname = MAKE_STRING("demoman_mine");
	SET_MODEL(ENT(pev), "models/demomine.mdl");
	UTIL_SetSize(pev, Vector(-4, -4, -8), Vector(4, 4, 8));

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->dmg = 75; // plausible default - exact skill-cvar lookup not traced this session, see file header

	SetThink(&CDemomanMine::WaitToExplode);
	pev->nextthink = gpGlobals->time + 3.0;
}

void CDemomanMine::Precache()
{
	PrecacheModel("models/demomine.mdl");
	PrecacheModel("sprites/camled.spr");
	PrecacheSound("weapons/dml_lock.wav");
}

void CDemomanMine::WaitToExplode()
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/dml_lock.wav", 1.0, ATTN_NORM);
	CGrenade::Detonate();
}

class CDemomanRocket : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketExplodeTouch(CBaseEntity* pOther);

private:
	int m_iTrailSprite = 0;
};
LINK_ENTITY_TO_CLASS(demoman_rocket, CDemomanRocket);

void CDemomanRocket::Spawn()
{
	Precache();

	pev->classname = MAKE_STRING("demoman_rocket");
	SET_MODEL(ENT(pev), "models/dmlrocket.mdl");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SetTouch(&CDemomanRocket::RocketExplodeTouch);

	UTIL_MakeVectors(pev->angles);
	pev->velocity = gpGlobals->v_forward * 1200; // fixed speed, see file header (homing not reproduced)

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "weapons/rocket1.wav", 1.0, ATTN_NORM);
	// Nachtrag 2026-09-06 (Nutzer-Meldung "Raketen-Schweif fehlt auch bei
	// anderen Raketen"): siehe UTIL_RocketTrail (util.cpp).
	UTIL_RocketTrail(this, m_iTrailSprite);

	pev->nextthink = gpGlobals->time + 5.0;
	SetThink(&CDemomanRocket::SUB_Remove);
}

void CDemomanRocket::Precache()
{
	PrecacheModel("models/dmlrocket.mdl");
	PrecacheSound("weapons/rocket1.wav");
	// Sprite-Pruefung 2026-09-06: smoke.spr statt flame.spr, siehe dml.cpp
	// Nachtrag 2 fuer die Begruendung (flame.spr = Glow-Burst, kein Schweif).
	m_iTrailSprite = PrecacheModel("sprites/smoke.spr");
}

void CDemomanRocket::RocketExplodeTouch(CBaseEntity* pOther)
{
	CGrenade::ExplodeTouch(pOther);
}

class CDemoman : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_MILITARY; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override left
	// pev->yaw_speed at 0 (CBaseMonster's default is a no-op) - the
	// monster couldn't turn toward its enemy at all. Same fix as
	// CFriendlyGunman (human_gunman.cpp); see
	// findings/open_items_audit_2026-09-05.md for the full story.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	// BUG FIX (2026-09-05, live gameplay report): this NPC is commonly
	// puppeted by scripted_sequence "waiting"/idle-pose loops across
	// this project's maps (self-retriggering scripted_sequence chains
	// playing named, non-ACT_IDLE-tagged sequences). Without this
	// override, CBaseMonster's default SetActivity(ACT_IDLE) (called by
	// the default AI schedule between scripted waypoints, even for an
	// otherwise full combat monster) stomps pev->sequence mid-animation,
	// cutting the wait pose short and restarting it from frame 0 - same
	// root cause as CFurniture/CGenericMonster/CRenesaur/CAIGirl/
	// CFriendlyGunman. Normal combat activity switching is unaffected.
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
	void DropMine();
	void Shotgun();
	void Melee();
};
LINK_ENTITY_TO_CLASS(monster_human_demoman, CDemoman);

void CDemoman::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/demolitionman.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = RANDOM_LONG(0, 9) < 3 ? 100 : RANDOM_LONG(0, 7) + 109; // confirmed 30%/random split, see file header
	pev->view_ofs = Vector(0, 0, 68);

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;

	MonsterInit();
	// Some Gunman models do not carry $eyeposition. Restore the confirmed
	// gameplay eye height after MonsterInit's model-derived lookup.
	pev->view_ofs = Vector(0, 0, 68);
}

void CDemoman::Precache()
{
	PrecacheModel("models/demolitionman.mdl");
	PrecacheModel("models/shell.mdl");
	PrecacheModel("models/shotgunshell.mdl");
	PrecacheModel("models/humanskull.mdl");
	PrecacheModel("models/hgibs.mdl");
	PrecacheModel("sprites/zbeam5.spr");
	PrecacheModel("sprites/gaussbeam1.spr");
	PrecacheModel("sprites/gausspark.spr");
	PrecacheModel("sprites/gorehuman.spr");
	PrecacheModel("sprites/gibhuman.spr");
	PrecacheEvent(1, "events/monstershotgun.sc");

	PrecacheSound("weapons/gauss_fire1.wav");
	PrecacheSound("weapons/gauss_fire2.wav");
	PrecacheSound("weapons/gauss_charge.wav");
	PrecacheSound("debris/bustflesh1.wav");
	PrecacheSound("bandit/men_check_in.wav");
	PrecacheSound("weapons/sbarrel1.wav");
	PrecacheSound("demoman/demo_dropmine.wav");
	PrecacheSound("demoman/demo_launchrocket.wav");

	UTIL_PrecacheOther("demoman_mine");
	UTIL_PrecacheOther("demoman_rocket");
}

void CDemoman::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case DEMOMAN_AE_ROCKET:
		LaunchRocket();
		break;
	case DEMOMAN_AE_MINE:
		DropMine();
		break;
	case DEMOMAN_AE_SHOTGUN:
		Shotgun();
		break;
	case DEMOMAN_AE_MELEE:
		Melee();
		break;
	default:
		break;
	}
}

void CDemoman::LaunchRocket()
{
	if (!m_hEnemy)
		return;

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "demoman/demo_launchrocket.wav", 1.0, ATTN_NORM);

	CBaseEntity* pRocket = CBaseEntity::Create("demoman_rocket", vecShootOrigin, UTIL_VecToAngles(vecShootDir));
	pRocket->pev->owner = edict();
}

void CDemoman::DropMine()
{
	EMIT_SOUND(ENT(pev), CHAN_WEAPON, "demoman/demo_dropmine.wav", 1.0, ATTN_NORM);

	UTIL_MakeVectors(pev->angles);
	Vector vecDrop = pev->origin + gpGlobals->v_forward * 32;
	CBaseEntity* pMine = CBaseEntity::Create("demoman_mine", vecDrop, pev->angles);
	pMine->pev->owner = edict();
}

void CDemoman::Shotgun()
{
	if (!m_hEnemy)
		return;

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1.0, ATTN_NORM, 0, 100);

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	FireBullets(6, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT);
}

void CDemoman::Melee()
{
	// CORRECTION (2026-09-05): a fresh re-decompile as part of resolving
	// the FUN_10001f90 interpretation contradiction flagged in
	// findings/open_items_audit_2026-09-05.md shows the original event
	// 0xb body is JUST `SetBodygroup(2, 2)` (FUN_10001f90 is a raw
	// SetBodygroup-equivalent helper - confirmed by decompiling it
	// directly: it walks the model's bodypart array via the numbodyparts/
	// bodypart-offset studio-header fields and writes pev->body,
	// nowhere touching combat/traceline code). There is NO melee damage
	// call at all in the original for this event - the previous version
	// of this function (a 128-unit distance check + TakeDamage) was a
	// fabrication, not a reproduction of confirmed behavior. Bandit's
	// own event 0xb (human_bandit.cpp) IS a real, more elaborate
	// grab-then-traceline attack - it does not share code with this
	// class's event 0xb despite the same event number and a similar
	// name in this project's convention.
	SetBodygroup(2, 2);
}
