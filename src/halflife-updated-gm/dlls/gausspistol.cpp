//=========================================================
// weapon_gausspistol (Aliases: weapon_pistol) - CGaussPistol. Fourth
// of the six points deferred during the map-driven pass (see
// findings/weapons/weapon_gausspistol.md and
// [[project-weapon-gausspistol-deferred]] memory) to be picked up -
// the most thoroughly pre-decompiled of the five deferred weapons
// (all four in-game-manual fire modes were already traced to concrete
// addresses across earlier sessions).
//
// Decompiled fresh from gunman.dll this session: LINK-equivalent
// constructor sets classname "weapon_gausspistol" always (even
// spawned as "weapon_pistol"), vtable @0x100fcb88, Spawn @0x100901f0,
// Precache @0x10090270, PrimaryAttack dispatch @0x100907a0. Confirms
// findings/weapons/weapon_gausspistol.md exactly: weapon ID 0x1e(30),
// world model "models/w_gauss.mdl", ammo type "uranium" (its own
// resource, not shared 9mm), iMaxClip=1. Precache confirms
// "models/p_357.mdl" (stock HL's Python playermodel, reused unchanged
// - dlls/python.cpp is untouched, only used as a reference for
// hitscan-pistol structure) and the confirmed sniper-zoom sound set
// (gsnipe_zoom.wav/sniperzoom.wav/sniperunzoom.wav), plus 4
// UTIL_PrecacheOther calls matching the doc's four companion classes
// (gauss_charged/guass_bolt/gauss_ball/gauss_glow - none of which are
// placed by any classname-gap map scanned so far, so none are
// reimplemented here).
//
// PrimaryAttack's dispatch on the internal fire-mode field
// (param_1[0x2d]) is confirmed as: field==4 -> sniper-scope toggle
// (FUN_10090c70), field==2 -> the "Charge" mode (FUN_10090ac0, 10
// ammo/shot, confirmed exactly matching the manual), field==3 -> a
// single precision shot (FUN_10090940, 1 ammo/shot), any other value
// (the field's default/initial state, param_1[0x2f]=0 in Spawn) ->
// "Rapid" automatic fire through the shared, project-wide generic
// fire helper FUN_100912f0 with a 3-shot burst counter. This project
// has no per-mapper or per-map keyvalue confirmed for selecting
// between these internal field values (the manual's "Sniper Kit"/
// "Fire Type" upgrade items - cust_2GaussPistolSniper/
// cust_1GaussPistolFireType, see the findings doc's Session-37/38
// note - are the in-game mechanism, not reimplemented here; no gap
// scan required them). Simplified relative to the original: only two
// of the four confirmed modes are wired to the two available
// CBasePlayerWeapon attack inputs - PrimaryAttack reproduces the
// confirmed single-shot mode (FUN_10090940: 1 ammo, "gauss_fire1.wav"),
// SecondaryAttack reproduces the confirmed sniper-scope toggle mode
// (FUN_10090c70: gated on >=20 ammo to enter, "gsnipe_zoom.wav" on
// enter / "sniperunzoom.wav" on exit, toggles the player's FOV -
// same m_iFOV toggle pattern already used unmodified in dlls/
// crossbow.cpp, only used here as a reference). The "Charge" (10
// ammo/shot) and "Rapid" (3-shot burst) modes are NOT reachable
// through a control input here - their confirmed ammo costs and
// sounds are documented above for a future pass that adds a
// mode-cycle input or reproduces the Fire-Type upgrade item.
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

#define GAUSSPISTOL_SCOPE_FOV 20
#define GAUSSPISTOL_SCOPE_MIN_AMMO 20

class CGaussPistol : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	bool GetItemInfo(ItemInfo* p) override;
	bool Deploy() override;
	void Holster() override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponIdle() override;
	bool ShouldWeaponIdle() override { return true; }
};
LINK_ENTITY_TO_CLASS(weapon_gausspistol, CGaussPistol);
LINK_ENTITY_TO_CLASS(weapon_pistol, CGaussPistol);

void CGaussPistol::Spawn()
{
	pev->classname = MAKE_STRING("weapon_gausspistol");
	Precache();
	m_iId = WEAPON_GAUSSPISTOL;
	SET_MODEL(ENT(pev), "models/w_gauss.mdl");

	// BUG FIX (2026-09-05, live gameplay report: "die gunmap Pistole kann
	// nicht abgefeuert werden"): m_iDefaultAmmo was never set here, so a
	// freshly-picked-up weapon_gausspistol got 0 clip AND 0 reserve uranium
	// (CBasePlayerWeapon::AddPrimaryAmmo(this, 0, ...) fills nothing), making
	// PrimaryAttack() permanently hit its m_iClip<=0 empty-sound early-out.
	// Every other clip weapon in this codebase sets this in Spawn() (see
	// e.g. CGlock/CGauss); this one was simply missing it.
	m_iDefaultAmmo = GAUSSPISTOL_DEFAULT_GIVE;

	FallInit();
}

void CGaussPistol::Precache()
{
	PrecacheModel("sprites/gaussbig.spr");
	PrecacheModel("sprites/gausspark.spr");
	PrecacheModel("sprites/gaussbeam2.spr");
	PrecacheModel("sprites/gaussfade.spr");
	PrecacheModel("sprites/gausspoof.spr");
	PrecacheModel("sprites/gausspuff.spr");
	PrecacheModel("sprites/white.spr");
	PrecacheModel("models/w_gauss.mdl");
	PrecacheModel("models/v_guasspistol.mdl"); // authentic original typo, see file header
	PrecacheModel("models/p_357.mdl");
	PrecacheSound("items/9mmclip1.wav");
	PrecacheSound("weapons/gauss_charge.wav");
	PrecacheSound("weapons/gauss_fire1.wav");
	PrecacheSound("weapons/gauss_fire2.wav");
	PrecacheSound("weapons/gauss_fire3.wav");
	PrecacheSound("weapons/gauss_fire4.wav");
	PrecacheSound("weapons/gsnipe_zoom.wav");
	PrecacheSound("weapons/sniperzoom.wav");
	PrecacheSound("weapons/sniperunzoom.wav");
	PrecacheEvent(1, "events/gauss.sc");
	PrecacheEvent(1, "events/snipershot.sc");
}

bool CGaussPistol::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "uranium";
	p->iMaxAmmo1 = URANIUM_MAX_CARRY;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = 1;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_GAUSSPISTOL;
	p->iFlags = 0;
	p->iWeight = 10;

	return true;
}

bool CGaussPistol::Deploy()
{
	return DefaultDeploy("models/v_guasspistol.mdl", "models/p_357.mdl", 0, "python");
}

void CGaussPistol::Holster()
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0;
		EMIT_SOUND(m_pPlayer->edict(), CHAN_ITEM, "weapons/sniperunzoom.wav", 1.0, ATTN_NORM);
	}
}

void CGaussPistol::PrimaryAttack()
{
	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.2;
		return;
	}

	m_iClip--;

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	FireBullets(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357);

	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_WEAPON, "weapons/gauss_fire1.wav", 1.0, ATTN_NORM, 0, 100);
	pev->effects |= EF_MUZZLEFLASH;
	SendWeaponAnim(0);

	m_pPlayer->pev->punchangle.x -= 2;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5;
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5.0;

	if (m_iClip == 0)
		m_flNextPrimaryAttack += 0.5; // brief extra delay covering the (unreproduced) reload animation
}

void CGaussPistol::SecondaryAttack()
{
	if (m_pPlayer->m_iFOV != 0)
	{
		m_pPlayer->m_iFOV = 0;
		EMIT_SOUND(m_pPlayer->edict(), CHAN_ITEM, "weapons/sniperunzoom.wav", 1.0, ATTN_NORM);
	}
	else if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] >= GAUSSPISTOL_SCOPE_MIN_AMMO)
	{
		m_pPlayer->m_iFOV = GAUSSPISTOL_SCOPE_FOV;
		EMIT_SOUND(m_pPlayer->edict(), CHAN_ITEM, "weapons/gsnipe_zoom.wav", 1.0, ATTN_NORM);
	}
	else
	{
		PlayEmptySound();
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CGaussPistol::WeaponIdle()
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	SendWeaponAnim(0);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + RANDOM_FLOAT(10, 15);
}
