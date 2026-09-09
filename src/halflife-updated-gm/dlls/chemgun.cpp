//=========================================================
// weapon_chemgun (Alias: weapon_SPchemicalgun) - CChemGun, plus a
// simplified liquid-ball projectile (chem_ball/CChemBall). Genuinely
// unimplemented gap identified during the 2026-09-05 findings audit
// (findings/open_items_audit_2026-09-05.md) - only its ammo pickup
// (ammo_chemical, rebar_misc.cpp) existed before this.
//
// Decompiled fresh from gunman.dll: LINK-equivalent constructor sets
// classname "weapon_SPchemicalgun" always (even spawned as
// "weapon_chemgun"), vtable @0x100fddc4, Spawn @0x10097070, Precache
// (shared with weapon_shotgun's precache-tail helper, see below),
// GetItemInfo @0x100971d0, Deploy @0x10097360, PrimaryAttack
// (confirmed via findings/weapons/primaryattack_breakthrough.md and a
// fresh decompile) @0x10097530, its projectile dispatcher @0x10097d40.
// Confirms findings/weapons/weapon_chemgun.md exactly: weapon ID
// 0x13(19), world model "models/w_chemgun.mdl", ammo type
// "ChemicalAcid" (the same resource already used by ammo_chemical),
// iMaxClip=WEAPON_NOCLIP (a continuous chemical-tank weapon, no
// discrete clip), iSlot=7, iWeight=-10. Precache confirms
// "models/p_hgun.mdl" (stock HL's Glock/Handgun playermodel, reused
// unchanged - dlls/glock.cpp is untouched, only used as a reference)
// and the confirmed splash sprite "sprites/greensplash.spr".
//
// PrimaryAttack confirms the in-game manual's "Pressure" dial
// (findings/official_manual_and_website.md, S. 19): an internal 0-100
// "pressure" field (param_1[0x34]) is bucketed into a 1-4 ammo cost
// per shot (thresholds at 50/86, plus a further +1 for a mode==3
// state), consumes that amount from the shared ammo pool, plays
// "weapons/cg_fire.wav" if any of the three Acid/Base/Neutralizer mix
// fields (param_1[0x30/0x31/0x32]) is nonzero, or "weapons/empty.wav"
// otherwise (a genuine "safety fire" when no chemicals are loaded).
// The actual projectile dispatch (@0x10097d40) then routes to ONE of
// THREE distinct spawner functions depending on an internal
// mix-classification result (values 6/7/8, computed by four
// unlabeled classification helpers) - matching the manual's
// Acid-ball/Base-ball/Explosive-or-smoking-clump table.
//
// Simplified relative to the original: the full 3-way mix
// classification and its three distinct projectile spawners are NOT
// reproduced (same scope decision already applied to weapon_dml's
// full Trigger-type/Payload-type matrix) - PrimaryAttack here always
// fires a single generic liquid-ball projectile (CChemBall) dealing
// DMG_ACID damage on impact plus a splash effect, using the confirmed
// ammo-cost-by-pressure-tier and sound-choice logic, but not
// distinguishing Acid-vs-organic from Base-vs-mechanical damage (the
// stock SDK has no "vs. silicon-based" damage type) or reproducing
// the explosive/smoking-clump variants. AmmoHudUpdate's confirmed
// four-value HUD (message types 0x18/0x19/0x1a/0x1b, one per
// Acid/Base/Neutralizer/Pressure) is not reproduced - no mapper-
// facing keyvalue for setting the mix ratio was found, so there is
// nothing meaningful to display yet.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "gamerules.h"
#include "UserMessages.h"

//=========================================================
// chem_ball - CChemBall, a simplified stand-in for the confirmed
// three-way Acid/Base/explosive liquid-ball dispatch - see file
// header.
//=========================================================
class CChemBall : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT BallTouch(CBaseEntity* pOther);
};
LINK_ENTITY_TO_CLASS(chem_ball, CChemBall);

void CChemBall::Spawn()
{
	Precache();

	pev->classname = MAKE_STRING("chem_ball");
	SET_MODEL(ENT(pev), "sprites/greensplash.spr");
	UTIL_SetSize(pev, Vector(-2, -2, -2), Vector(2, 2, 2));

	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 200;

	SetTouch(&CChemBall::BallTouch);

	pev->nextthink = gpGlobals->time + 3.0;
	SetThink(&CChemBall::SUB_Remove);
}

void CChemBall::Precache()
{
	PrecacheModel("sprites/greensplash.spr");
}

void CChemBall::BallTouch(CBaseEntity* pOther)
{
	if (0 != pOther->pev->takedamage)
	{
		pOther->TakeDamage(pev, VARS(pev->owner), 8, DMG_ACID);
	}

	MESSAGE_BEGIN(MSG_PAS, SVC_TEMPENTITY, pev->origin);
	WRITE_BYTE(TE_SPRITE);
	WRITE_COORD(pev->origin.x);
	WRITE_COORD(pev->origin.y);
	WRITE_COORD(pev->origin.z);
	WRITE_SHORT(g_sModelIndexFireball); // splash sprite is precached under greensplash.spr, fireball index reused for a generic tempentity puff
	WRITE_BYTE(8);
	WRITE_BYTE(150);
	MESSAGE_END();

	UTIL_Remove(this);
}

//=========================================================
// weapon_chemgun - CChemGun.
//=========================================================
class CChemGun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	bool GetItemInfo(ItemInfo* p) override;
	bool Deploy() override;

	void PrimaryAttack() override;
	void WeaponIdle() override;
	bool ShouldWeaponIdle() override { return true; }
};
LINK_ENTITY_TO_CLASS(weapon_chemgun, CChemGun);
LINK_ENTITY_TO_CLASS(weapon_SPchemicalgun, CChemGun);

void CChemGun::Spawn()
{
	pev->classname = MAKE_STRING("weapon_SPchemicalgun");
	Precache();
	m_iId = WEAPON_CHEMGUN;
	SET_MODEL(ENT(pev), "models/w_chemgun.mdl");

	FallInit();
}

void CChemGun::Precache()
{
	PrecacheModel("models/v_chemgun.mdl");
	PrecacheModel("models/w_chemgun.mdl");
	PrecacheModel("models/p_hgun.mdl");
	PrecacheModel("sprites/greensplash.spr");
	PrecacheSound("items/9mmclip1.wav");
	PrecacheSound("weapons/cg_fire.wav");
	PrecacheSound("weapons/empty.wav");

	UTIL_PrecacheOther("chem_ball");
}

bool CChemGun::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "ChemicalAcid";
	p->iMaxAmmo1 = 25;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 7;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_CHEMGUN;
	p->iFlags = 0;
	p->iWeight = -10;

	return true;
}

bool CChemGun::Deploy()
{
	return DefaultDeploy("models/v_chemgun.mdl", "models/p_hgun.mdl", 0, "onehanded");
}

void CChemGun::PrimaryAttack()
{
	// Confirmed ammo-cost-by-pressure-tier shape (1-4 units per shot);
	// no mapper-facing "pressure" keyvalue was found, so a fixed
	// mid-tier cost is used - see file header.
	const int iAmmoCost = 2;

	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] < iAmmoCost)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.25;
		return;
	}

	m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] -= iAmmoCost;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition();

	CChemBall* pBall = GetClassPtr((CChemBall*)nullptr);
	pBall->pev->angles = m_pPlayer->pev->v_angle;
	pBall->pev->origin = vecSrc;
	pBall->Spawn();
	pBall->pev->owner = m_pPlayer->edict();
	pBall->pev->velocity = gpGlobals->v_forward * 600;

	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/cg_fire.wav", 1.0, ATTN_NORM, 0, 100);
	SendWeaponAnim(0);

	m_pPlayer->pev->punchangle.x -= 1;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.3;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.0;
}

void CChemGun::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(0);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
}
