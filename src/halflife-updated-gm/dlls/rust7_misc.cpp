//=========================================================
// rust7c/rust7d's remaining classes with no SDK precedent:
// monster_xenome (CXenome, the "Emperor Xenome") and
// button_aiwallplug (CAiWallPlug). weapon_beamgun/weapon_gausspistol/
// weapon_minigun, this pass's other gaps, are already-deferred player
// weapons.
//
// Decompiled fresh from gunman.dll this session:
//   monster_xenome: LINK @0x100c9920, vtable @0x101043c8, Spawn
//     @0x100cad30, Precache @0x100cace0, Classify @0x100c9bd0.
//     Confirmed bbox (-16,-16,0)/(16,16,48), SOLID_SLIDEBOX, health
//     from a skill cvar. Spawn sets pev->rendermode = kRenderTransColor
//     with pev->renderamt = 0 - a genuinely, code-confirmed invisible
//     spawn state, matching findings/entity_review_list.csv's
//     "teilweise unsichtbar" ("Emperor Xenome") note exactly rather
//     than being a rendering bug. Classify itself is dynamic: it
//     compares the entity's CURRENT pev->renderamt against a float
//     threshold and returns CLASS_PLAYER_BIOWEAPON(12) once visible
//     enough, CLASS_ALIEN_BIOWEAPON(13) otherwise - a
//     visibility-linked hostility switch, reproduced here with the
//     confirmed structure but an approximated threshold (the exact
//     float constant wasn't resolved to a symbol this session).
//   button_aiwallplug: LINK @0x1007ad00, vtable @0x100f9b24, Spawn
//     @0x1007adc0, Precache @0x1007aed0, ObjectCaps (slot 14,
//     @0x1007ad50, confirmed literal return -1 - matches
//     findings/entity_review_list.csv's "ObjectCaps=FCAP_CONTINUOUS_USE-
//     artig" note, reproduced faithfully). Confirmed class name
//     "CAiWallPlug" (Save-string literal, via
//     @0x1007ad60's embedded save-field helper). Confirmed bbox
//     (-16,-16,0)/(16,16,16), MOVETYPE_FLY, SOLID_BBOX,
//     takedamage=DAMAGE_NO, body index 1, and both confirmed sounds
//     ("mainframe/aiplug_activate_gs.wav"/
//     "mainframe/aiplug_deactivate_gs.wav") plus a shared sound-group
//     precache call not individually decompiled this session (a
//     sentence group, per findings - matches this project's
//     established random_speaker/player_speaker sentence-group
//     idiom).
//
// Simplified relative to the original: neither class's real
// interactive behavior (CXenome's full combat/AI beyond the
// visibility-linked Classify, CAiWallPlug's actual Use-driven
// activate/deactivate state machine and its sentence-group playback)
// was decompiled in full this session - both rely on the confirmed
// Spawn/Precache/Classify/ObjectCaps behavior plus a straightforward
// toggle-and-fire-targets Use() for the wall plug, same
// simplification level used throughout this project for secondary
// interactive props and creatures.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

//=========================================================
// monster_xenome - CXenome ("Emperor Xenome").
//=========================================================
class CXenome : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override
	{
		// Confirmed dynamic, visibility-linked classify - see file
		// header. Exact original threshold float not resolved to a
		// symbol this session; approximated as "more than barely
		// visible".
		return pev->renderamt >= 32 ? CLASS_PLAYER_BIOWEAPON : CLASS_ALIEN_BIOWEAPON;
	}
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
LINK_ENTITY_TO_CLASS(monster_xenome, CXenome);

void CXenome::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/xenome.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 48));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 60; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->rendermode = kRenderTransColor;
	pev->renderamt = 0; // confirmed spawn-invisible, see file header
	pev->view_ofs = Vector(0, 0, 40);

	MonsterInit();
}

void CXenome::Precache()
{
	PrecacheModel("models/xenome.mdl");
	PrecacheModel("sprites/gibxeno.spr");
	PrecacheModel("sprites/gorexeno.spr");
}

//=========================================================
// button_aiwallplug - CAiWallPlug.
//=========================================================
class CAiWallPlug : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int ObjectCaps() override { return -1; } // confirmed literal, see file header
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

private:
	bool m_bActive = false;
};
LINK_ENTITY_TO_CLASS(button_aiwallplug, CAiWallPlug);

void CAiWallPlug::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/aiwallplug.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
	pev->body = 1;

	UTIL_SetOrigin(pev, pev->origin);
}

void CAiWallPlug::Precache()
{
	PrecacheModel("models/aiwallplug.mdl");
	PrecacheSound("mainframe/aiplug_activate_gs.wav");
	PrecacheSound("mainframe/aiplug_deactivate_gs.wav");
}

void CAiWallPlug::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_bActive = !m_bActive;
	EMIT_SOUND(ENT(pev), CHAN_VOICE, m_bActive ? "mainframe/aiplug_activate_gs.wav" : "mainframe/aiplug_deactivate_gs.wav", 1.0, ATTN_NORM);

	if (!FStringNull(pev->target))
		FireTargets(STRING(pev->target), pActivator, this, USE_TOGGLE, 0);
}
