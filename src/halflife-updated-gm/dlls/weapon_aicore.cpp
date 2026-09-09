//=========================================================
// weapon_aicore - CAiCore. A carried Gunman-only quest/story item
// implemented via the weapon system, not a combat weapon (see
// findings/weapons/weapon_aicore.md, cross-checked and confirmed by
// this session's own fresh decompile rather than trusted as-is).
//
// Decompiled fresh from gunman.dll: LINK @0x1007b1f0, vtable
// @0x100f9d50, Spawn @0x1007b2c0, Precache @0x1007b330. Confirms the
// doc's findings exactly: weapon ID 7 (re-mapped here to the new
// WEAPON_AICORE enum entry in cdll_dll.h rather than reusing the
// stock enum's own slot 7, which is WEAPON_SHOTGUN - Gunman's own
// internal weapon-ID numbering is unrelated to the stock HL enum),
// world model "models/w_aicore.mdl", view model "models/v_aicore.mdl",
// no ammo type (both pszAmmo1/2 stay null), no clip
// (iMaxClip = WEAPON_NOCLIP), pickup sound "buttons/button7.wav" -
// and confirms there is NOT a single firing/weapon sound precached
// anywhere, matching the doc's conclusion that this is a
// carry-and-use quest item (presumably interacted with at
// button_aiwallplug terminals, see rust7d_misc.cpp) rather than a
// combat weapon.
//
// Simplified relative to the original: the actual player-side
// interaction with button_aiwallplug (how "using" this item at a
// terminal is detected/triggered) was not located in either the
// findings doc or this session's decompile - PrimaryAttack/
// SecondaryAttack are left as CBasePlayerWeapon's no-op defaults.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"

class CAiCore : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	bool GetItemInfo(ItemInfo* p) override;
	bool Deploy() override;
};
LINK_ENTITY_TO_CLASS(weapon_aicore, CAiCore);

void CAiCore::Spawn()
{
	pev->classname = MAKE_STRING("weapon_aicore");
	Precache();
	m_iId = WEAPON_AICORE;
	SET_MODEL(ENT(pev), "models/w_aicore.mdl");

	FallInit();
}

void CAiCore::Precache()
{
	PrecacheModel("models/v_aicore.mdl");
	PrecacheModel("models/w_aicore.mdl");
	PrecacheSound("buttons/button7.wav");
}

bool CAiCore::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = nullptr;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 5;
	p->iPosition = 0;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AICORE;
	p->iWeight = 0;

	return true;
}

bool CAiCore::Deploy()
{
	return DefaultDeploy("models/v_aicore.mdl", "models/w_aicore.mdl", 0, "onehanded");
}
