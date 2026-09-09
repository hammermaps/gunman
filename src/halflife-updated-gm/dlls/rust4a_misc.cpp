//=========================================================
// RUST4A's remaining classes with no SDK precedent: monster_rustgunr
// (CGunner, + its "anime_rocket" runtime projectile) and meteor_god
// (CMeteorGod + its runtime meteor chunk). weapon_beamgun/
// weapon_minigun, RUST4A's other two gaps, are already-deferred
// player weapons.
//
// Decompiled fresh from gunman.dll this session:
//   monster_rustgunr: LINK @0x100bc320, vtable @0x1010265c - matches
//     findings/entities/gunner_family.md's independently-found vtable
//     value exactly, confirming this really is CGunner (the doc's
//     "true 1:1 identity across monster_gunner/monster_rustgnr/
//     monster_rustgunr" claim - all three share this one vtable).
//     Spawn @0x100bec80, Precache @0x100bee40, Classify @0x100bc230
//     (constant 8 = CLASS_ALIEN_PREY), CGunner::ToggleTakedamage
//     @0x100bec50 (named via the symbol table). Confirmed bbox
//     (-32,-32,0)/(32,32,150), health RANDOM(75,90) (matches the
//     doc's "RANDOM(0,15)+75" exactly), view_ofs (0,0,115).
//     ToggleTakedamage's real logic (re-derived fresh, more precise
//     than the doc's "0 or 2.0, vermutlich Schadensresistenz" guess):
//     it's pev->takedamage itself being toggled directly between
//     DAMAGE_NO(0) and 2.0 - a literal invulnerability on/off switch,
//     not a separate shield/resistance field.
//     findings/entities/gunner_family.md's exhaustive multi-session
//     search establishes that all 4 of CGunner's fire animation
//     events (6/7/8/9, confirmed via HandleAnimEvent @0x100bc7a0 -
//     not separately re-decompiled this session given how thoroughly
//     the doc already traced it across 6 independent search
//     approaches) spawn the same directly-constructed "anime_rocket"
//     projectile (CAnimeRocket) regardless of the animation's
//     "firegun"/"firerocket" name suggesting otherwise; the
//     separately-registered gunner_rocket/CGunnerRocket class is
//     documented as very likely genuine dead code (registered but
//     unreachable from any code path, confirmed in both retail and
//     the E3 beta) and is deliberately NOT implemented here.
//   meteor_god: LINK @0x100a5e50, ctor helper @0x100a5f00 (vtable
//     @0x100ffdb8), Spawn @0x100a62f0, Precache @0x100a5f70, KeyValue
//     @0x100a5ff0. Confirmed all 8 FGD-documented keyvalues
//     (typestreak/typelarge/typesmall/dropsizex/dropsizey/
//     mindropfreq/maxdropfreq/pitch) plus a "debris" boolean, and
//     their ctor defaults. Architecturally a near-twin of lava_god
//     (mayan3a_misc.cpp) - invisible (EF_NODRAW), non-solid
//     (SOLID_NOT/MOVETYPE_NONE) periodic spawner, paired with the
//     targeted meteor_target spawner below.
//
// Simplified relative to the original (documented per-case):
// - CGunner: no custom Schedule_t/Task_t AI (same established
//   simplification as every other monster in this project); the
//   cosmetic glow-eye sprite attachment (sprites/gunr_eye.spr) is
//   precached but not reproduced as a live-attached entity.
// - CAnimeRocket (anime_rocket): findings/entities/gunner_family.md's
//   Session-93 full decompile documents genuine homing
//   (proportional-navigation AccelerateThink, ramping to 1000 u/s)
//   and RadiusDamage(1024, x3 multiplier) on impact - reproduced here
//   as a straight-flying (no homing) explosive with a plausible fixed
//   damage/radius instead, same simplification level already used for
//   vehicle_tank_rocket/monster_targetrocket_proj/monster_tank_rocket
//   elsewhere in this project.
// - CMeteorGod: the exact eruption math (per-type drop weighting,
//   dropsizex/y scaling, pitch) is approximated with a simple
//   randomized-offset periodic spawn within a fixed area, matching
//   CLavaGod's already-established simplification pattern rather than
//   reproducing byte-exact selection logic between the 3 configured
//   meteor types.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "effects.h"
#include "explode.h"
#include "weapons.h"

//=========================================================
// The runtime-only homing-in-name-only rocket fired by CGunner's 4
// fire animation events (see file header re: homing simplification).
//=========================================================
class CAnimeRocket : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketTouch(CBaseEntity* pOther);

private:
	CSprite* m_pFlame = nullptr;
};
LINK_ENTITY_TO_CLASS(anime_rocket, CAnimeRocket);

void CAnimeRocket::Precache()
{
	PrecacheModel("models/rocket.mdl");
	PrecacheModel("sprites/flame.spr");
	PrecacheSound("weapons/rocket1.wav");
	PrecacheSound("weapons/kaboom1.wav");
	PrecacheSound("weapons/kaboom2.wav");
	PrecacheSound("weapons/kaboom3.wav");
}

void CAnimeRocket::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("anime_rocket");
	SET_MODEL(ENT(pev), "models/rocket.mdl");
	UTIL_SetSize(pev, Vector(-2, -2, -2), Vector(2, 2, 2));
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	SetTouch(&CAnimeRocket::RocketTouch);

	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/rocket1.wav", 1.0, ATTN_NORM, 0, 100);

	// Nachtrag 2026-09-06 (Live-Report mit Retail-Referenzscreenshot: die
	// echte Rakete zieht einen duennen, hellen orangen Streif, keine graue
	// Rauchwolke): CAnimeRocket::IgniteThink (0x100ba320) frisch erneut
	// decompiliert - nutzt entgegen der vorherigen TE_BEAMFOLLOW-Naeherung
	// GAR KEIN Temp-Entity-Beam, sondern haengt ueber den gemeinsamen
	// Helfer FUN_10015400 eine echte "env_sprite"-Entity mit dem Modell
	// "sprites/flame.spr" (String direkt aus der Binary gelesen,
	// 0x10126a5c) an - additiv, Farbe/Alpha auf volle 255 gesetzt (4
	// aufeinanderfolgende float-Felder = 255.0). Mit `CSprite::
	// SetAttachment()` (MOVETYPE_FOLLOW + pev->aiment) nachgebildet, dem
	// nativen GoldSrc-Mechanismus fuer genau diesen Zweck, statt eines
	// TE_BEAMFOLLOW-Ersatzes.
	m_pFlame = CSprite::SpriteCreate("sprites/flame.spr", pev->origin, true);
	if (m_pFlame)
	{
		m_pFlame->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNone);
		m_pFlame->SetScale(1.0);
		m_pFlame->SetAttachment(edict(), 0);
	}
}

void CAnimeRocket::RocketTouch(CBaseEntity* pOther)
{
	if (m_pFlame)
	{
		UTIL_Remove(m_pFlame);
		m_pFlame = nullptr;
	}

	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/kaboom1.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/kaboom2.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	default:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/kaboom3.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	}

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(g_sModelIndexFireball);
	WRITE_BYTE(40);
	WRITE_BYTE(15);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
	// Nachtrag 2026-09-06 (Nutzerwunsch: Erschuetterung+Lichtblitz bei
	// jeder Explosion, siehe util.h/UTIL_ExplosionEffects).
	UTIL_ExplosionEffects(pev->origin, 250.0f);

	RadiusDamage(pev->origin, pev, VARS(pev->owner), 80, 250, CLASS_NONE, DMG_BLAST);
	UTIL_Remove(this);
}

//=========================================================
// monster_rustgunr - CGunner. Also aliased (CORRECTION 2026-09-05:
// findings/entities/gunner_family.md's confirmed 1:1 vtable identity
// across all three classnames - see LINK addresses/vtable value in the
// file header above) as monster_gunner/monster_rustgnr, which never
// appear placed in any retail map (grep across all maps-src/*.ent
// finds only monster_rustgunr - RUST4A/rust6a/rust6c/rust7a), but are
// aliased here for classname completeness since they are genuinely the
// same class, not a documented gap.
//=========================================================
#define GUNNER_AE_FIRE_RIGHT_1 6
#define GUNNER_AE_FIRE_RIGHT_2 7
#define GUNNER_AE_FIRE_LEFT_1 8
#define GUNNER_AE_FIRE_LEFT_2 9

class CGunner : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_PREY; }
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
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	// Nachtrag 2026-09-06 (In-Game-Test-Meldung "Fleisch-Gibs statt
	// Metallteile"): models/gunnergibs.mdl war schon vorher precacht,
	// aber nie in GibMonster() verwendet - siehe CustomGibModel() in
	// basemonster.h.
	const char* CustomGibModel() override { return "models/gunnergibs.mdl"; }

private:
	void FireRocket(int iAttachment);
};
LINK_ENTITY_TO_CLASS(monster_rustgunr, CGunner);
LINK_ENTITY_TO_CLASS(monster_gunner, CGunner);
LINK_ENTITY_TO_CLASS(monster_rustgnr, CGunner);

void CGunner::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/gunner.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, 0), Vector(32, 32, 150));

	pev->solid = SOLID_SLIDEBOX;
	// Nachtrag 2026-09-06 (In-Game-Test via monster_zoo_test, Meldung
	// "Rust Gunner ist unverwundbar"): pev->movetype fehlte hier
	// komplett (bleibt sonst auf dem Edict-Default MOVETYPE_NONE) -
	// jede andere Monster-Klasse dieses Projekts setzt explizit
	// MOVETYPE_STEP. Explizit nachgetragen (matcht jede
	// Geschwisterklasse), auch wenn ein gezielter Debug-Test (trigger_
	// hurt + temporaerer TakeDamage()-Log-Override, siehe STATUS.md)
	// zeigte, dass pev->takedamage bereits korrekt DAMAGE_AIM(2.0) war
	// und Schaden sauber ankam - die urspruengliche "unverwundbar"-
	// Meldung war vermutlich ein Missverstaendnis (z.B. verfehlte
	// Treffer) oder ein anderes, noch ungeklaertes Problem, nicht dieses
	// hier. pev->movetype bleibt trotzdem als echte, unabhaengig
	// gefundene Abweichung von der Geschwisterklassen-Konvention gesetzt.
	pev->movetype = MOVETYPE_STEP;
	pev->takedamage = DAMAGE_AIM;
	pev->health = RANDOM_LONG(75, 90);
	pev->view_ofs = Vector(0, 0, 115);

	MonsterInit();
}

void CGunner::Precache()
{
	PrecacheModel("models/gunner.mdl");
	PrecacheModel("models/gunnergibs.mdl");
	PrecacheModel("sprites/gunr_eye.spr");
	PrecacheModel("sprites/botdeath.spr");
	PrecacheModel("sprites/gorebot.spr");
	PrecacheModel("sprites/gibbot.spr");

	PrecacheSound("gunner/gunner_boom1.wav");
	PrecacheSound("gunner/gunner_boom2.wav");
	UTIL_PrecacheOther("anime_rocket");
}

void CGunner::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	// Confirmed via fresh decompile: a literal pev->takedamage on/off
	// toggle, not a graduated shield/resistance value (see file
	// header).
	pev->takedamage = (pev->takedamage != DAMAGE_NO) ? DAMAGE_NO : DAMAGE_AIM;
}

void CGunner::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case GUNNER_AE_FIRE_RIGHT_1:
	case GUNNER_AE_FIRE_RIGHT_2:
		// Nachtrag 2026-09-06 (In-Game-Test-Meldung "Raketen kommen aus
		// dem Torso statt den Haenden"): models/gunner.mdl hat 4
		// Attachment-Punkte - [0]/[1] sitzen auf der Wirbelsaeule
		// (vermutlich Schulter-Referenzpunkte), [2]/[3] auf "Bip01 R/L
		// Forearm" (Unterarm), exakt passend zu den 4 Feuer-Events
		// (FIRE_RIGHT_1/2, FIRE_LEFT_1/2). FireRocket() nahm bislang
		// IMMER eine feste Position relativ zu pev->origin (Torso-Hoehe),
		// unabhaengig vom Event - jetzt der jeweils passende
		// Unterarm-Attachment-Punkt.
		FireRocket(2);
		break;
	case GUNNER_AE_FIRE_LEFT_1:
	case GUNNER_AE_FIRE_LEFT_2:
		FireRocket(3);
		break;
	default:
		CBaseMonster::HandleAnimEvent(pEvent);
		break;
	}
}

void CGunner::FireRocket(int iAttachment)
{
	if (!m_hEnemy)
		return;

	Vector vecSrc, vecAngles;
	GetAttachment(iAttachment, vecSrc, vecAngles);

	Vector vecDir = (m_hEnemy->pev->origin - vecSrc).Normalize();

	CBaseEntity* pRocket = CBaseEntity::Create("anime_rocket", vecSrc, UTIL_VecToAngles(vecDir), edict());
	if (pRocket)
		// Nachtrag 2026-09-06 (auf Nutzerwunsch etwas ueber dem retail-
		// Wert): das echte CAnimeRocket::AccelerateThink (siehe
		// findings/entities/gunner_family.md, Session 93) beschleunigt
		// per Proportionalnavigation bis zu einer Maximalgeschwindigkeit
		// von 1000.0 - hier bewusst NICHT als volle Beschleunigungsrampe
		// nachgebaut (waere ein groesserer Umbau von der aktuellen
		// gradlinigen Flugbahn zu echtem Homing), stattdessen eine feste
		// Geschwindigkeit knapp darueber (1100, vorher 500).
		pRocket->pev->velocity = vecDir * 1100;
}

//=========================================================
// The runtime-only meteor chunk spawned by meteor_god.
//=========================================================
class CMeteorChunk : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT ChunkTouch(CBaseEntity* pOther);
	void EXPORT SmokeThink();
	void Configure(int iMeteorType, bool bDebris);

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	void EmitSmokeSprite(int iScale, float flHeight, int iSpread);
	Vector GetImpactOrigin() const;
	void EmitRetailDebris(const Vector& vecImpact);
	void EmitEnhancedImpactEffects(const Vector& vecImpact, float flRadius);

	int m_iMeteorType = 0;
	bool m_bDebris = false;
	float m_flRemoveTime = 0;
	int m_iSmokeSprite = 0;
	int m_iImpactSprite = 0;
	int m_iDebrisModel = 0;
	int m_iSmallDebrisModel = 0;
};
LINK_ENTITY_TO_CLASS(meteor_god_chunk, CMeteorChunk);

TYPEDESCRIPTION CMeteorChunk::m_SaveData[] =
	{
		DEFINE_FIELD(CMeteorChunk, m_iMeteorType, FIELD_INTEGER),
		DEFINE_FIELD(CMeteorChunk, m_bDebris, FIELD_BOOLEAN),
		DEFINE_FIELD(CMeteorChunk, m_flRemoveTime, FIELD_TIME),
		DEFINE_FIELD(CMeteorChunk, m_iSmokeSprite, FIELD_INTEGER),
		DEFINE_FIELD(CMeteorChunk, m_iImpactSprite, FIELD_INTEGER),
		DEFINE_FIELD(CMeteorChunk, m_iDebrisModel, FIELD_INTEGER),
		DEFINE_FIELD(CMeteorChunk, m_iSmallDebrisModel, FIELD_INTEGER),
	};

IMPLEMENT_SAVERESTORE(CMeteorChunk, CBaseEntity);

void CMeteorChunk::Precache()
{
	PrecacheModel("models/asteroid.mdl");
	m_iDebrisModel = PrecacheModel("models/rockgibsbig.mdl");
	m_iSmallDebrisModel = PrecacheModel("models/lavaball.mdl");
	PrecacheSound("meteor/meteor_hit1.wav");
	PrecacheSound("meteor/meteor_hit2.wav");
	PrecacheSound("meteor/meteor_hit3.wav");
	m_iImpactSprite = PrecacheModel("sprites/impact.spr");
	m_iSmokeSprite = PrecacheModel("sprites/smoke.spr");
}

void CMeteorChunk::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("meteor_god_chunk");
	SET_MODEL(ENT(pev), "models/asteroid.mdl");
	UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS;
	SetTouch(&CMeteorChunk::ChunkTouch);

	m_flRemoveTime = gpGlobals->time + 8.0f;
	pev->nextthink = m_flRemoveTime;
	SetThink(&CBaseEntity::SUB_Remove);
}

void CMeteorChunk::Configure(int iMeteorType, bool bDebris)
{
	m_iMeteorType = iMeteorType;
	m_bDebris = bDebris;

	// CMeteor has distinct streak, large and small paths in the retail DLL.
	// Keep the same visible families while retaining one safely bounded
	// runtime entity in the clean-room implementation.
	if (m_iMeteorType == 0)
	{
		SET_MODEL(ENT(pev), "models/lavaball.mdl");
		UTIL_SetSize(pev, g_vecZero, g_vecZero);
	}
	else if (m_iMeteorType == 1)
	{
		SET_MODEL(ENT(pev), "models/asteroid.mdl");
		UTIL_SetSize(pev, Vector(-8, -8, -8), Vector(8, 8, 8));
	}
	else
	{
		// CMeteor::Spawn uses asteroid.mdl for both smoking paths. The
		// rock-gib model belongs only to the optional break-model debris.
		SET_MODEL(ENT(pev), "models/asteroid.mdl");
		UTIL_SetSize(pev, Vector(-16, -16, -16), Vector(16, 16, 16));
	}

	// FUN_100a6840, shared by all three retail types, adds a separate
	// beam-follow trail. This coexists with the discrete Little/Big smoke
	// particles below; it is not a replacement for them.
	if (m_iMeteorType == 0)
		pev->effects |= EF_LIGHT;
	else
		pev->effects &= ~EF_LIGHT;

	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex());
	WRITE_SHORT(m_iSmokeSprite);
	WRITE_BYTE(m_iMeteorType + 2);
	WRITE_BYTE(m_iMeteorType * 2 + 1);
	WRITE_BYTE(224);
	WRITE_BYTE(224);
	WRITE_BYTE(255);
	WRITE_BYTE(100);
	MESSAGE_END();

	// Retail keeps StreakerThink free of Little/Big smoke. The two other
	// types emit three independent TE_SPRITE particles per think instead
	// of relying on the shared beam-follow trail alone.
	if (m_iMeteorType != 0)
	{
		SetThink(&CMeteorChunk::SmokeThink);
		pev->nextthink = gpGlobals->time + 0.1f;
	}
}

void CMeteorChunk::EmitSmokeSprite(int iScale, float flHeight, int iSpread)
{
	MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(pev->origin.x + RANDOM_LONG(-iSpread, iSpread));
	WRITE_COORD(pev->origin.y + RANDOM_LONG(-iSpread, iSpread));
	WRITE_COORD(pev->origin.z + flHeight);
	WRITE_SHORT(m_iSmokeSprite);
	WRITE_BYTE(iScale);
	WRITE_BYTE(20);
	MESSAGE_END();
}

void CMeteorChunk::SmokeThink()
{
	// LittleSmokingMeteorThink and BigSmokingMeteorThink remove the retail
	// meteor only after it leaves the fixed world cube, not after a timer.
	constexpr float WORLD_BOUNDS = 4096.0f;
	if (pev->origin.x < -WORLD_BOUNDS || pev->origin.x > WORLD_BOUNDS ||
		pev->origin.y < -WORLD_BOUNDS || pev->origin.y > WORLD_BOUNDS ||
		pev->origin.z < -WORLD_BOUNDS || pev->origin.z > WORLD_BOUNDS)
	{
		UTIL_Remove(this);
		return;
	}

	if (m_iMeteorType == 2)
	{
		// BigSmokingMeteorThink @0x100a7230: three larger smoke sprites.
		EmitSmokeSprite(25, 32.0f, 8);
		EmitSmokeSprite(75, 256.0f, 16);
		EmitSmokeSprite(100, 512.0f, 32);
	}
	else
	{
		// LittleSmokingMeteorThink @0x100a6f70: three smaller sprites.
		EmitSmokeSprite(50, 32.0f, 8);
		EmitSmokeSprite(30, 128.0f, 16);
		EmitSmokeSprite(15, 256.0f, 32);
	}

	// The retail particle cadence is driven by a private constant whose
	// exact semantic unit is not recovered; 100 ms keeps the discrete
	// particle trail continuous alongside the shared beam-follow.
	pev->nextthink = gpGlobals->time + 0.1f;
}

Vector CMeteorChunk::GetImpactOrigin() const
{
	if (m_iMeteorType == 0)
		return pev->origin;

	Vector vecForward = pev->velocity.Normalize();
	if (m_iMeteorType == 1)
		return pev->origin + vecForward * -50.0f;

	return pev->origin + vecForward * RANDOM_LONG(-156, -128);
}

void CMeteorChunk::EmitRetailDebris(const Vector& vecImpact)
{
	if (!m_bDebris || m_iMeteorType == 0)
		return;

	const int iShardCount = RANDOM_LONG(5, 10);
	const int iDebrisModel = m_iMeteorType == 1 ? m_iSmallDebrisModel : m_iDebrisModel;
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecImpact);
	WRITE_BYTE(TE_BREAKMODEL);
	WRITE_COORD(vecImpact.x);
	WRITE_COORD(vecImpact.y);
	WRITE_COORD(vecImpact.z);
	WRITE_COORD(10);
	WRITE_COORD(10);
	WRITE_COORD(10);
	WRITE_COORD(RANDOM_LONG(-200, 200));
	WRITE_COORD(RANDOM_LONG(-200, 200));
	WRITE_COORD(RANDOM_LONG(-300, -100));
	WRITE_BYTE(10);
	WRITE_SHORT(iDebrisModel);
	WRITE_BYTE(iShardCount);
	WRITE_BYTE(60 / iShardCount);
	WRITE_BYTE(TE_BOUNCE_SHELL);
	MESSAGE_END();
}

void CMeteorChunk::EmitEnhancedImpactEffects(const Vector& vecImpact, float flRadius)
{
	// Deliberate non-retail presentation layer. Keep it separate from the
	// debris/smoke paths above so future fidelity work can opt out cleanly.
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecImpact);
	WRITE_BYTE(TE_EXPLOSION);
	WRITE_COORD(vecImpact.x);
	WRITE_COORD(vecImpact.y);
	WRITE_COORD(vecImpact.z);
	WRITE_SHORT(m_iImpactSprite);
	WRITE_BYTE(m_iMeteorType == 2 ? 32 : 20);
	WRITE_BYTE(18);
	WRITE_BYTE(TE_EXPLFLAG_NONE);
	MESSAGE_END();
	// Nachtrag 2026-09-06 (Nutzerwunsch: Erschuetterung+Lichtblitz bei
	// jeder Explosion, siehe util.h/UTIL_ExplosionEffects).
	UTIL_ExplosionEffects(vecImpact, flRadius);

	// Visible expanding shockwave, analogous to the SDK's blast-cylinder
	// effects.  This is cosmetic; RadiusDamage below is authoritative.
	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, vecImpact);
	WRITE_BYTE(TE_BEAMCYLINDER);
	WRITE_COORD(vecImpact.x);
	WRITE_COORD(vecImpact.y);
	WRITE_COORD(vecImpact.z + 8);
	WRITE_COORD(vecImpact.x);
	WRITE_COORD(vecImpact.y);
	WRITE_COORD(vecImpact.z + 8 + flRadius / 0.2f);
	WRITE_SHORT(m_iSmokeSprite);
	WRITE_BYTE(0);
	WRITE_BYTE(0);
	WRITE_BYTE(3);
	WRITE_BYTE(14);
	WRITE_BYTE(0);
	WRITE_BYTE(255);
	WRITE_BYTE(128);
	WRITE_BYTE(32);
	WRITE_BYTE(220);
	WRITE_BYTE(0);
	MESSAGE_END();

}

void CMeteorChunk::ChunkTouch(CBaseEntity* pOther)
{
	const Vector vecImpact = GetImpactOrigin();
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "meteor/meteor_hit1.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "meteor/meteor_hit2.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	default:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "meteor/meteor_hit3.wav", 1.0, ATTN_NORM, 0, 100);
		break;
	}

	const float flDamage = m_iMeteorType == 2 ? 100.0f : 50.0f;
	const float flRadius = m_iMeteorType == 2 ? 400.0f : 100.0f;
	EmitRetailDebris(vecImpact);
	EmitEnhancedImpactEffects(vecImpact, flRadius);
	// Both retail type-specific paths first move the meteor to their
	// calculated impact point and then apply the area damage from there.
	RadiusDamage(vecImpact, pev, pev, flDamage, flRadius, CLASS_NONE, DMG_BLAST);

	UTIL_Remove(this);
}

//=========================================================
// meteor_target - CMeteorTarget. Triggered, targeted Meteor path.
// Retail Use @0x100a8210 projects the spawn origin by heightabove along
// pitch, traces that segment, then creates a CMeteor with the configured
// raw type and a downward start vector. This closes the map-facing path
// used by RUST4B/RUST5A/rust6b/rust6c/rust6d.
//=========================================================
class CMeteorTarget : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

private:
	float m_flHeightAbove = 0.0f;
	int m_iMeteorType = 1;
};
LINK_ENTITY_TO_CLASS(meteor_target, CMeteorTarget);

TYPEDESCRIPTION CMeteorTarget::m_SaveData[] =
	{
		DEFINE_FIELD(CMeteorTarget, m_flHeightAbove, FIELD_FLOAT),
		DEFINE_FIELD(CMeteorTarget, m_iMeteorType, FIELD_INTEGER),
	};

IMPLEMENT_SAVERESTORE(CMeteorTarget, CBaseEntity);

void CMeteorTarget::Precache()
{
	PrecacheModel("models/asteroid.mdl");
	PrecacheModel("sprites/smoke.spr");
	PrecacheModel("sprites/impact.spr");
	PrecacheModel("sprites/chip.spr");
	UTIL_PrecacheOther("meteor_god_chunk");
}

void CMeteorTarget::Spawn()
{
	Precache();
	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);
}

bool CMeteorTarget::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pev->angles.x = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "heightabove"))
	{
		m_flHeightAbove = atof(pkvd->szValue);
		return true;
	}
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iMeteorType = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CMeteorTarget::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	UTIL_MakeAimVectors(pev->angles);
	const Vector vecUp = gpGlobals->v_forward;
	TraceResult tr;
	UTIL_TraceLine(pev->origin, pev->origin + vecUp * m_flHeightAbove,
		ignore_monsters, edict(), &tr);

	CBaseEntity* pEntity = CBaseEntity::Create("meteor_god_chunk", tr.vecEndPos, g_vecZero, edict());
	if (pEntity)
	{
		auto pMeteor = static_cast<CMeteorChunk*>(pEntity);
		pMeteor->Configure(m_iMeteorType, false);
		pEntity->pev->velocity = vecUp * -1400.0f;
		pEntity->pev->angles = UTIL_VecToAngles(pEntity->pev->velocity);
	}
	// CMeteorTarget::Use removes the one-shot marker after creating its
	// meteor. This remains true even if entity allocation fails.
	UTIL_Remove(this);
}

//=========================================================
// meteor_god - CMeteorGod. Invisible ambient meteor-shower spawner
// (see file header re: eruption-math simplification).
//=========================================================
class CMeteorGod : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;
	void EXPORT ShowerThink();

	bool Save(CSave& save) override;
	bool Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	float m_flDropSizeX = 1.0;
	float m_flDropSizeY = 5.0;
	float m_flMinDropFreq = 1.0;
	float m_flMaxDropFreq = 1.0;
	int m_iTypeStreak = 1;
	int m_iTypeLarge = 1;
	int m_iTypeSmall = 1;
	bool m_bDebris = false;
	bool m_bActive = true;
	Vector m_vecDropDirection = Vector(0, 0, 1);
};
LINK_ENTITY_TO_CLASS(meteor_god, CMeteorGod);

TYPEDESCRIPTION CMeteorGod::m_SaveData[] =
	{
		DEFINE_FIELD(CMeteorGod, m_flDropSizeX, FIELD_FLOAT),
		DEFINE_FIELD(CMeteorGod, m_flDropSizeY, FIELD_FLOAT),
		DEFINE_FIELD(CMeteorGod, m_flMinDropFreq, FIELD_FLOAT),
		DEFINE_FIELD(CMeteorGod, m_flMaxDropFreq, FIELD_FLOAT),
		DEFINE_FIELD(CMeteorGod, m_iTypeStreak, FIELD_INTEGER),
		DEFINE_FIELD(CMeteorGod, m_iTypeLarge, FIELD_INTEGER),
		DEFINE_FIELD(CMeteorGod, m_iTypeSmall, FIELD_INTEGER),
		DEFINE_FIELD(CMeteorGod, m_bDebris, FIELD_BOOLEAN),
		DEFINE_FIELD(CMeteorGod, m_bActive, FIELD_BOOLEAN),
		DEFINE_FIELD(CMeteorGod, m_vecDropDirection, FIELD_VECTOR),
	};

IMPLEMENT_SAVERESTORE(CMeteorGod, CBaseEntity);

bool CMeteorGod::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "dropsizex"))
	{
		m_flDropSizeX = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "dropsizey"))
	{
		m_flDropSizeY = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "mindropfreq"))
	{
		m_flMinDropFreq = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "maxdropfreq"))
	{
		m_flMaxDropFreq = atof(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "typestreak"))
	{
		m_iTypeStreak = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "typelarge"))
	{
		m_iTypeLarge = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "typesmall"))
	{
		m_iTypeSmall = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "debris"))
	{
		// Retail stores the parsed map value inverted, then emits break-model
		// debris only when the stored field is non-zero.
		m_bDebris = atoi(pkvd->szValue) == 0;
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "pitch"))
	{
		pev->angles.x = atof(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CMeteorGod::Spawn()
{
	Precache();

	pev->effects |= EF_NODRAW;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	UTIL_SetOrigin(pev, pev->origin);
	UTIL_MakeAimVectors(pev->angles);
	m_vecDropDirection = gpGlobals->v_forward;

	// Retail Spawn starts inactive when bit 1 is set; Use toggles this
	// state independently of the USE_TYPE argument.
	m_bActive = (pev->spawnflags & 1) == 0;

	SetThink(&CMeteorGod::ShowerThink);
	pev->nextthink = gpGlobals->time + 1.0;
}

void CMeteorGod::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	m_bActive = !m_bActive;
}

void CMeteorGod::Precache()
{
	PrecacheModel("models/rockgibsbig.mdl");
	PrecacheModel("models/lavaball.mdl");
	PrecacheModel("models/asteroid.mdl");
	PrecacheModel("sprites/smoke.spr");
	PrecacheModel("sprites/impact.spr");
	PrecacheModel("sprites/chip.spr");
	PrecacheSound("meteor/meteor_hit1.wav");
	PrecacheSound("meteor/meteor_hit2.wav");
	PrecacheSound("meteor/meteor_hit3.wav");
	UTIL_PrecacheOther("meteor_god_chunk");
}

void CMeteorGod::ShowerThink()
{
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT(m_flMinDropFreq, m_flMaxDropFreq);
	if (!m_bActive)
		return;

	// CMeteorGod::Think samples the configured drop sizes as world units.
	Vector vecSpot = pev->origin + Vector(RANDOM_FLOAT(-m_flDropSizeX, m_flDropSizeX), RANDOM_FLOAT(-m_flDropSizeY, m_flDropSizeY), 0);

	CBaseEntity* pChunk = CBaseEntity::Create("meteor_god_chunk", vecSpot, g_vecZero, edict());
	if (pChunk)
	{
		const int iTotalWeight = m_iTypeStreak + m_iTypeLarge + m_iTypeSmall;
		const int iChoice = RANDOM_LONG(1, iTotalWeight > 0 ? iTotalWeight : 1);
		int iMeteorType = 0;
		if (iChoice > m_iTypeStreak && iChoice <= m_iTypeStreak + m_iTypeLarge)
			iMeteorType = 2;
		else if (iChoice > m_iTypeStreak + m_iTypeLarge)
			iMeteorType = 1;

		auto pMeteor = static_cast<CMeteorChunk*>(pChunk);
		pMeteor->Configure(iMeteorType, m_bDebris);
		// FUN_100a6500 jitters the first direction component by +/-0.1,
		// then applies a random scalar in the range -1600..-1200.
		Vector vecDropDirection = m_vecDropDirection;
		vecDropDirection.x += RANDOM_FLOAT(-0.1f, 0.1f);
		pChunk->pev->velocity = vecDropDirection * RANDOM_FLOAT(-1600.0f, -1200.0f);
		pChunk->pev->angles = UTIL_VecToAngles(pChunk->pev->velocity);
	}
}
