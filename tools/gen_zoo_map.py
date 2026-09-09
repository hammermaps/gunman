#!/usr/bin/env python3
"""Generate a hand-authored .map for the monster zoo test level: a
single large hollow room (6 slab brushes) plus every zoo monster as a
point entity on the flat floor, an info_player_start, and a simple
3-brush drivable vehicle_tank right next to spawn."""
import sys

FLOOR_TEX = "FLOOR6"
WALL_TEX = "GRAYWALL"

def box_brush(x1, y1, z1, x2, y2, z2, tex):
    x1, y1, z1, x2, y2, z2 = map(int, (x1, y1, z1, x2, y2, z2))
    lines = ["{"]
    # Valve220 texture format: TEXTURE [ux uy uz uoff] [vx vy vz voff] rot sx sy
    faces = [
        (((x1, y1, z2), (x2, y1, z2), (x2, y2, z2)), (1, 0, 0), (0, -1, 0)),  # top
        (((x1, y2, z1), (x2, y2, z1), (x2, y1, z1)), (1, 0, 0), (0, -1, 0)),  # bottom
        (((x2, y1, z1), (x2, y2, z1), (x2, y2, z2)), (0, 1, 0), (0, 0, -1)),  # +x
        (((x1, y2, z1), (x1, y1, z1), (x1, y1, z2)), (0, 1, 0), (0, 0, -1)),  # -x
        (((x2, y2, z1), (x1, y2, z1), (x1, y2, z2)), (1, 0, 0), (0, 0, -1)),  # +y
        (((x1, y1, z1), (x2, y1, z1), (x2, y1, z2)), (1, 0, 0), (0, 0, -1)),  # -y
    ]
    for (p1, p2, p3), u, v in faces:
        # zhlt's PlaneFromPoints computes normal = cross(p0-p1, p2-p1) for
        # points listed (p0,p1,p2) in the .map line - opposite handedness
        # from the "textbook" cross(p2-p1,p3-p1) convention. Emit points
        # reversed (p3,p2,p1) so the outward-normal faces come out correct;
        # verified against a minimal test brush compiled with hlcsg.
        p1, p3 = p3, p1
        lines.append(
            "( %d %d %d ) ( %d %d %d ) ( %d %d %d ) %s [ %d %d %d 0 ] [ %d %d %d 0 ] 0 1 1"
            % (p1[0], p1[1], p1[2], p2[0], p2[1], p2[2], p3[0], p3[1], p3[2], tex,
               u[0], u[1], u[2], v[0], v[1], v[2])
        )
    lines.append("}")
    return "\n".join(lines)


def main():
    roster_path, out_path = sys.argv[1], sys.argv[2]
    with open(roster_path) as f:
        roster = [l.strip() for l in f if l.strip()]

    # Grid layout for the packed zoo (see OVERSIZED_CLEARANCE below for
    # the handful of classes excluded from this grid). 300-unit spacing
    # (bumped from an initial 220 on 2026-09-06). Root-caused a
    # "monster_human_gunman stuck in wall" report to monster_osprey's
    # UTIL_SetSize(-400,-400,-100)/(400,400,32) - an 800x800-unit
    # bounding box, by far the largest of any zoo class (next largest
    # is monster_human_chopper at 192x192) - it was landing one grid
    # cell away and its huge hull physically overlapped the neighbour's
    # spawn cell even at 300 spacing. Fix: give oversized classes their
    # own isolated spot far outside the packed grid instead of trying
    # to brute-force enough spacing for the whole grid to accommodate
    # the single largest outlier.
    OVERSIZED_CLEARANCE = {
        "monster_osprey": 500,  # half-width 400 + margin
    }
    grid_roster = [cn for cn in roster if cn not in OVERSIZED_CLEARANCE]
    COLS = 7
    STEP = 300
    IX1, IY1, IZ1 = -400, -400, 0
    base_x, base_y, base_z = IX1 + 200, IY1 + 200, 40
    grid_rows = (len(grid_roster) - 1) // COLS + 1

    # Oversized classes go in their own row below the grid, centered on
    # the grid's X midpoint so their wide bounding box has clearance on
    # both sides too, not just front/back.
    oversized_y = base_y + (grid_rows + 1) * STEP
    grid_max_x = base_x + (COLS - 1) * STEP
    oversized_center_x = (base_x + grid_max_x) // 2
    oversized_positions = []
    y = oversized_y
    for cn, clearance in OVERSIZED_CLEARANCE.items():
        oversized_positions.append((cn, oversized_center_x, y, clearance))
        y += clearance * 2 + 100

    # Room bounds (interior) - sized to fit the grid plus the oversized
    # row (whichever needs more X/Y clearance).
    max_clearance = max((c for *_, c in oversized_positions), default=0)
    IX2 = max(base_x + (COLS - 1) * STEP + 200, oversized_center_x + max_clearance + 200)
    IX1 = min(IX1, oversized_center_x - max_clearance - 200)
    IY2 = (oversized_positions[-1][2] + oversized_positions[-1][3] + 200) if oversized_positions else (base_y + grid_rows * STEP + 200)
    IZ2 = 512
    WALL = 32

    out = []
    out.append('{\n"classname" "worldspawn"\n"mapversion" "220"\n"wad" "rewolf.wad;update.wad;decals.wad"\n')
    out.append(box_brush(IX1 - WALL, IY1 - WALL, IZ1 - WALL, IX2 + WALL, IY2 + WALL, IZ1, FLOOR_TEX))  # floor
    out.append(box_brush(IX1 - WALL, IY1 - WALL, IZ2, IX2 + WALL, IY2 + WALL, IZ2 + WALL, FLOOR_TEX))  # ceiling
    out.append(box_brush(IX1 - WALL, IY1 - WALL, IZ1, IX1, IY2 + WALL, IZ2, WALL_TEX))  # west wall
    out.append(box_brush(IX2, IY1 - WALL, IZ1, IX2 + WALL, IY2 + WALL, IZ2, WALL_TEX))  # east wall
    out.append(box_brush(IX1, IY1 - WALL, IZ1, IX2, IY1, IZ2, WALL_TEX))  # south wall
    out.append(box_brush(IX1, IY2, IZ1, IX2, IY2 + WALL, IZ2, WALL_TEX))  # north wall
    out.append("}")

    # player start near the room origin corner
    px, py, pz = -300, -300, 32
    out.append('{\n"origin" "%d %d %d"\n"classname" "info_player_start"\n}' % (px, py, pz))

    # light so the room isn't pitch black (hlrad needs at least one light
    # to avoid an all-black/undefined-lighting map)
    out.append('{\n"origin" "800 1000 480"\n"classname" "light"\n"_light" "300"\n}')

    for i, cn in enumerate(grid_roster):
        col = i % COLS
        row = i // COLS
        x = base_x + col * STEP
        y = base_y + row * STEP
        out.append(
            '{\n"origin" "%d %d %d"\n"angles" "0 0 0"\n"classname" "%s"\n"targetname" "zoo_%s"\n}'
            % (x, y, base_z, cn, cn)
        )

    for cn, x, y, _clearance in oversized_positions:
        out.append(
            '{\n"origin" "%d %d %d"\n"angles" "0 0 0"\n"classname" "%s"\n"targetname" "zoo_%s"\n}'
            % (x, y, base_z, cn, cn)
        )

    # simple 3-brush drivable tank right next to player start, matching
    # the vehicle_tank/_body/_turret/_barrel classname convention (see
    # a real placement in maps-src/west5b/west5b.ent for the keyvalue
    # pattern) - crude box shapes, not the real tank silhouette, but
    # functionally driveable since CVehicleTankBSP just needs a solid
    # brush model per part.
    tx, ty, tz = 0, -300, 0

    def brush_entity(classname, extra_kv, x1, y1, z1, x2, y2, z2, tex):
        kv = "".join('"%s" "%s"\n' % (k, v) for k, v in extra_kv.items())
        return (
            '{\n"classname" "%s"\n%s%s\n}'
            % (classname, kv, box_brush(x1, y1, z1, x2, y2, z2, tex))
        )

    out.append(brush_entity(
        "vehicle_tank_body", {"vehicle_id": "1", "renderamt": "0", "rendermode": "5"},
        tx - 64, ty - 96, tz, tx + 64, ty + 96, tz + 48, WALL_TEX))
    out.append(brush_entity(
        "vehicle_tank_turret", {"vehicle_id": "1", "renderamt": "0", "rendermode": "5"},
        tx - 32, ty - 32, tz + 48, tx + 32, ty + 32, tz + 72, WALL_TEX))
    out.append(brush_entity(
        "vehicle_tank_barrel", {"vehicle_id": "1", "renderamt": "0", "rendermode": "5"},
        tx - 8, ty + 32, tz + 52, tx + 8, ty + 96, tz + 64, WALL_TEX))
    out.append(
        '{\n"origin" "%d %d %d"\n"angles" "0 90 0"\n"vehicle_volume" "1"\n"vehicle_id" "1"\n"classname" "vehicle_tank"\n}'
        % (tx, ty, tz + 8)
    )

    with open(out_path, "w") as f:
        f.write("\n".join(out) + "\n")
    print("wrote", out_path, "with", len(roster), "monsters +", "1 tank")


if __name__ == "__main__":
    main()
