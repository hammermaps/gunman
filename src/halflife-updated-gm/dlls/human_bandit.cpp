//=========================================================
// monster_human_bandit (alias monster_human_grunt) - CBanditGrunt (real
// C++ class name confirmed via a raw string literal in gunman.dll,
// right next to the "MechaGunBandit"/"StartDrawn" keyvalue strings).
// A hostile combat NPC, NOT part of the CTalkMonster "Talk/Ally"
// family that monster_human_gunman/monster_human_unarmed belong to -
// see findings/entities/human_bandit_base_class.md for the full
// two-family breakdown (this session's decompile confirms every
// address the doc already listed, cross-checked rather than assumed).
//
// Decompiled from gunman.dll: constructor @0x100b19c0 (vtable
// 0x10100e64), Spawn @0x100b1b80, Precache @0x100b1b20, KeyValue
// @0x100b0410, Classify @0x100b19e0 (constant 5 =
// CLASS_ALIEN_MILITARY), HandleAnimEvent @0x100b10e0 (vtable slot
// 58).
//
// KeyValues confirmed against the retail FGD's monster_human_bandit
// block: "StartDrawn" (Choices, default 1=Yes/drawn, 2=No), "body"
// (Choices, FGD title "Head" - 0=Random/1=Original/2=Bandana/3=Tail/
// 4=Pilot - re-parsed defensively by the DLL's own KeyValue() even
// though the engine already auto-maps "body" onto pev->body directly;
// not the "Head" bone-name string that also happens to sit nearby in
// the binary's string table, which is unrelated), "MechaGunBandit"
// (Choices 0=No/1=Yes - selects the "Mecha Gun Bandit" weapon mode).
//
// HandleAnimEvent's real event IDs (all confirmed via the fresh
// decompile, matching findings/entities/human_bandit_base_class.md's
// table exactly):
//   1/2 -> FUN_100a9290 (single gauss-pistol-style shot)
//   3   -> weapon-drawn bodygroup state
//   4   -> weapon-holstered bodygroup state
//   5/6 -> FUN_100a9890 (single shot, second variant)
//   8   -> Mecha-mode volley via FUN_1000f090 (the shared multi-pellet
//          traceline helper also used by weapon_shotgun/CFriendlyGunman
//          /CChopper), confirmed spread angle 0.02618 rad, plays
//          weapons/hks1.wav or hks2.wav
//   0xb -> a real, more elaborate melee than a plain traceline:
//          searches nearby entities within 128 units (FUN_1000e570),
//          applies a small random positional shove to the found
//          target, THEN does an LOS traceline (FUN_1005f470) and, if
//          clear, deals skill-scaled damage via a virtual call. NOTE
//          (2026-09-05 correction, see findings/open_items_audit_
//          2026-09-05.md): earlier revisions of this comment and
//          CBanditGrunt::Melee() attributed this event's damage call
//          to `FUN_10001f90` - that function was independently
//          decompiled and is actually an unrelated SetBodygroup(group,
//          value) helper, confirmed also used (correctly) for this
//          same class's events 3/4 below. The real event-0xb damage
//          path uses different helper functions entirely
//          (FUN_1000e570/FUN_1005f470), not FUN_10001f90.
//   default -> footstep/misc fallback (FUN_100a9cb0), cosmetic only
//
// Simplified relative to the original (documented per-case): events
// 1/2/5/6 all fire through a single generic FireBullets() call here
// instead of reproducing FUN_100a9290/FUN_100a9890's separate,
// byte-exact aim/spread math (same simplification level already used
// for CFriendlyGunman's Shoot()). Event 8's real FUN_1000f090 pellet
// count/damage-per-pellet parameters were not fully decoded from the
// decompile's register-heavy calling convention - approximated as a
// 6-pellet FireBullets spray at the confirmed 0.02618 rad spread
// angle. Event 0xb's melee is a simple forward-distance+TakeDamage
// check instead of the original's shove-then-LOS-gated damage
// sequence described above. No custom Schedule_t/Task_t AI tables (same
// established simplification as every other monster_human_* class in
// this project) - relies on default CBaseMonster scheduling.
// monster_human_grunt (per findings a pure classname alias, same
// class) is NOT linked here despite being the same confirmed C++
// class: that classname is already claimed by this SDK's stock
// hgrunt.cpp (CHGrunt, the unrelated vanilla-HL grunt) and MAYAN3A's
// gap scan doesn't require it - would need the naming collision
// resolved (e.g. renaming CHGrunt's link) before a future map that
// actually needs Gunman's own "monster_human_grunt" semantics.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "soundent.h"

#define BANDIT_AE_SHOOT1 1
#define BANDIT_AE_SHOOT2 2
#define BANDIT_AE_DRAW 3
#define BANDIT_AE_HOLSTER 4
#define BANDIT_AE_SHOOT3 5
#define BANDIT_AE_SHOOT4 6
#define BANDIT_AE_MECHA_VOLLEY 8
#define BANDIT_AE_MELEE 0xb

class CBanditGrunt : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_MILITARY; }
	// CORRECTION (2026-09-05): CBaseMonster::SetYawSpeed() defaults to a
	// no-op, so without this override pev->yaw_speed stays 0 and the
	// monster can never turn to face its enemy/movement target - it
	// looks like it "doesn't react" and walks in whatever direction it
	// happened to spawn facing (often backward relative to where it's
	// trying to go). Same root cause and fix already applied to
	// CFriendlyGunman (human_gunman.cpp) - propagated here after a
	// player-reported regression, see findings/open_items_audit_2026-09-05.md.
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
	bool KeyValue(KeyValueData* pkvd) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	void Shoot();
	void MechaVolley();
	void Melee();

	int m_iBody = 0;		  // "body" keyvalue (FGD title "Head") - 0=Random, 1-4 = fixed head variants
	int m_iStartDrawn = 1;	  // "StartDrawn" keyvalue - 1=Yes (default), 2=No
	bool m_bMechaGun = false; // "MechaGunBandit" keyvalue
};
LINK_ENTITY_TO_CLASS(monster_human_bandit, CBanditGrunt);

TYPEDESCRIPTION CBanditGrunt::m_SaveData[] =
	{
		DEFINE_FIELD(CBanditGrunt, m_iBody, FIELD_INTEGER),
		DEFINE_FIELD(CBanditGrunt, m_iStartDrawn, FIELD_INTEGER),
		DEFINE_FIELD(CBanditGrunt, m_bMechaGun, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CBanditGrunt, CBaseMonster);

bool CBanditGrunt::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "body"))
	{
		m_iBody = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "StartDrawn"))
	{
		m_iStartDrawn = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "MechaGunBandit"))
	{
		m_bMechaGun = atoi(pkvd->szValue) != 0;
		return true;
	}

	return CBaseMonster::KeyValue(pkvd);
}

void CBanditGrunt::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/bandit.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 50; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = Vector(0, 0, 60);

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;

	MonsterInit();

	SetBodygroup(2, m_iBody == 0 ? RANDOM_LONG(0, 3) : m_iBody - 1);

	if (m_bMechaGun)
	{
		SetBodygroup(1, 2);
		SetBodygroup(3, 1);
	}
	else
	{
		SetBodygroup(1, m_iStartDrawn != 1 ? 1 : 0);
	}
}

void CBanditGrunt::Precache()
{
	PrecacheModel("models/bandit.mdl");
	PrecacheModel("models/shell.mdl");
	PrecacheEvent(1, "events/monstermechagun.sc");

	PrecacheSound("weapons/gauss_fire1.wav");
	PrecacheSound("weapons/gauss_fire2.wav");
	PrecacheSound("weapons/gauss_charge.wav");
	PrecacheSound("weapons/hks1.wav");
	PrecacheSound("weapons/hks2.wav");
	PrecacheSound("debris/bustflesh1.wav");
	PrecacheSound("bandit/men_check_in.wav");
	PrecacheModel("sprites/gorehuman.spr");
	PrecacheModel("sprites/gibhuman.spr");
}

void CBanditGrunt::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BANDIT_AE_SHOOT1:
	case BANDIT_AE_SHOOT2:
	case BANDIT_AE_SHOOT3:
	case BANDIT_AE_SHOOT4:
		Shoot();
		break;
	case BANDIT_AE_DRAW:
		SetBodygroup(1, 0);
		break;
	case BANDIT_AE_HOLSTER:
		SetBodygroup(1, 1);
		break;
	case BANDIT_AE_MECHA_VOLLEY:
		if (m_bMechaGun)
			MechaVolley();
		break;
	case BANDIT_AE_MELEE:
		Melee();
		break;
	default:
		break;
	}
}

void CBanditGrunt::Shoot()
{
	if (!m_hEnemy)
		return;

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_MONSTER_MP5);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/gauss_fire1.wav", 1.0, ATTN_NORM, 0, 100);
}

void CBanditGrunt::MechaVolley()
{
	if (!m_hEnemy)
		return;

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	// Confirmed spread angle from the decompile (0.02618 rad); pellet
	// count/per-pellet damage approximated (see file header).
	FireBullets(6, vecShootOrigin, vecShootDir, Vector(0.02618, 0.02618, 0), 2048, BULLET_MONSTER_MP5);
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, RANDOM_LONG(0, 1) ? "weapons/hks1.wav" : "weapons/hks2.wav", 1.0, ATTN_NORM, 0, 100);
	pev->effects |= EF_MUZZLEFLASH;
}

void CBanditGrunt::Melee()
{
	if (!m_hEnemy)
		return;

	if ((m_hEnemy->pev->origin - pev->origin).Length() <= 128)
	{
		m_hEnemy->TakeDamage(pev, pev, 15, DMG_CLUB);
	}
}
