//=========================================================
// monster_human_gunman - CFriendlyGunman. A friendly, talking, armed
// human ally ("Human Gunman Friendly" per the FGD) - Gunman
// Chronicles' equivalent of stock HL's CBarney, built the same way:
// a CTalkMonster subclass using the SDK's existing generic sentence-
// group system (TLK_ANSWER/TLK_QUESTION/etc., see talkmonster.h),
// except Gunman exposes all 19 sentence-group names as map keyvalues
// ("gn_*") instead of hardcoding them per monster type, plus two
// extra groups (gn_kill/gn_attack) the stock TLK_ enum has no slot
// for.
//
// Decompiled from gunman.dll: constructor @0x100ae360 (vtable
// 0x101009f0), Spawn @0x100ae1d0, Precache @0x100ae300, KeyValue
// @0x100ae750 (the field-name-to-offset mapping below was verified
// against the FGD's exact 21-keyvalue list, all present and in
// order), Save @0x100add00 (class name literal "CFriendlyGunman"),
// Classify @0x100adda0 (constant 3 = CLASS_HUMAN_PASSIVE, matching
// the FGD title "Human Gunman Friendly").
//
// Simplified relative to the original (not attempted this pass, given
// the size of the class already covered): CBarney-style custom
// Schedule_t/Task_t tables for polished draw/follow animations (relies
// on CTalkMonster's default scheduling instead - still follows/fights,
// just less choreographed); a dedicated ranged-attack implementation
// tailored to Gunman's own animation events (Precache confirms an
// "events/monstershotgun.sc" playback event exists, suggesting a
// shotgun-style attack - approximated here with a generic FireBullets
// shot on the same anim event convention as hgrunt.cpp, not verified
// against the actual HandleAnimEvent ids); the two Gunman-only
// gn_kill/gn_attack sentence groups are parsed and stored but not
// wired into any playback logic (no stock TLK_ slot exists for them
// and their consumption site wasn't decompiled).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "weapons.h"
#include "soundent.h"

#define GUNMAN_AE_SHOOT 3

class CFriendlyGunman : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_HUMAN_PASSIVE; }
	void SetYawSpeed() override;
	void SetActivity(Activity NewActivity) override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;
	void TalkInit();
	void Shoot();

	// BUG FIX (2026-09-06, user request: "monster_gunman als Follower wie
	// Barney usw. hinzufuegen"): CTalkMonster::FollowerUse() (already
	// wired via SetUse() in Spawn(), and the TLK_USE/TLK_UNUSE sentence
	// groups were already correctly hooked up in TalkInit()) was
	// unreachable - CBaseEntity's default ObjectCaps() has no use-capability
	// bit at all, and CBasePlayer::PlayerUse()'s +use dispatch gates on
	// FCAP_IMPULSE_USE|FCAP_CONTINUOUS_USE|FCAP_ONOFF_USE before ever
	// calling Use(). Same root cause/fix as the vehicle_tank mount bug
	// earlier this session, and the same pattern stock CBarney/CScientist
	// both already have.
	int ObjectCaps() override { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }

	bool KeyValue(KeyValueData* pkvd) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	int m_iHealthValue = 80;
	int m_iGunState = 1; // 1 = Drawn, 2 = None (per FGD; no "Holstered" choice for this variant)

	int m_iszAnswer = 0;
	int m_iszQuestion = 0;
	int m_iszIdle = 0;
	int m_iszStare = 0;
	int m_iszUseSentence = 0;
	int m_iszUnUseSentence = 0;
	int m_iszStop = 0;
	int m_iszNoShoot = 0;
	int m_iszHello = 0;
	int m_iszPlHurt1 = 0;
	int m_iszPlHurt2 = 0;
	int m_iszPlHurt3 = 0;
	int m_iszPHello = 0;
	int m_iszPIdle = 0;
	int m_iszPQuestion = 0;
	int m_iszSmell = 0;
	int m_iszWound = 0;
	int m_iszMortal = 0;
	int m_iszKill = 0;	 // Gunman-only, no stock TLK_ slot - see file header comment
	int m_iszAttack = 0; // Gunman-only, no stock TLK_ slot - see file header comment
};
LINK_ENTITY_TO_CLASS(monster_human_gunman, CFriendlyGunman);

TYPEDESCRIPTION CFriendlyGunman::m_SaveData[] =
		{
			DEFINE_FIELD(CFriendlyGunman, m_iHealthValue, FIELD_INTEGER),
			DEFINE_FIELD(CFriendlyGunman, m_iGunState, FIELD_INTEGER),
			DEFINE_FIELD(CFriendlyGunman, m_iszAnswer, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszQuestion, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszIdle, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszStare, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszUseSentence, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszUnUseSentence, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszStop, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszNoShoot, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszHello, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszPlHurt1, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszPlHurt2, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszPlHurt3, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszPHello, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszPIdle, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszPQuestion, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszSmell, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszWound, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszMortal, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszKill, FIELD_STRING),
			DEFINE_FIELD(CFriendlyGunman, m_iszAttack, FIELD_STRING),
		};

IMPLEMENT_SAVERESTORE(CFriendlyGunman, CTalkMonster);

bool CFriendlyGunman::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "healthvalue"))
	{
		m_iHealthValue = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gunstate"))
	{
		m_iGunState = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_answer"))
	{
		m_iszAnswer = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_question"))
	{
		m_iszQuestion = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_idle"))
	{
		m_iszIdle = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_stare"))
	{
		m_iszStare = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_use"))
	{
		m_iszUseSentence = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_unuse"))
	{
		m_iszUnUseSentence = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_stop"))
	{
		m_iszStop = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_noshoot"))
	{
		m_iszNoShoot = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_hello"))
	{
		m_iszHello = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_plhurt1"))
	{
		m_iszPlHurt1 = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_plhurt2"))
	{
		m_iszPlHurt2 = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_plhurt3"))
	{
		m_iszPlHurt3 = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_phello"))
	{
		m_iszPHello = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_pidle"))
	{
		m_iszPIdle = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_pquestion"))
	{
		m_iszPQuestion = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_smell"))
	{
		m_iszSmell = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_wound"))
	{
		m_iszWound = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_mortal"))
	{
		m_iszMortal = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_kill"))
	{
		m_iszKill = ALLOC_STRING(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "gn_attack"))
	{
		m_iszAttack = ALLOC_STRING(pkvd->szValue);
		return true;
	}

	return CTalkMonster::KeyValue(pkvd);
}

void CFriendlyGunman::TalkInit()
{
	CTalkMonster::TalkInit();

	m_szGrp[TLK_ANSWER] = STRING(m_iszAnswer);
	m_szGrp[TLK_QUESTION] = STRING(m_iszQuestion);
	m_szGrp[TLK_IDLE] = STRING(m_iszIdle);
	m_szGrp[TLK_STARE] = STRING(m_iszStare);
	m_szGrp[TLK_USE] = STRING(m_iszUseSentence);
	m_szGrp[TLK_UNUSE] = STRING(m_iszUnUseSentence);
	m_szGrp[TLK_STOP] = STRING(m_iszStop);
	m_szGrp[TLK_NOSHOOT] = FStringNull(m_iszNoShoot) ? "GN_DONTSHOOT" : STRING(m_iszNoShoot);
	m_szGrp[TLK_HELLO] = STRING(m_iszHello);
	m_szGrp[TLK_PLHURT1] = STRING(m_iszPlHurt1);
	m_szGrp[TLK_PLHURT2] = STRING(m_iszPlHurt2);
	m_szGrp[TLK_PLHURT3] = STRING(m_iszPlHurt3);
	m_szGrp[TLK_PHELLO] = STRING(m_iszPHello);
	m_szGrp[TLK_PIDLE] = STRING(m_iszPIdle);
	m_szGrp[TLK_PQUESTION] = STRING(m_iszPQuestion);
	m_szGrp[TLK_SMELL] = STRING(m_iszSmell);
	m_szGrp[TLK_WOUND] = STRING(m_iszWound);
	m_szGrp[TLK_MORTAL] = STRING(m_iszMortal);
}

void CFriendlyGunman::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/gunmantrooper.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = (float)m_iHealthValue;
	pev->view_ofs = Vector(0, 0, 50);
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_RANGE_ATTACK1;

	MonsterInit();
	SetUse(&CFriendlyGunman::FollowerUse);

	// gunstate 2 = "None" (no visible weapon), body 1; otherwise drawn, body 0.
	SetBodygroup(1, m_iGunState == 2 ? 1 : 0);
}

void CFriendlyGunman::SetYawSpeed()
{
	// CBaseMonster deliberately provides a no-op default. Without a class
	// override the scripted route updates ideal_yaw, but ChangeYaw() cannot
	// rotate the Gunman before it moves.
	pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70;
}

// BUG FIX (2026-09-05, live gameplay report): "die gunman ... in einer
// wartenden Animation ... beenden diese und beginnen von vorne" - this
// NPC is commonly puppeted by scripted_sequence "waiting"/idle-pose
// loops across this project's maps (e.g. mayan1's self-retriggering
// "fart1" scripted_sequence, m_iszPlay="onground#idle", a named,
// non-ACT_IDLE-tagged sequence). Without this override, CBaseMonster's
// default SetActivity(ACT_IDLE) (called by the default AI schedule
// between scripted waypoints, even for an otherwise full combat/talk
// monster) stomps pev->sequence mid-animation, cutting the wait pose
// short and restarting it from frame 0 - exactly the reported symptom.
// Same root cause and fix as CFurniture/CGenericMonster/CRenesaur/
// CAIGirl (see findings/entities and the
// feedback-setactivity-act-idle-stomps-scripted-sequence memory note).
// Normal combat/talk activity switching outside scripted segments is
// unaffected.
//
// Nachtrag 2026-09-06 (Live-Report "gunman haben wieder das gleiche
// Animationsproblem"): mayan1's "fart1" scripted_sequence retriggert
// sich selbst (target==targetname). CCineMonster::Use() (scripted.cpp)
// startet eine bereits bekannte Zielentity dabei NIE sofort neu, sondern
// immer mit 0.05s Verzoegerung (Stock-SDK-Verhalten, unveraendert) - in
// dieser kurzen Luecke ist m_pCine bereits null, obwohl die Figur
// eigentlich weiter "im Skript" wartet. Der reine m_pCine-Check greift
// dort nicht mehr durch, der Sprung war zurueck. Anders als bei
// CFurniture (siehe [[feedback-setactivity-guard-per-class-only]]) darf
// der Guard hier NICHT unbedingt gemacht werden - CFriendlyGunman ist
// ein echtes, dauerhaft eigenstaendig laufendes KI-/Talk-Monster
// (Follower, Kampf, echte ACT_IDLE-Sequenzen), ein unbedingtes Abfangen
// wuerde jede normale Idle-Aktivitaetsumschaltung ausserhalb von Cines
// kaputt machen. Stattdessen ueberbrueckt m_flCineGraceEnd
// (basemonster.h, von CineCleanup() gesetzt) genau die 0.05s-Luecke,
// ohne echtes freies Verhalten zu beeinflussen.
void CFriendlyGunman::SetActivity(Activity NewActivity)
{
	if ((NewActivity == ACT_IDLE || NewActivity == ACT_RESET) && (m_pCine != nullptr || gpGlobals->time < m_flCineGraceEnd))
	{
		m_Activity = NewActivity;
		m_IdealActivity = NewActivity;
		return;
	}
	CTalkMonster::SetActivity(NewActivity);
}

void CFriendlyGunman::Precache()
{
	PrecacheModel("models/gunmantrooper.mdl");
	PrecacheEvent(1, "events/monstershotgun.sc");
	PrecacheModel("sprites/gorehuman.spr");
	PrecacheModel("sprites/gibhuman.spr");

	PrecacheSound("barney/ba_pain1.wav");
	PrecacheSound("barney/ba_pain2.wav");
	PrecacheSound("barney/ba_pain3.wav");
	PrecacheSound("barney/ba_die1.wav");
	PrecacheSound("barney/ba_die2.wav");
	PrecacheSound("barney/ba_die3.wav");

	TalkInit();
	CTalkMonster::Precache();
}

void CFriendlyGunman::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case GUNMAN_AE_SHOOT:
		Shoot();
		break;
	default:
		CTalkMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CFriendlyGunman::Shoot()
{
	if (!m_hEnemy)
		return;

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5);
}
