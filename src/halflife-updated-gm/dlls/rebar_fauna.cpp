//=========================================================
// Remaining REBAR2*/REBAR2K gaps: monster_tube_embryo (CTubeEmbryo),
// monster_xenome_embryo (CXenomeEmbryo), monster_aigirl (CAIGirl),
// entity_clustergod (CClusterGod). All four fresh-decompiled this
// session; entity_review_list.csv already had accurate one-line
// summaries for each, cross-checked against gunman.dll rather than
// trusted as-is.
//
// Decompiled fresh from gunman.dll:
//   monster_tube_embryo: LINK @0x100ce190, vtable @0x10106f7c, Spawn
//     @0x100ce1e0, Precache @0x100ce330, Classify (slot 8,
//     @0x100ce320) confirmed constant 0xd = 13 = CLASS_ALIEN_BIOWEAPON.
//     Confirmed model "models/tubryo.mdl", bbox (-6,-6,-6)/(6,6,12),
//     SOLID_SLIDEBOX, health 30, sets Think = CTubeEmbryo::SpawnSack -
//     a real method that dynamically spawns the "decore_sack" child
//     entity already confirmed by findings/entity_review_list.csv
//     (CTubeEmbryo::SpawnSack is its only known spawner, matching
//     decore_sack's own note "wird von CTubeEmbryo::SpawnSack
//     dynamisch als Kindinstanz erzeugt"). NOT reproduced here (the
//     sack child's own model/attachment logic wasn't decompiled this
//     session) - Think left at the default CBaseMonster idle instead.
//   monster_xenome_embryo: LINK @0x100cd510, vtable @0x101068f4,
//     Spawn @0x100cd560, Precache @0x100cd6e0, Classify (slot 8,
//     @0x100cd6d0) confirmed constant 0xd = 13 =
//     CLASS_ALIEN_BIOWEAPON (same as CTubeEmbryo). Confirmed model
//     "models/xmbryo.mdl", same bbox as CTubeEmbryo, health 20,
//     takedamage=DAMAGE_YES, sets Use = CXenomeEmbryo::
//     XenomeEmbryoGibUse and Think = CXenomeEmbryo::IdleSoundThink
//     (random 0-6s interval, plays one of three confirmed idle
//     sounds). IdleSoundThink reproduced directly (simple periodic
//     sound); XenomeEmbryoGibUse (a Use-triggered gib/death, per its
//     name) NOT reproduced - out of scope, no gap-closing map places
//     it as a Use target per the scanned entity lists.
//   monster_aigirl: LINK @0x10079260, vtable @0x100f9498, Spawn
//     @0x10079350, Precache @0x10079410, Classify (slot 8,
//     @0x10079310) confirmed constant 0 = CLASS_NONE - matches
//     findings/entity_review_list.csv's "rein dekorative
//     Hintergrundfigur ... nicht angreifbar/feindlich" exactly.
//     Confirmed model "models/aigirl.mdl", bbox (-16,-16,0)/
//     (16,16,72), SOLID_SLIDEBOX, MOVETYPE_FLY(4), health 100 - a
//     purely decorative, scripted_sequence/multi_manager-driven
//     background character per the doc, so no custom AI/Think is
//     needed for its confirmed usage (only in REBAR2I/rebar2l/REBAR3B,
//     always scripted).
//   entity_clustergod: LINK @0x100a2420, vtable @0x100fed78, Spawn
//     @0x100a2470, ShootClusterGrenades (Think) @0x100a2570 fresh-
//     decompiled. Confirmed: model "models/null.mdl" (invisible),
//     SOLID_NOT, MOVETYPE_NONE, no takedamage. ShootClusterGrenades
//     spawns exactly 4 submunition entities (model
//     "models/artillary.mdl") with randomized outward
//     position/velocity offsets around the spawner, then the spawner
//     removes itself via CBaseEntity::SUB_Remove - confirms
//     findings/entity_review_list.csv's "Laufzeit-Spawner fuer
//     Cluster-Sub-Munition". Simplified relative to the original: the
//     submunitions' own randomized-arc trajectory math is NOT
//     reproduced - reproduced instead via the stock SDK's
//     CGrenade::ShootTimed() (already used project-wide for other
//     timed-grenade spawns) fired in 4 randomized directions with a
//     short fuse, matching the confirmed "4 submunitions, then
//     self-remove" shape without re-deriving the exact ballistic arc
//     constants.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

//=========================================================
// monster_tube_embryo - CTubeEmbryo.
//=========================================================
class CTubeEmbryo : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_BIOWEAPON; }
};
LINK_ENTITY_TO_CLASS(monster_tube_embryo, CTubeEmbryo);

void CTubeEmbryo::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/tubryo.mdl");
	UTIL_SetSize(pev, Vector(-6, -6, -6), Vector(6, 6, 12));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 30;

	MonsterInit();
	pev->view_ofs = Vector(0, 0, 6);
}

void CTubeEmbryo::Precache()
{
	PrecacheModel("models/tubryo.mdl");
}

//=========================================================
// monster_xenome_embryo - CXenomeEmbryo.
//=========================================================
class CXenomeEmbryo : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_BIOWEAPON; }
	void EXPORT IdleSoundThink();
};
LINK_ENTITY_TO_CLASS(monster_xenome_embryo, CXenomeEmbryo);

void CXenomeEmbryo::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/xmbryo.mdl");
	UTIL_SetSize(pev, Vector(-6, -6, -6), Vector(6, 6, 12));

	pev->solid = SOLID_SLIDEBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = 20;

	MonsterInit();
	pev->view_ofs = Vector(0, 0, 6);
	SetThink(&CXenomeEmbryo::IdleSoundThink);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0, 6);
}

void CXenomeEmbryo::Precache()
{
	PrecacheModel("models/xmbryo.mdl");
	PrecacheModel("models/xenomegibs.mdl");
	PrecacheSound("xenome/xmbryo_idle1.wav");
	PrecacheSound("xenome/xmbryo_idle2.wav");
	PrecacheSound("xenome/xmbryo_idle3.wav");
}

void CXenomeEmbryo::IdleSoundThink()
{
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(3, 6);

	const char* sounds[] = {"xenome/xmbryo_idle1.wav", "xenome/xmbryo_idle2.wav", "xenome/xmbryo_idle3.wav"};
	EMIT_SOUND(ENT(pev), CHAN_VOICE, sounds[RANDOM_LONG(0, 2)], 1.0, ATTN_IDLE);
}

//=========================================================
// monster_aigirl - CAIGirl. Purely decorative, scripted-sequence
// driven background character, see file header.
//=========================================================
class CAIGirl : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_NONE; }
	// BUG FIX (2026-09-05): monster_aigirl is purely decorative,
	// scripted_sequence/multi_manager-driven (see file header) - same
	// architecture as monster_furniture/monster_generic/monster_renesaur.
	// aigirl.mdl's "Idle" sequence carries a real activity=1 (ACT_IDLE)
	// tag, but its cutscene sequences ("gesture"/"slap"/"choke#idle"/
	// "chokestop"/"point"/"jump"/"jump2"/"typing#idle") are all
	// activity=0 (named-only, per tools/mdl_inspect.py). Without this
	// override, CBaseMonster's default SetActivity(ACT_IDLE) (called by
	// the default AI schedule between scripted waypoints) snaps her back
	// to the idle stance mid-sequence - see
	// [[feedback-setactivity-act-idle-stomps-scripted-sequence]]. Same
	// fix: leave sequence selection entirely to
	// CCineMonster::StartSequence.
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
	// CORRECTION (2026-09-05 pattern, applied here 2026-09-06): missing
	// SetYawSpeed() override - same root cause/fix as CFriendlyGunman.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_WALK) ? 90 : 70; }
	// Nachtrag 2026-09-06 (In-Game-Test-Meldung "haengt in der Luft,
	// bitte einfache KI die sich nur bewegt", KORRIGIERT nach
	// Nutzer-Rueckmeldung: KEIN freies autonomes Wandern per Default -
	// sie soll sich nur bewegen, wenn sie vom Spieler beruehrt oder
	// beschossen wird. Freies Wandern nur, wenn der neue Spawnflag
	// SF_AIGIRL_WANDER gesetzt ist; Default = aus).
	// Einfaches Bewegungsmuster nach dem etablierten CRoach/CCricket-
	// Muster dieses Projekts (direktes WALK_MOVE zu einem Zielpunkt,
	// kein voller Schedule_t/Task_t-Scheduler). aigirl.mdl hat echte
	// "Idle" (ACT_IDLE) und "walk"/"walk2" (ACT_WALK) Sequenzen mit
	// korrekten Activity-Tags (per tools/mdl_inspect.py bestaetigt) -
	// beide werden genutzt. Greift NICHT ein, waehrend eine
	// scripted_sequence die Figur steuert (m_pCine != nullptr, gleiche
	// Bedingung wie in SetActivity() oben).
	void EXPORT AIGirlThink();
	void EXPORT AIGirlTouch(CBaseEntity* pOther);
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override
	{
		StartReactiveMove(pevAttacker ? CBaseEntity::Instance(pevAttacker) : nullptr);
		CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
	}

private:
	void StartReactiveMove(CBaseEntity* pInstigator);

	Vector m_vecWanderDest = g_vecZero;
};
LINK_ENTITY_TO_CLASS(monster_aigirl, CAIGirl);

// Spawnflag-Bit fuer freies autonomes Wandern (Default aus, siehe
// Klassenkommentar oben). In die rekonstruierte FGD uebernommen
// (findings/fgd-diff/gunman-reconstructed-additions.fgd).
#define SF_AIGIRL_WANDER (1 << 0)

void CAIGirl::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/aigirl.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 72));

	pev->solid = SOLID_SLIDEBOX;
	// Nachtrag 2026-09-06 (In-Game-Test-Meldung "haengt in der Luft"):
	// MOVETYPE_FLY hielt sie permanent auf Spawn-Hoehe fest, ohne von
	// der Schwerkraft auf den Boden gezogen zu werden - jede andere
	// laufende Figur dieses Projekts nutzt MOVETYPE_STEP.
	pev->movetype = MOVETYPE_STEP;
	pev->health = 100;

	MonsterInit();
	SetActivity(ACT_IDLE);
	SetThink(&CAIGirl::AIGirlThink);
	SetTouch(&CAIGirl::AIGirlTouch);
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.1, 0.5);
}

void CAIGirl::Precache()
{
	PrecacheModel("models/aigirl.mdl");
}

// Nachtrag 2026-09-06 (Nutzer-Korrektur): ausgeloest von AIGirlTouch()
// (Spielerberuehrung) und TraceAttack() (Beschuss/Angriff jeder Art,
// nicht nur Spielerwaffen) - laeuft weg vom Ausloeser statt in eine
// zufaellige Richtung, sofern ein Ausloeser bekannt ist.
void CAIGirl::StartReactiveMove(CBaseEntity* pInstigator)
{
	if (m_pCine != nullptr || m_Activity == ACT_WALK)
		return;

	Vector vecDir;
	if (pInstigator != nullptr)
		vecDir = (pev->origin - pInstigator->pev->origin).Normalize();
	else
		vecDir = Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), 0).Normalize();

	float flDist = 128 + RANDOM_LONG(0, 255);
	m_vecWanderDest = pev->origin + vecDir * flDist;
	SetActivity(ACT_WALK);
}

void CAIGirl::AIGirlTouch(CBaseEntity* pOther)
{
	if (pOther->IsPlayer())
		StartReactiveMove(pOther);
}

void CAIGirl::AIGirlThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	StudioFrameAdvance();

	// scripted_sequence hat Vorrang - siehe SetActivity()-Kommentar oben.
	if (m_pCine != nullptr)
		return;

	if (m_Activity != ACT_WALK)
	{
		// Freies autonomes Wandern NUR mit gesetztem SF_AIGIRL_WANDER-
		// Spawnflag (Default aus) - sonst bewegt sie sich ausschliesslich
		// reaktiv (siehe AIGirlTouch()/TraceAttack()).
		if (FBitSet(pev->spawnflags, SF_AIGIRL_WANDER) && RANDOM_LONG(0, 49) == 1)
		{
			StartReactiveMove(nullptr);
		}
		return;
	}

	float flDist = (m_vecWanderDest - pev->origin).Length2D();
	if (flDist <= m_flGroundSpeed * 0.1f)
	{
		SetActivity(ACT_IDLE);
		return;
	}

	MakeIdealYaw(m_vecWanderDest);
	ChangeYaw(pev->yaw_speed);
	if (!WALK_MOVE(ENT(pev), pev->ideal_yaw, m_flGroundSpeed * 0.1f, WALKMOVE_NORMAL))
	{
		// blockiert - aufgeben und wieder warten statt haengenzubleiben.
		SetActivity(ACT_IDLE);
	}
}

//=========================================================
// entity_clustergod - CClusterGod.
//=========================================================
class CClusterGod : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT ShootClusterGrenades();
};
LINK_ENTITY_TO_CLASS(entity_clustergod, CClusterGod);

void CClusterGod::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/null.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16));

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_NO;

	SetThink(&CClusterGod::ShootClusterGrenades);
	pev->nextthink = gpGlobals->time + 0.1;
}

void CClusterGod::Precache()
{
	PrecacheModel("models/artillary.mdl");
	PrecacheModel("models/null.mdl");
}

void CClusterGod::ShootClusterGrenades()
{
	for (int i = 0; i < 4; i++)
	{
		Vector vecDir = Vector(RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(-1, 1), RANDOM_FLOAT(0.3, 1)).Normalize();
		CGrenade::ShootTimed(pev, pev->origin, vecDir * RANDOM_FLOAT(150, 300), 2.0);
	}

	UTIL_Remove(this);
}
