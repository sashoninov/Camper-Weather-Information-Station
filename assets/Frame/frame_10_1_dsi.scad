// 10.1" Waveshare DSI front frame
// Outer glass: 239 x 147 mm
// Border: 7 mm, inner bevel 30°

glass_w = 239;
glass_h = 147;

border = 7;
tol    = 0.3;      // хлабина към стъклото
thick  = 2.0;      // обща дебелина на рамката
bevel_angle = 30;  // вътрешен скос

module frame() {
    outer_w = glass_w + 2*border;
    outer_h = glass_h + 2*border;
    inner_w = glass_w + 2*tol;
    inner_h = glass_h + 2*tol;

    // основна плоча
    difference() {
        // външен контур
        cube([outer_w, outer_h, thick], center=true);

        // прав вътрешен отвор (дъното на скоса)
        translate([0,0,-0.01])
            cube([inner_w, inner_h, thick+0.02], center=true);

        // скос – изрязваме клин към отвора
        bevel_h = thick; // целия профил
        bevel_offset = (thick) * tan(bevel_angle * PI/180);

        translate([0,0,thick/2])
        linear_extrude(height=bevel_h, center=true, scale=[
            (inner_w + 2*bevel_offset)/inner_w,
            (inner_h + 2*bevel_offset)/inner_h
        ])
            square([inner_w, inner_h], center=true);
    }
}

frame();
