//=========================================================
// ammo_chemical - first gap of the REBAR0A/REBAR2* block in the
// map-driven pass. A CBasePlayerAmmo pickup for weapon_chemgun's
// primary "ChemicalAcid" ammo type - see
// findings/weapons/weapon_chemgun.md (weapon_chemgun itself is not a
// gap on any scanned map so far and remains unimplemented/undeployed;
// only its ammo pickup is placed).
//
// Decompiled fresh from gunman.dll: LINK @0x100986d0, vtable
// @0x100fe0bc, Spawn @0x10098730 -> shared CBasePlayerAmmo spawn tail
// @0x100633c0 (same shared tail already confirmed for
// ammo_dmlsingle/ammo_beamgunclip elsewhere in this project - a real
// CBasePlayerAmmo, not CItem), Precache @0x10098760. Confirmed model
// "models/chem_ammo.mdl", pickup sound "items/9mmclip1.wav" (same
// generic clip-pickup sound reused by every other small ammo pickup
// in this project). Ammo type "ChemicalAcid" and its max/give amounts
// are only decompiled for weapon_chemgun's GetItemInfo() (25 max per
// findings/weapons/weapon_chemgun.md's Session-74 note); the pickup's
// own AddAmmo() give-amount was not separately decompiled this
// session - approximated as a 15-unit give, matching the give-vs-max
// ratio already established for this project's other clip pickups
// (e.g. ammo_beamgunclip's 4-of-40).
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"

class CChemicalAmmo : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/chem_ammo.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PrecacheModel("models/chem_ammo.mdl");
		PrecacheSound("items/9mmclip1.wav");
	}
	bool AddAmmo(CBaseEntity* pOther) override
	{
		if (pOther->GiveAmmo(15, "ChemicalAcid", 25) != -1) // give amount approximated, max=25 confirmed via weapon_chemgun GetItemInfo, see file header
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return true;
		}
		return false;
	}
};
LINK_ENTITY_TO_CLASS(ammo_chemical, CChemicalAmmo);
