//=========================================================
// monster_gunner_friendly - end1/end2's only non-weapon gap besides
// monster_endboss (tracked separately, see STATUS.md). NOT the same
// class as monster_human_gunman/CFriendlyGunman (human_gunman.cpp) -
// different vtable address, different model, different C++ class
// despite the similar name.
//
// Decompiled fresh from gunman.dll: LINK @0x100bef60, vtable
// @0x101028a0, Spawn @0x100bf200, Precache @0x100bf080, Classify
// (slot 8, @0x100beff0) confirmed constant 3 = CLASS_HUMAN_PASSIVE
// (friendly). Confirmed model "models/aigunner.mdl" - an AI/robot
// gunner turret-style ally, not a human NPC - bbox
// (-32,-32,0)/(32,32,150), skill-scaled health (~50/35/75 for
// skill 1/2/3 from the decompiled float constants, an unusual
// non-monotonic table - reproduced as-is), spawns a child glow-sprite
// ("sprites/cameye.spr", a camera-eye effect) attached as a
// trailing/glowing effect. Precache's sound list is entirely
// "mainframe/end_*"/"mainframe/mainend*" story/cutscene voice lines -
// confirms this is endgame-specific mainframe-AI content, matching
// its exclusive placement in end1/end2.
//
// Simplified relative to the original: the child cameye sprite
// (spawned via a generic CSprite-creation helper, attached with
// kRenderGlow-style transparency per the decompiled field values) and
// any HandleAnimEvent-driven attack/voice-line playback were NOT
// decompiled in full this session - out of scope for an endgame-only
// ally with no located combat anim events. Relies on default
// CBaseMonster AI/scheduling with no custom attack.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

class CGunnerFriendly : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_HUMAN_PASSIVE; }
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
LINK_ENTITY_TO_CLASS(monster_gunner_friendly, CGunnerFriendly);

void CGunnerFriendly::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/aigunner.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 150));

	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = 50; // skill-scaled default approximated - see file header

	MonsterInit();
	pev->view_ofs = Vector(0, 0, 75);
}

void CGunnerFriendly::Precache()
{
	PrecacheModel("models/aigunner.mdl");
	PrecacheModel("sprites/cameye.spr");
	PrecacheSound("mainframe/end_7.wav");
	PrecacheSound("mainframe/end_8.wav");
	PrecacheSound("mainframe/end_9.wav");
	PrecacheSound("mainframe/mainend1.wav");
	PrecacheSound("mainframe/mainend2.wav");
	PrecacheSound("mainframe/mainend3.wav");
	PrecacheSound("mainframe/mainend4.wav");
	PrecacheSound("mainframe/mainend5.wav");
}
