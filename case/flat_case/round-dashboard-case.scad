// =====================================================================
//  Round Dashboard — Braun-style white puck case  (parametric OpenSCAD)
//  Device: ESP32-2424S012 (round 1.28" GC9A01), USB-C powered (no battery).
//
//  A minimal matte-white puck. The front is an OPEN, FLAT-TOP BEZEL RING —
//  the module's own screen shows through the empty centre and is pressed
//  directly: a press floats the bezel+board ~`travel` and actuates the GPIO9
//  BOOT switch against a post in the body ("click the screen"); touch works too.
//
//  Print notes:
//   * The bezel's FLAT TOP is the show face — print it FACE-DOWN on the plate
//     (skirt pointing up) for the smoothest finish, no supports. Show faces are
//     kept flat; edges get chamfers (printable face-down), not fillets.
//
//  Geometry from hardwarespec.md (datasheet [DS]). Board = Ø37 circle + a
//  14.23 x 3.74 USB tab (38.5 overall). MEASURE ME values — esp. boot_x/boot_y
//  and glass_dia — must be confirmed with calipers. `make render`, print, iterate.
// =====================================================================

/* [What to render] */
part = "fit";   // bezel | body | pcb | fit | section | both | jig

/* [Board — datasheet, Ø circle + USB tab] */
board_dia   = 37.0;   // [DS] round PCB diameter
overall_h   = 38.5;   // [DS] circle + tab total height
tab_w       = 14.23;  // [DS] USB tab width (centred, bottom)
tab_h       = 3.74;   // [DS] USB tab height
board_th    = 1.6;    // [MEASURE]
back_components = 3.0;// [MEASURE] module ~2.4, USB-C shell ~3.2

/* [Display — datasheet] */
active_dia    = 32.4; // [DS] visible pixels
glass_dia     = 35.0; // [MEASURE] cover glass (< board_dia; leaves a PCB rim to grip)
glass_stand   = 1.5;  // [MEASURE] glass top above PCB front
screen_recess = 0.3;  // bezel top sits this far above the glass (screen ~flush)

/* [Press-the-face click — MEASURE ME]
   The datasheet p.5 photo is a BACK/component-side view; BOOT is upper-RIGHT there.
   The case body + post sit on that SAME back side, so seen from the back the post
   must also land upper-RIGHT — i.e. in the model frame (screen = +z, +x = screen-
   right) that is boot_x NEGATIVE = upper-LEFT on the screen face. Confirmed w/ user.
   No dimensioned coord in the datasheet, so the magnitude is scaled off p.5 (~±2 mm);
   the wide pad (boot_pad_d) absorbs that error — jig-measure to finalise. */
boot_x = -10.0;  // [MEASURE ±2mm] BOOT switch X (photo upper-RIGHT → screen upper-LEFT) << CRITICAL
boot_y =  10.0;  // [MEASURE ±2mm] BOOT switch Y                                          << CRITICAL
boot_post_d = 3.0;    // actuation post shaft
boot_pad_d  = 5.0;    // wide contact pad on the tip (covers ±2mm XY miss; <= module gap)
boot_sw_h   = 1.2;    // [MEASURE] switch cap protrusion below the PCB back face
travel      = 0.5;    // front float / click travel

/* [Board standoff — no battery; back components clear the floor] */
board_lift  = 0.5;    // gap under the tallest back component

/* [USB-C tab opening — bottom tab, racetrack shape, slightly off-centre — MEASURE ME] */
usb_x      =  2.0;    // [MEASURE] port off-centre: photo shows it a touch LEFT (back view) → +x
usb_slot_w = 10.0;    // [MEASURE] racetrack width (clears the USB-C plug overmold)
usb_slot_h = 5.0;     // [MEASURE] racetrack height

/* [Shell / finish] */
wall        = 2.0;
skirt_th    = 1.4;
floor_th    = 2.0;
bezel_cham  = 1.0;    // 45deg chamfer on the bezel's outer top edge
tilt_deg    = 15;     // preview-only tilt (stand foot = TODO)

/* [3D-print tolerances — standard calibrated FDM (0.4mm nozzle / 0.2mm layer)]
   Re-calibrate with a fit test if your printer differs; change here in ONE place. */
tol_press = 0.10;     // interference / snap features
tol_slip  = 0.20;     // sliding fits: board->skirt, bezel->body
tol_loose = 0.40;     // free clearance: battery pocket, USB / tab openings
rim_gap   = 0.40;     // visible hairline seam / float clearance

$fn = 96;             // bump to 160+ for final export

// ---------------------------------------------------------------------
//  Derived
// ---------------------------------------------------------------------
board_r  = board_dia / 2;
tab_cy   = -(board_r + (overall_h - board_dia) - tab_h/2);

skirt_ir = board_r + tol_slip;       // board slides into the skirt
skirt_or = skirt_ir + skirt_th;
body_ir  = skirt_or + rim_gap;       // skirt floats inside the body
body_or  = body_ir + wall;

screen_open = glass_dia + tol_loose; // open bezel window (glass passes through)

// vertical stack, z=0 at back, +z toward the screen
z_boardback = floor_th + back_components + board_lift;  // back components clear the floor
z_boardfrnt = z_boardback + board_th;
z_glass_top = z_boardfrnt + glass_stand;
z_body_top  = z_glass_top + screen_recess;   // flat bezel show face
bezel_th    = z_body_top - z_boardfrnt;       // flat ring thickness (rests on PCB border)
body_h      = z_body_top;
z_seam      = z_boardfrnt;                     // body meets the bezel at the board front
// The BOOT cap protrudes `boot_sw_h` below the PCB back; leave exactly `travel` of gap.
z_post_top  = z_boardback - boot_sw_h - travel;

// ---------------------------------------------------------------------
//  Helpers
// ---------------------------------------------------------------------
module board_outline() { union() {           // circle + bottom tab
    circle(d = board_dia);
    translate([0, tab_cy]) square([tab_w, tab_h + 2], center = true);
} }

// Interior cavity the board drops into (from the front): the round pocket (sized for
// the bezel skirt) UNION a bay at the bottom for the USB tab, so the round+tab PCB
// seats and the tab keys into the bay. Open at the top for insertion.
tab_tip_y = -(board_r + (overall_h - board_dia));   // real tab bottom edge, ≈ −20
module cavity_2d() {
    circle(r = body_ir);
    // tab bay: from well inside the circle out to the tab tip + clearance
    bay_out = tab_tip_y - tol_loose;                 // ≈ −20.4
    bay_in  = -(board_r - 5);                         // overlaps into the circle
    translate([0, (bay_out + bay_in) / 2])
        offset(r = 1) offset(r = -1)                 // round the two outer corners
            square([tab_w + 2*tol_loose, bay_in - bay_out], center = true);
}

// USB-C window in the body's BOTTOM wall (−Y, under the board's tab). A slot with a
// rounded bottom that runs OPEN through the top rim — so there is no thin (unprintable)
// bridge of wall above the port; the bezel ring rests on the rim and caps it. Nudged
// off-centre by usb_x; the rounded bottom sits just above the floor to clear the plug.
module usb_cut() {
    zc = floor_th + usb_slot_h/2;          // centre of the rounded bottom
    dx = (usb_slot_w - usb_slot_h) / 2;    // cap offset → width usb_slot_w
    hull() {
        // rounded bottom (racetrack half), radially through the wall
        translate([usb_x, -(body_or + 1), zc])
            rotate([-90, 0, 0])
                hull() for (s = [-1, 1])
                    translate([s * dx, 0, 0]) cylinder(d = usb_slot_h, h = wall * 3);
        // square top carried up THROUGH the rim (hidden under the bezel) — kills the bridge
        translate([usb_x, -(body_or + 1) + wall*1.5, z_seam + 1])
            cube([usb_slot_w, wall * 3, 2], center = true);
    }
}

// ---------------------------------------------------------------------
//  BODY  (fixed: plain cylinder shell, floor, boot post — no battery)
// ---------------------------------------------------------------------
module body() {
    difference() {
        union() {
            // shell up to the seam; interior = round pocket + USB-tab bay (board drops in)
            difference() {
                cylinder(r = body_or, h = z_seam);
                translate([0, 0, floor_th])
                    linear_extrude(z_seam - floor_th + 0.01) cavity_2d();
            }
            // boot post: a slim shaft rising from the floor to a WIDE contact pad that
            // strikes the GPIO9 switch. The pad (chamfered top) covers a ~±2mm XY miss.
            translate([boot_x, boot_y, floor_th - 0.01]) {
                pad_h = min(1.2, z_post_top - floor_th);   // pad thickness (top slice)
                cylinder(d = boot_post_d, h = z_post_top - floor_th - pad_h + 0.01);
                translate([0, 0, z_post_top - floor_th - pad_h])
                    cylinder(d1 = boot_pad_d - 2*bezel_cham, d2 = boot_pad_d, h = bezel_cham);
                translate([0, 0, z_post_top - floor_th - pad_h + bezel_cham])
                    cylinder(d = boot_pad_d, h = pad_h - bezel_cham);
            }
        }
        usb_cut();
    }
}

// ---------------------------------------------------------------------
//  BEZEL  (printed FLAT-TOP-DOWN. Open centre = full screen; rests on the PCB
//          border ring; skirt grips the board sides.)
// ---------------------------------------------------------------------
module bezel() {
    union() {
        // flat-top ring, 45deg chamfer on the outer top edge only
        translate([0, 0, z_boardfrnt])
            rotate_extrude()
                polygon([
                    [screen_open/2,        0],
                    [screen_open/2,        bezel_th],
                    [body_or - bezel_cham, bezel_th],
                    [body_or,              bezel_th - bezel_cham],
                    [body_or,              0],
                ]);
        // skirt gripping the board sides (hidden under the ring), tab notch at bottom
        translate([0, 0, z_boardback])
            linear_extrude(z_boardfrnt - z_boardback + 0.02)
                difference() {
                    circle(r = skirt_or);
                    circle(r = skirt_ir);
                    translate([0, tab_cy]) square([tab_w + 2*tol_loose, tab_h + 4], center = true);
                }
    }
}

// ---------------------------------------------------------------------
//  PCB  (REFERENCE ONLY — not printed. The real module, for fit-checking.)
// ---------------------------------------------------------------------
module pcb() {
    color([0.13, 0.42, 0.22])                                   // green board + tab
        translate([0, 0, z_boardback]) linear_extrude(board_th) board_outline();
    color([0.13, 0.16, 0.22])                                   // cover glass
        translate([0, 0, z_boardfrnt]) cylinder(d = glass_dia, h = glass_stand);
    color([0.03, 0.04, 0.06])                                   // active screen
        translate([0, 0, z_glass_top - 0.05]) cylinder(d = active_dia, h = 0.3);
    color([0.15, 0.15, 0.15])                                   // BOOT switch nub (back)
        translate([boot_x, boot_y, z_boardback - 0.5]) cylinder(d = boot_post_d - 0.6, h = 0.5);
    color([0.55, 0.55, 0.55])                                   // USB-C shell on the tab (off-centre by usb_x)
        translate([usb_x, tab_cy, z_boardback - 1.6]) cube([9, 7, 3.2], center = true);
    color([0.2, 0.2, 0.2])                                      // module block (back)
        translate([-7.5, 2, z_boardback - back_components]) cube([15, 13, back_components]);
}

// ---------------------------------------------------------------------
//  MEASUREMENT JIG — drops onto the PCB back (keyed by the tab), 2 mm hole
//  grid to read the BOOT switch X/Y.
// ---------------------------------------------------------------------
module jig() {
    difference() {
        linear_extrude(1.2) offset(r = 2) board_outline();
        linear_extrude(2)   offset(r = tol_slip) board_outline();
        for (x = [-board_r : 2 : board_r], y = [-board_r : 2 : board_r])
            if (x*x + y*y < board_r*board_r)
                translate([x, y, -0.1]) cylinder(d = 0.8, h = 2);
    }
}

// ---------------------------------------------------------------------
//  Assemblies
// ---------------------------------------------------------------------
module assembled() {                 // at rest: front (bezel+pcb) floated up by `travel`
    color("white") body();
    translate([0, 0, travel]) { color("white") bezel(); pcb(); }
}

// ---------------------------------------------------------------------
//  Output
// ---------------------------------------------------------------------
if (part == "body")    color("white") rotate([tilt_deg,0,0]) body();
if (part == "bezel")   color("white") rotate([tilt_deg,0,0]) bezel();

// print-ready: bezel flipped flat-show-face DOWN onto the bed (skirt pointing up)
if (part == "bezel_print") translate([0, 0, z_body_top]) rotate([180, 0, 0]) bezel();
if (part == "pcb")     rotate([tilt_deg,0,0]) pcb();
if (part == "jig")     color("white") jig();

if (part == "both")    rotate([tilt_deg,0,0]) { color("white") body();
                        color("white") translate([0,0,travel]) bezel(); }

if (part == "fit")     rotate([tilt_deg,0,0]) assembled();

if (part == "section")                        // cut rotated 90° → remove the +X (right) half
    difference() {
        assembled();
        translate([0, -100, -50]) cube([120, 200, 200]);
    }
