//=========================================================
// MAYAN6's monster classes with no SDK precedent: monster_gator
// (CGator) and monster_microraptor (CMicroRaptor), plus MAYAN6's one
// env_clusterExplosion placement (CClusterExplosion + its dml_cluster/
// CClusterGrenade sub-munition). weapon_minigun, MAYAN6's fourth gap,
// is a player weapon and out of scope for this pass (see STATUS.md).
//
// Decompiled fresh from gunman.dll this session:
//   monster_gator: LINK @0x10071920, vtable @0x100f80d0, Spawn
//     @0x10071f10, Precache @0x10071ef0, Classify @0x10071d90
//     (returns 5 = CLASS_ALIEN_MILITARY). Matches
//     findings/entity_review_list.csv's "keine Verhaltenslogik
//     decompiliert" - no custom Schedule_t/Task_t table found here
//     either, this session's decompile only covers Spawn/Precache/
//     Classify (consistent with the doc, not a gap introduced by
//     this pass).
//   monster_microraptor: LINK @0x10073b50, vtable @0x100f8540, Spawn
//     @0x10074c70, Precache @0x10074c30, Classify @0x10073bb0
//     (returns 15 = CLASS_ALIEN_PREDATOR_RAPTOR, same constant
//     confirmed independently for monster_raptor's hostile variant
//     and monster_largescorpion). Confirms
//     findings/entity_review_list.csv's "Modell 'Raptor.mdl' gehoert
//     zu DIESER Klasse, NICHT zu 'monster_raptor'" naming-trap note -
//     this is the small, fast, 12 HP raptor from the user's original
//     screenshot, distinct from monster_raptor/monster_rheptor's
//     larger models/rheptor.mdl.
//
// Simplified relative to the original: neither class has a
// documented or decompiled custom TakeDamage/Killed/attack override
// in this pass - both use plain default CBaseMonster combat/AI
// behavior (matches monster_gator's already-established "keine
// Verhaltenslogik" finding; monster_microraptor's own attack
// schedule, per entity_review_list.csv, exists in the binary but
// wasn't decompiled this session - out of scope for a fast-moving
// secondary enemy, same simplification level as CRaptor/CCricket
// elsewhere in this project).
//
// env_clusterExplosion (CClusterExplosion) - reviewed this session via
// the already-fully-decompiled ghidra/logs/cexp_ctor.txt (ctor helper
// FUN_10081a70 @0x10081a70, constructing "dml_cluster") and
// cexp_combat.txt (CClusterExplosion::ClusterExplode @0x100a2c20,
// vtable @0x100fefa4, findings/entities/env_clusterexplosion.md,
// Session 64) - confirmed against the actual decompile output, not
// just the findings prose. Confirmed: invisible models/null.mdl point
// entity, KeyValue-parsed clusterDamage(int,default 40)/
// numGrenades(int,default 4), spawnflag bit 2 = Repeatable (self-
// removes after firing when unset, matches
// `(*(byte*)(...+0x1a0) & 2) == 0 -> FUN_100606b0` = UTIL_Remove),
// Use=ClusterExplode spawning `numGrenades` "dml_cluster" entities in
// a loop via the same raw-vtable-construct pattern used elsewhere in
// this project (see CDmlRocket/CChemBall), each given a randomized
// scatter position/velocity and pev->dmg = clusterDamage directly
// (offset 0x1e0, matches this SDK's pev->dmg usage in
// CGrenade::Explode). The previously approximated dml_cluster lifecycle
// is now fully reconstructed from Spawn/Precache/TumbleClusterThink/
// ClusterDetonate/ClusterSmokeThink; see
// findings/entities/cluster_grenade_retail_batch_2026-09-09.md.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

//=========================================================
// monster_gator - CGator.
//=========================================================
class CGator : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_MILITARY; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
};
LINK_ENTITY_TO_CLASS(monster_gator, CGator);

void CGator::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/gator.mdl");
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 25));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 60; // plausible default - exact skill-cvar lookup not traced this session, see file header
	pev->view_ofs = Vector(0, 0, 20);

	MonsterInit();
}

void CGator::Precache()
{
	PrecacheModel("models/gator.mdl");
}

//=========================================================
// monster_microraptor - CMicroRaptor.
//=========================================================
class CMicroRaptor : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_ALIEN_PREDATOR_RAPTOR; }
	// CORRECTION (2026-09-05): missing SetYawSpeed() override, same
	// root cause/fix as CFriendlyGunman (human_gunman.cpp) - see
	// findings/open_items_audit_2026-09-05.md.
	void SetYawSpeed() override { pev->yaw_speed = (m_Activity == ACT_RUN) ? 90 : 70; }
};
LINK_ENTITY_TO_CLASS(monster_microraptor, CMicroRaptor);

void CMicroRaptor::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/raptor.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 32));

	pev->solid = SOLID_SLIDEBOX;
	pev->health = 12;
	pev->view_ofs = Vector(0, 0, 24);
	pev->flags |= FL_MONSTER;

	MonsterInit();
}

void CMicroRaptor::Precache()
{
	PrecacheModel("models/raptor.mdl");
	PrecacheModel("sprites/gorehuman.spr");
	PrecacheModel("sprites/gibhuman.spr");
}

//=========================================================
// dml_cluster - CClusterGrenade. Shared cluster sub-munition (also
// used by weapon_dml and monster_manta). It is not a stock timed grenade:
// retail has its own randomized fuse, smoke trail, detonation and expanding
// smoke-ring lifecycle. See findings/entities/cluster_grenade_retail_batch_2026-09-09.md.
//=========================================================
class CClusterGrenade : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT TumbleClusterThink();
	void EXPORT ClusterDetonate();
	void EXPORT ClusterSmokeThink();

	static CClusterGrenade* Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float flDamage);

private:
	int m_iSmokeSprite = 0;
	int m_iShockSprite = 0;
	unsigned short m_usClusterExplosion = 0;
	EHANDLE m_hSmokeRing;
	bool m_bTrailStarted = false;
};
LINK_ENTITY_TO_CLASS(dml_cluster, CClusterGrenade);

void CClusterGrenade::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_NO;
	pev->gravity = 0.5f;
	pev->friction = 0.6f;
	pev->classname = MAKE_STRING("dml_cluster");
	SET_MODEL(ENT(pev), "models/dmlcluster.mdl");
	UTIL_SetSize(pev, g_vecZero, g_vecZero);
	SetTouch(&CGrenade::BounceTouch);
	SetThink(&CClusterGrenade::TumbleClusterThink);
	pev->dmgtime = gpGlobals->time + RANDOM_FLOAT(0.8f, 4.0f);
	pev->nextthink = gpGlobals->time + 0.1f;
	pev->framerate = 1.0f;
	m_bTrailStarted = false;
	m_hSmokeRing = nullptr;
}

void CClusterGrenade::Precache()
{
	PrecacheModel("models/dmlcluster.mdl");
	m_iSmokeSprite = PrecacheModel("sprites/smoke.spr");
	PrecacheModel("sprites/kaboom.spr");
	m_usClusterExplosion = PrecacheEvent(1, "events/ClusterExplosion.sc");
	m_iShockSprite = PrecacheModel("sprites/clustershock.spr");
	PrecacheModel("sprites/smokering.spr");
	PrecacheSound("manta/carpetbomb1.wav");
	PrecacheSound("manta/carpetbomb2.wav");
	PrecacheSound("manta/carpetbomb3.wav");
}

CClusterGrenade* CClusterGrenade::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float flDamage)
{
	CClusterGrenade* pGrenade = GetClassPtr((CClusterGrenade*)nullptr);
	pGrenade->Spawn();

	UTIL_SetOrigin(pGrenade->pev, vecStart);
	pGrenade->pev->velocity = vecVelocity;
	pGrenade->pev->angles = UTIL_VecToAngles(pGrenade->pev->velocity);
	pGrenade->pev->owner = ENT(pevOwner);
	pGrenade->pev->dmg = flDamage;

	pGrenade->pev->sequence = RANDOM_LONG(3, 6);
	pGrenade->pev->animtime = gpGlobals->time;

	return pGrenade;
}

void CClusterGrenade::TumbleClusterThink()
{
	if (!IsInWorld())
	{
		UTIL_Remove(this);
		return;
	}

	if (!m_bTrailStarted)
	{
		MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);
		WRITE_BYTE(TE_BEAMFOLLOW);
		WRITE_SHORT(entindex());
		WRITE_SHORT(m_iSmokeSprite);
		WRITE_BYTE(20);
		WRITE_BYTE(5);
		WRITE_BYTE(224);
		WRITE_BYTE(224);
		WRITE_BYTE(255);
		WRITE_BYTE(255);
		MESSAGE_END();
		m_bTrailStarted = true;
	}

	pev->nextthink = gpGlobals->time + 0.1f;
	if (pev->dmgtime - 1.0f < gpGlobals->time)
		UTIL_DynamicLight(pev->origin + pev->velocity * (pev->dmgtime - gpGlobals->time), 400.0f, 255, 255, 255, 0.1f, 0.0f);
	if (pev->dmgtime <= gpGlobals->time)
		SetThink(&CClusterGrenade::ClusterDetonate);

	if (pev->waterlevel != 0)
	{
		pev->velocity = pev->velocity * 0.8f;
		pev->framerate = 0.2f;
	}
}

void CClusterGrenade::ClusterDetonate()
{
	TraceResult tr;
	Vector vecStart = pev->origin + Vector(0, 0, 8);
	Vector vecEnd = pev->origin - Vector(0, 0, 64);
	UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, edict(), &tr);
	pev->origin = tr.vecEndPos;

	PLAYBACK_EVENT_FULL(FEV_GLOBAL, edict(), m_usClusterExplosion, 0.0f, pev->origin, g_vecZero, 0.0f, 0.0f, 0, 0, 0, 0);
	UTIL_ExplosionEffects(pev->origin, 400.0f);
	RadiusDamage(pev->origin, pev, pev->owner ? VARS(pev->owner) : pev, pev->dmg, pev->dmg * 2.5f, CLASS_NONE, DMG_BLAST);

	pev->effects |= EF_NODRAW;
	pev->velocity = g_vecZero;
	SetThink(&CClusterGrenade::ClusterSmokeThink);
	pev->nextthink = gpGlobals->time;
}

void CClusterGrenade::ClusterSmokeThink()
{
	CSprite* pSmoke = static_cast<CSprite*>((CBaseEntity*)m_hSmokeRing);
	if (!pSmoke)
	{
		pSmoke = CSprite::SpriteCreate("sprites/smokering.spr", pev->origin, false);
		if (!pSmoke)
		{
			UTIL_Remove(this);
			return;
		}
		m_hSmokeRing = pSmoke;
		pSmoke->SetScale(0.2f);
		pSmoke->SetTransparency(kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation);
	}
	else
	{
		pSmoke->SetScale(pSmoke->pev->scale + 0.16f);
	}

	pev->nextthink = gpGlobals->time + 0.1f;
	if (pSmoke->pev->scale > 1.25f)
	{
		UTIL_Remove(pSmoke);
		m_hSmokeRing = nullptr;
		UTIL_Remove(this);
	}
}

// Thin wrapper so other translation units (dml.cpp/dmlgrenade.cpp - the
// confirmed real dml_cluster/CClusterGrenade users besides
// env_clusterExplosion, see findings/weapons/weapon_dml.md) can spawn a
// dml_cluster sub-munition without redeclaring the CClusterGrenade class
// itself (which would violate ODR since it's only partially declared
// elsewhere).
CBaseEntity* Dml_ShootClusterGrenade(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float flDamage)
{
	return CClusterGrenade::Shoot(pevOwner, vecStart, vecVelocity, flDamage);
}

//=========================================================
// env_clusterExplosion - CClusterExplosion. Level-scripting trigger,
// see file header for the full decompile-verified behavior.
//=========================================================
class CClusterExplosion : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	bool KeyValue(KeyValueData* pkvd) override;
	void EXPORT ClusterExplode(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);

private:
	int m_iClusterDamage = 40;
	int m_iNumGrenades = 4;
};
LINK_ENTITY_TO_CLASS(env_clusterExplosion, CClusterExplosion);

bool CClusterExplosion::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "clusterDamage"))
	{
		m_iClusterDamage = atoi(pkvd->szValue);
		return true;
	}
	else if (FStrEq(pkvd->szKeyName, "numGrenades"))
	{
		m_iNumGrenades = atoi(pkvd->szValue);
		return true;
	}

	return CBaseEntity::KeyValue(pkvd);
}

void CClusterExplosion::Spawn()
{
	Precache();

	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL(ENT(pev), "models/null.mdl");
	UTIL_SetOrigin(pev, pev->origin);

	SetUse(&CClusterExplosion::ClusterExplode);
}

void CClusterExplosion::Precache()
{
	PrecacheModel("models/null.mdl");
	UTIL_PrecacheOther("dml_cluster");
}

void CClusterExplosion::ClusterExplode(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	for (int i = 0; i < m_iNumGrenades; i++)
	{
		Vector vecScatter = pev->origin + Vector(RANDOM_FLOAT(-15, 15), RANDOM_FLOAT(-15, 15), RANDOM_FLOAT(-15, 15));
		Vector vecVelocity = Vector(RANDOM_FLOAT(-250, 250), RANDOM_FLOAT(-250, 250), RANDOM_FLOAT(50, 250));

		CClusterGrenade::Shoot(pev, vecScatter, vecVelocity, (float)m_iClusterDamage);
	}

	// SF_CLUSTEREXPLOSION_REPEATABLE (bit 2): if unset, the trigger is
	// single-use and removes itself after firing - confirmed via
	// decompile, see file header.
	if ((pev->spawnflags & 2) == 0)
		UTIL_Remove(this);
}
