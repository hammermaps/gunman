//=========================================================
// rust1's remaining classes with no SDK precedent: ammo_dmlsingle
// (CDMLSingleAmmo), monster_rustbit (CRustbit) and monster_beak
// (CBeak). Also adds rustbitshot (CRustbitShot, monster_rustbit's
// long-open fireball projectile, added 2026-09-05 during the
// open-items audit follow-up - see its own header comment below for
// full details).
//
// Decompiled fresh from gunman.dll this session:
//   ammo_dmlsingle: LINK @0x100847b0, vtable @0x100fb070, Spawn
//     @0x10084810, Precache @0x10084840 -> shared CBasePlayerAmmo
//     spawn tail @0x100633c0 (confirms it's actually a
//     CBasePlayerAmmo, not a plain CItem as
//     findings/entity_review_list.csv's summary said - uses
//     CBasePlayerAmmo::GlowThink/DefaultTouch directly). Model
//     "models/singlerocket.mdl", pickup sound "items/9mmclip1.wav".
//   monster_rustbit: LINK @0x100b6a10, vtable @0x10101934 (matches
//     findings/entities/rustbot_family.md's independently-found
//     vtable value exactly), Spawn @0x100b7750, Precache @0x100b7920,
//     Classify @0x100b6a70 (constant 8 = CLASS_ALIEN_PREY),
//     HandleAnimEvent @0x100b71c0 (vtable slot 58). Confirmed bbox
//     (-16,-16,0)/(16,16,42), SOLID_SLIDEBOX, cooldown field
//     RANDOM(-10,10)+80 (matches the doc's "Zufalls-Cooldown-Feld"
//     note). HandleAnimEvent's real structure, freshly decompiled
//     rather than assumed from the doc: events 1/3 and 2/4 maintain a
//     persistent hand-mounted dynamic light + glow sprite (left/right
//     hand respectively) - NOT a ranged fireball throw as an older
//     findings summary line suggested; events 5/6 are in fact an
//     80-unit alternating left/right melee claw traceline with random
//     +-10 unit jitter and direct TakeDamage - this session's fresh
//     decompile corrects that specific point in the older doc rather
//     than reproducing it.
//   monster_beak: LINK @0x100c04a0, vtable @0x10102ad4, Spawn
//     @0x100c1af0, Precache @0x100c1a20, Classify @0x100c04f0
//     (constant 0xb = 11 = CLASS_PLAYER_ALLY - confirmed literally by
//     the decompile and kept as-is per "Code ist Wahrheit", even
//     though findings/entity_review_list.csv describes it purely as
//     a leaping melee attacker with no mention of being allied).
//     Confirmed bbox (-16,-16,0)/(16,16,40), SOLID_SLIDEBOX, matches
//     the FGD's documented 32x32x40 footprint.
//
// Simplified relative to the original (documented per-case):
// - CDMLSingleAmmo: gives ammo type "dml" (weapon_dml itself is not
//   yet implemented in this project - see the deferred weapon_minigun
//   memory note for the analogous situation; the give amount/max
//   carry here are plausible placeholders, not byte-verified against
//   an unimplemented weapon's clip size).
// - CRustbit: the persistent two-hand glow sprite rig (events 1-4),
//   its Save/Restore links and death cleanup are reproduced. The
//   former approximation that events 2/4 fire rustbitshot was removed:
//   retail's events 1-4 are visual only. The confirmed melee claw
//   attack (events 5/6) uses the retail 80-unit hull trace, but its
//   exact skill-scaled damage remains unresolved.
//   findings/entities/rustbot_family.md's "GetSchedule/GetScheduleOfType
//   waehlen ueber Distanz-/Zustandsfelder zwischen Melee-/
//   Fernkampf-Schedules" custom AI is not reproduced - default
//   CBaseMonster scheduling is used instead (same simplification
//   level as every other monster in this project).
// - CBeak: findings/entity_review_list.csv documents a full custom
//   Schedule_t/Task_t leap-attack system (StartTask case 100 = a
//   gravity-compensated jump-to-enemy + LeapTouch, shared logic with
//   CXenome::LeapTouch) across 20 decompiled functions - not
//   reproduced this session (same "no custom schedule tables"
//   simplification used throughout this project); relies on default
//   CBaseMonster melee capability/scheduling instead.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "weapons.h"

//=========================================================
// ammo_dmlsingle - CDMLSingleAmmo.
//=========================================================
class CDMLSingleAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/singlerocket.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PrecacheModel("models/singlerocket.mdl");
		PrecacheSound("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(1, "dml", 10) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_dmlsingle, CDMLSingleAmmo);

//=========================================================
// rustbitshot - CRustbitShot. A confirmed but previously
// unimplemented hitscan-style projectile for monster_rustbit/
// monster_rustbot (findings/entities/rustbot_family.md's
// Session-37/38 find, ShotTouch decompiled in Session 39, but its
// real spawn/calling site was never located across multiple prior
// sessions - see findings/open_items_audit_2026-09-05.md).
//
// Decompiled fresh this session: LINK @0x100b5680, vtable
// @0x10101530, Spawn @0x100b5760, Precache @0x100b5730, ShotTouch
// @0x100b5af0 (already named in the binary's own symbol table).
// Confirmed: sprite "sprites/bitboltsmall.spr", bbox
// (-1,-1,-1)/(1,1,1), MOVETYPE_FLY, SOLID_BBOX, kRenderGlow-style
// rendermode, takedamage=DAMAGE_NO. ShotTouch confirmed: ignores
// same-team/self hits, plays a world-impact sound
// ("RustBit_Projectile2.wav") plus a bullet-hole decal on a static
// hit, or deals skill-scaled damage via TakeDamage on a living target
// (playing "RustBit_Projectile1.wav" on a successful hit,
// "_Projectile2.wav" otherwise) - standard hitscan-projectile
// behavior, no special effects, matching the doc's own conclusion
// exactly.
//
// Where it's actually launched from was empirically re-checked this
// session via tools/mdl_inspect.py against the real rustbit.mdl: the
// "shootarm" (ACT_RANGE_ATTACK1) and "rapidfire" (ACT_RANGE_ATTACK2)
// sequences embed ONLY the confirmed hand-glow-sprite events (1/2 and
// 3/4 respectively) - there is no additional embedded event that
// could carry a shot-spawn call, and CRustbit::HandleAnimEvent's own
// decompiled switch (below) has no case beyond 1-6. The real
// projectile-launch site is therefore NOT in HandleAnimEvent at all -
// most likely dispatched directly from a custom AI schedule/task this
// project does not reproduce (same "no custom Schedule_t/Task_t
// tables" scope boundary already applied throughout this project).
// It is intentionally not launched from an animation event: the
// decompiled event switch proves that events 1-4 only maintain the hand
// sprites and events 5-6 are melee. Its custom schedule/task launch site
// remains unresolved.
//=========================================================
class CRustbitShot : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT ShotTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(rustbitshot, CRustbitShot);

void CRustbitShot::Spawn()
{
	Precache();

	pev->classname = MAKE_STRING("rustbitshot");
	SET_MODEL(ENT(pev), "sprites/bitboltsmall.spr");
	UTIL_SetSize(pev, Vector(-1, -1, -1), Vector(1, 1, 1));

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->takedamage = DAMAGE_NO;

	SetTouch(&CRustbitShot::ShotTouch);

	pev->nextthink = gpGlobals->time + 3.0;
	SetThink(&CRustbitShot::SUB_Remove);
}

void CRustbitShot::Precache()
{
	PrecacheModel("sprites/bitboltsmall.spr");
	PrecacheSound("rustbit/RustBit_Projectile1.wav");
	PrecacheSound("rustbit/RustBit_Projectile2.wav");
}

void CRustbitShot::ShotTouch(CBaseEntity* pOther)
{
	if (pOther->pev->owner == pev->owner)
		return;

	if (0 != pOther->pev->takedamage)
	{
		pOther->TakeDamage(pev, VARS(pev->owner), 6, DMG_BULLET); // skill-scaled amount approximated, see file header
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "rustbit/RustBit_Projectile1.wav", 1.0, ATTN_NORM);
	}
	else
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "rustbit/RustBit_Projectile2.wav", 1.0, ATTN_NORM);
	}

	UTIL_Remove(this);
}

//=========================================================
// monster_rustbit - CRustbit.
//=========================================================
#define RUSTBIT_AE_GLOW_LEFT_1 1
#define RUSTBIT_AE_GLOW_LEFT_2 3
#define RUSTBIT_AE_GLOW_RIGHT_1 2
#define RUSTBIT_AE_GLOW_RIGHT_2 4
#define RUSTBIT_AE_CLAW_LEFT 5
#define RUSTBIT_AE_CLAW_RIGHT 6

class CRustbit : public CBaseMonster
{
public:
	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_PREY; }
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void GibMonster() override;
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
	// BUG FIX (2026-09-05): scripted_sequence-driven "waiting" poses
	// get stomped mid-animation without this - see
	// findings/entities and the
	// feedback-setactivity-act-idle-stomps-scripted-sequence memory
	// note (same fix as CFriendlyGunman etc; covers CRustbitFriendly
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

private:
	void Claw();
	void EnsureHandGlow(bool rightHand);
	void RemoveHandGlows();
	CSprite* m_pRightGlow = nullptr;
	CSprite* m_pLeftGlow = nullptr;
};
LINK_ENTITY_TO_CLASS(monster_rustbit, CRustbit);

TYPEDESCRIPTION CRustbit::m_SaveData[] =
{
	DEFINE_FIELD(CRustbit, m_pRightGlow, FIELD_CLASSPTR),
	DEFINE_FIELD(CRustbit, m_pLeftGlow, FIELD_CLASSPTR),
};
IMPLEMENT_SAVERESTORE(CRustbit, CBaseMonster);

void CRustbit::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/rustbit.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 42));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = 40; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = Vector(0, 0, 40);

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;

	MonsterInit();
}

void CRustbit::Precache()
{
	PrecacheModel("models/rustbit.mdl");
	PrecacheModel("sprites/rustbitglow.spr");

	UTIL_PrecacheOther("rustbitshot");
}

void CRustbit::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case RUSTBIT_AE_CLAW_LEFT:
	case RUSTBIT_AE_CLAW_RIGHT:
		Claw();
		break;
	case RUSTBIT_AE_GLOW_LEFT_1:
	case RUSTBIT_AE_GLOW_LEFT_2:
		EnsureHandGlow(false);
		break;
	case RUSTBIT_AE_GLOW_RIGHT_1:
	case RUSTBIT_AE_GLOW_RIGHT_2:
		EnsureHandGlow(true);
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CRustbit::EnsureHandGlow(bool rightHand)
{
	CSprite*& pGlow = rightHand ? m_pRightGlow : m_pLeftGlow;
	if (!pGlow)
	{
		pGlow = CSprite::SpriteCreate("sprites/rustbitglow.spr", pev->origin, true);
		pGlow->SetTransparency(kRenderGlow, 255, 255, 255, 200, kRenderFxNoDissipation);
		pGlow->SetScale(0.5f);
		// Retail binds the right hand to attachment 1 and the left to 2.
		pGlow->SetAttachment(edict(), rightHand ? 1 : 2);
	}
	else
	{
		pGlow->SetBrightness(200);
	}
}

void CRustbit::Claw()
{
	CBaseEntity* pHurt = CheckTraceHullAttack(80.0f, 10, DMG_SLASH);
	if (pHurt)
	{
		pHurt->pev->punchangle.x = RANDOM_LONG(-10, 10);
		pHurt->pev->punchangle.z = RANDOM_LONG(-10, 10);
		UTIL_MakeVectors(pev->angles);
		pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100.0f;
	}
}

void CRustbit::RemoveHandGlows()
{
	if (m_pRightGlow)
		UTIL_Remove(m_pRightGlow);
	if (m_pLeftGlow)
		UTIL_Remove(m_pLeftGlow);
	m_pRightGlow = nullptr;
	m_pLeftGlow = nullptr;
}

void CRustbit::Killed(entvars_t* pevAttacker, int iGib)
{
	RemoveHandGlows();
	CBaseMonster::Killed(pevAttacker, iGib);
}

void CRustbit::GibMonster()
{
	RemoveHandGlows();
	CBaseMonster::GibMonster();
}

//=========================================================
// monster_rustbit_friendly - CRustbitFriendly (RUST7B+). Confirmed
// fresh this session (LINK @0x100b7da0, vtable @0x10101b70): Spawn
// and Precache are the exact same addresses as monster_rustbit's own
// (0x100b7750/0x100b7920) - a true shared-vtable variant, not a
// separate implementation. Classify (slot 8, @0x100b7e20) is the
// only override and returns the literal constant 3
// (CLASS_HUMAN_PASSIVE) - confirms
// findings/entities/gunner_family.md's "einziger Unterschied ist
// Classify()-Override" finding exactly.
//=========================================================
class CRustbitFriendly : public CRustbit
{
public:
	int Classify() override { return CLASS_HUMAN_PASSIVE; }
};
LINK_ENTITY_TO_CLASS(monster_rustbit_friendly, CRustbitFriendly);

//=========================================================
// monster_beak - CBeak.
//=========================================================
class CBeak : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_PLAYER_ALLY; }
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
};
LINK_ENTITY_TO_CLASS(monster_beak, CBeak);

void CBeak::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/beak.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 40));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 30; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = Vector(0, 0, 40);

	m_afCapability |= bits_CAP_MELEE_ATTACK1;

	MonsterInit();
}

void CBeak::Precache()
{
	PrecacheModel("models/beak.mdl");

	PrecacheSound("beak/attack1.wav");
	PrecacheSound("beak/suffer.wav");
	PrecacheSound("beak/alert1.wav");
	PrecacheSound("beak/pain1.wav");
	PrecacheSound("beak/die1.wav");
	PrecacheSound("beak/eating.wav");
	PrecacheModel("sprites/gibbeak.spr");
	PrecacheModel("sprites/gorebeak.spr");
}
