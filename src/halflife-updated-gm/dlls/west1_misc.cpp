//=========================================================
// west1's remaining gaps: decore_eagle (CBaseBird-family ambient
// flying prop) and player_giveitems (confirmed dead cut-content stub,
// see gunman_custom_entities.cpp's CPlayerToggleHud for the identical
// pattern). weapon_dml (M.U.L.E. rocket launcher), also a gap on this
// map, is deferred - see [[project-weapon-dml-deferred]]-equivalent
// tracking in findings/weapons/weapon_dml.md; its PrimaryAttack was
// never located project-wide even before this session.
//
// Decompiled fresh from gunman.dll:
//   decore_eagle: LINK @0x10069ce0, vtable @0x100f6cf0, Spawn
//     @0x10069d90, Precache @0x10069f00, Classify (slot 8, @0x10069d70)
//     confirmed constant 0 (CLASS_NONE). Spawn's own body only sets the
//     model and calls a shared CBaseBird spawn-tail helper
//     @0x100689a0, the SAME helper already confirmed and documented in
//     mayan0a_fauna.cpp for decore_pteradon (CPteradon) -
//     bbox(-32,-32,-16)/(32,32,16) hardcoded floats in the helper
//     (0x42000000=32/0x41800000=16), FL_FLY+MOVETYPE_FLY(5)/SOLID_BBOX
//     (2), takedamage=DAMAGE_YES(0x40000000=2), health=10
//     (0x41200000), random start frame, and real hunting-flight AI via
//     CBaseBird::HuntThink/CBaseBird::FlyTouch (gunman.dll's own
//     internal base class, not decompiled in full this session - same
//     scope decision as CPteradon). Reproduced here with the exact
//     same CircleThink simplification already established for
//     CPteradon rather than re-deriving a third variant of the same
//     rationale.
//   player_giveitems: LINK @0x10043e70, vtable @0x100f2684, Spawn
//     (slot 0, @0x10052db0) - BYTE-IDENTICAL to player_togglehud's
//     Spawn (same address, same single `pev->solid = SOLID_NOT`
//     effect, same 0x20-byte object size with no class-owned
//     KeyValue/Touch/Use). Per
//     findings/entities/misc_gunman_custom_entities.md, both are
//     confirmed dead/cut-content stub classes in the retail build -
//     reproduced identically to CPlayerToggleHud in
//     gunman_custom_entities.cpp.
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"

//=========================================================
// player_giveitems - functionally dead stub, see file header.
//=========================================================
class CPlayerGiveItems : public CBaseEntity
{
public:
	void Spawn() override { pev->solid = SOLID_NOT; }
};
LINK_ENTITY_TO_CLASS(player_giveitems, CPlayerGiveItems);

//=========================================================
// decore_eagle - CEagle. Ambient killable flying prop, same
// CBaseBird-family CircleThink simplification as CPteradon.
//=========================================================
class CEagle : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_NONE; }
	void EXPORT CircleThink();
};
LINK_ENTITY_TO_CLASS(decore_eagle, CEagle);

void CEagle::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/eagle.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, -16), Vector(32, 32, 16));
	UTIL_SetOrigin(pev, pev->origin);

	pev->flags |= FL_FLY;
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->health = 10;
	pev->frame = RANDOM_LONG(0, 255);

	MonsterInit();
	SetThink(&CEagle::CircleThink);
	pev->nextthink = gpGlobals->time + 1.0;
}

void CEagle::Precache()
{
	PrecacheModel("models/eagle.mdl");
	PrecacheSound("eagle/eagle_cry1.wav");
	PrecacheSound("eagle/eagle_cry2.wav");
	PrecacheSound("eagle/eagle_cry3.wav");
	PrecacheSound("eagle/eagle_flap1.wav");
	PrecacheSound("eagle/eagle_flap2.wav");
	PrecacheSound("eagle/eagle_flap3.wav");
}

void CEagle::CircleThink()
{
	pev->nextthink = gpGlobals->time + 0.1;

	pev->angles.y += 3.0;
	if (pev->angles.y > 360)
		pev->angles.y -= 360;

	UTIL_MakeVectors(pev->angles);
	pev->velocity = gpGlobals->v_forward * 60;
}
