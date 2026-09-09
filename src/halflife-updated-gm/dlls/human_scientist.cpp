//=========================================================
// monster_human_scientist - CHumanScientist. Second gap of the
// REBAR2* block in the map-driven pass (after ammo_chemical, see
// rebar_misc.cpp). NOT the stock SDK's friendly CScientist/CTalkMonster
// - see findings/entities/scientist_family.md's Session-80 finding:
// this is a hostile, gauss-pistol/shotgun-armed enemy from the same
// "Human-Bandit" combat-NPC family as human_bandit.cpp's CBanditGrunt,
// just reskinned as "Human Evil Scientist" (confirmed by the FGD's own
// literal display name). Shares real vtable-slot code with
// CBanditGrunt (Classify()==5 at the same value, the same shared
// Spawn-tail FUN_100aa080 and Precache-tail FUN_100aa1d0 already
// reproduced in human_bandit.cpp/human_demoman.cpp).
//
// Decompiled fresh from gunman.dll: LINK @0x100b1d00, vtable
// @0x101010ac, Spawn @0x100b1e50, Classify (slot 8, @0x100b25d0,
// confirmed constant 5 = CLASS_ALIEN_MILITARY), HandleAnimEvent (slot
// 58, @0x100b2690). KeyValue (slot 2) is the generic shared stub - no
// custom FGD keyvalues beyond the engine's own generic "skin"
// mapping, matching the doc's finding that there is no weapon-choice
// keyvalue (weapon type is fixed in code, not mapper-selectable,
// unlike CBanditGrunt's MechaGunBandit).
//
// Spawn confirms: model "models/evil_scientist.mdl", bbox
// (-16,-16,0)/(16,16,72), SOLID_SLIDEBOX, skill-scaled health (a
// DAT_10136f14 global doubled for skill>1, not resolved to a concrete
// constant - approximated as 60/120), view_ofs.z = 36 (0x42480000), a
// fixed weapon-type bitmask field set to the constant 0x10 (one of
// the confirmed 1/2/4/0x10/0x40/0x80 values from the shared
// schedule-selection switch documented in
// findings/entities/human_bandit_base_class.md - a single-shot
// gauss-pistol-style ranged attack per the doc's weapon-type mapping).
//
// HandleAnimEvent's real events (fresh-decompiled): event 1 resets an
// internal reload/cooldown timer field (cosmetic bookkeeping, no
// direct player-visible effect reproduced); events 4/5 run a
// nearby-entity-search-then-grab routine (random offset within a
// 64-unit box, then a coordinate-transform/attach-style call) whose
// exact purpose (grab/capture animation vs. a scripted interaction)
// was not conclusively resolved this session - NOT reproduced, same
// "document but don't invent" rationale used elsewhere in this
// project for ambiguous decompiled behavior. Simplified relative to
// the original: reuses CBanditGrunt's confirmed single-shot Shoot()
// pattern (gauss-pistol-style FireBullets + gauss_fire1.wav) as the
// class's one ranged attack, triggered by the same default-fallback
// path other monster_human_* classes use in this project rather than
// re-deriving the exact anim-event ID this class's model uses for
// firing (not resolved in this session's decompile - HandleAnimEvent
// only showed events 1/4/5 plus the generic default branch).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"

class CHumanScientist : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_MILITARY; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
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
	void Shoot();
};
LINK_ENTITY_TO_CLASS(monster_human_scientist, CHumanScientist);

void CHumanScientist::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/evil_scientist.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 60; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = Vector(0, 0, 36);

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_RANGE_ATTACK1;

	MonsterInit();
	pev->view_ofs = Vector(0, 0, 36);
}

void CHumanScientist::Precache()
{
	PrecacheModel("models/evil_scientist.mdl");
	PrecacheModel("models/shell.mdl");
	PrecacheModel("models/shotgunshell.mdl");
	PrecacheEvent(1, "events/monstershotgun.sc");

	PrecacheSound("weapons/gauss_fire1.wav");
	PrecacheSound("weapons/gauss_fire2.wav");
	PrecacheSound("weapons/gauss_charge.wav");
	PrecacheSound("debris/bustflesh1.wav");
	PrecacheSound("bandit/men_check_in.wav");
	PrecacheModel("sprites/gaussbeam1.spr");
	PrecacheModel("sprites/gausspark.spr");
}

void CHumanScientist::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	// No confirmed "fire" anim-event ID was located in this session's
	// decompile (only events 1=cooldown reset and 4/5=an ambiguous
	// grab routine were resolved, see file header) - any other event
	// fires the confirmed gauss-pistol-style attack so the monster
	// still attacks during its firing animation.
	switch (pEvent->event)
	{
	case 1:
	case 4:
	case 5:
		break;
	default:
		Shoot();
		break;
	}
}

void CHumanScientist::Shoot()
{
	if (!m_hEnemy)
		return;

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_MONSTER_MP5);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/gauss_fire1.wav", 1.0, ATTN_NORM, 0, 100);
}
