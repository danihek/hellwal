/* integration for comfort.h. */
#include "comfort.h"

static NA_LAB comfort_accent_seed;
static int comfort_tonal_variants;
static RGB comfort_special[5];
static const char *comfort_special_names[] = {
    "accent", "accent_foreground", "selection_background", "selection_foreground", "cursor_text"
};

static PALETTE comfort_image_palette(void)
{
    NA_EXTRACT ex;
    IMG *img = img_load(ARGS.IMAGE);
    if (!na_extract(img->pixels, img->width, img->height, &ex)) err("Cannot extract OKLab colors");
    img_free(img);
    comfort_tonal_variants = ex.count < 6;
    double image_chroma = 0;
    for (unsigned i = 0; i < ex.count; i++) image_chroma = fmax(image_chroma, na_chroma(ex.colors[i].lab));
    if (image_chroma < .00001) comfort_tonal_variants = 1;
    NA_LAB colors[6], bg;
    na_representatives(&ex, colors, &bg, &comfort_accent_seed, ARGS.DOMINANT_BACKGROUND, !ARGS.SKIP_LUMINANCE_SORTING);
    PALETTE p = {0};
    bg = na_cap(bg, .03); bg.L = .24;
    p.colors[0] = na_to_rgb(bg);
    for (int i = 0; i < 6; i++) {
        NA_LAB c = na_cap(colors[i], .14);
        /* Normalize seed lightness without losing hue/chroma on dark solids. */
        c.L = .62 + .012*i;
        p.colors[i+1] = na_to_rgb(c);
        c.L += .08;
        p.colors[i+9] = na_to_rgb(c);
    }
    bg.L = .7; p.colors[7] = p.colors[15] = na_to_rgb(bg);
    bg.L = .55; p.colors[8] = na_to_rgb(bg);
    return p;
}

static double comfort_target(int i)
{
    if (i == 7 || i == 15) return 7.2;
    if (i == 8) return 4.7;
    return (i > 8 ? 6.2 : 5.2) + (comfort_tonal_variants ? .035*((i-1)%8) : 0);
}

static NA_LAB comfort_effects(NA_LAB c)
{
    if (ARGS.INVERT) {
        RGB rgb = na_to_rgb(c);
        c = na_from_rgb((RGB){255-rgb.R, 255-rgb.G, 255-rgb.B});
    }
    c.L = na_clamp(c.L + ARGS.OFFSET_GLOBAL*.25, 0, 1);
    if (ARGS.GRAY_SCALE >= 0) {
        c.a *= 1-ARGS.GRAY_SCALE; c.b *= 1-ARGS.GRAY_SCALE;
    }
    return c;
}

/* Restore fixed theme slots before this function; only adjustable colors ever
 * pass through the new effects. Keep the classic flag application order:
 * neon, dark, light, color, dominant background, inversion, offsets, grayscale,
 * static colors; then the new final readability solve.
 */
static void comfort_apply(PALETTE *p)
{
    uint16_t fixed = ARGS.THEME ? (uint16_t)~ARGS.OVERRIDE_COLORS : 0;
    if (ARGS.STATIC_BG) { p->colors[0] = *ARGS.STATIC_BG; fixed |= 1; }
    if (ARGS.STATIC_FG) { p->colors[15] = *ARGS.STATIC_FG; fixed |= 1u<<15; }
    double cap = ARGS.NEON_MODE ? .20 : .14;
    NA_LAB seed[16];
    int light = 0;
    if (ARGS.DARK_MODE) light = 0;
    if (ARGS.LIGHT_MODE) light = 1;
    if (ARGS.COLOR_MODE) light = 1;
    for (int i = 0; i < 16; i++) {
        seed[i] = na_from_rgb(p->colors[i]);
        if (fixed & (1u<<i)) continue;
        if (ARGS.NEON_MODE && i != 0 && i != 7 && i != 8 && i != 15) {
            seed[i].a *= 1.4; seed[i].b *= 1.4;
        }
        if (i == 0) {
            seed[i].L = light ? .94 : .24;
            /* Color mode strengthens the image tint without adding a hue. */
            if (ARGS.COLOR_MODE) { seed[i].a *= 2; seed[i].b *= 2; }
        }
        seed[i] = comfort_effects(seed[i]);
    }
    if (!(fixed & 1)) {
        /* Offsets and inversion may move a background, but cannot strand all
         * body text in the middle-luminance region with no 7.2:1 solution. */
        int bright = seed[0].L > .6;
        seed[0].L = na_clamp(seed[0].L, bright ? .88 : .14, bright ? .98 : .32);
        seed[0] = na_cap(seed[0], ARGS.COLOR_MODE ? .06 : .03);
        p->colors[0] = na_rgb_capped(seed[0], ARGS.COLOR_MODE ? .06 : .03);
        /* With fixed text, search background lightness for the nearest feasible
         * solution, or maximize the weakest normalized target if contradictory. */
        if (fixed & 0xfffe) {
            double best_score = -1, best_distance = 2;
            RGB best_bg = p->colors[0];
            for (int step = 0; step <= 1000; step++) {
                NA_LAB candidate = seed[0]; candidate.L = step/1000.0;
                RGB rgb = na_rgb_capped(candidate, ARGS.COLOR_MODE ? .06 : .03);
                double score = 1;
                for (int i = 1; i < 16; i++) if (fixed & (1u<<i))
                    score = fmin(score, na_contrast(p->colors[i], rgb)/comfort_target(i));
                /* Adjustable main text must remain solvable as well. */
                score = fmin(score, fmax(na_contrast((RGB){0,0,0}, rgb), na_contrast((RGB){255,255,255}, rgb))/7.2);
                double distance = fabs(candidate.L-seed[0].L);
                if (score > best_score+1e-10 || (fabs(score-best_score) < 1e-10 && distance < best_distance)) {
                    best_score = score; best_distance = distance; best_bg = rgb;
                }
            }
            p->colors[0] = best_bg;
        }
    }
    RGB bg = p->colors[0];
    for (int i = 1; i < 16; i++) {
        double target = comfort_target(i);
        if (!(fixed & (1u<<i)))
            p->colors[i] = na_fit(seed[i], bg, target, i == 7 || i == 8 || i == 15 ? .03 : cap);
    }
    NA_LAB accent = comfort_accent_seed;
    if (ARGS.NEON_MODE) { accent.a *= 1.4; accent.b *= 1.4; }
    accent = comfort_effects(accent);
    comfort_special[0] = na_fit(accent, bg, 5.2, cap);
    comfort_special[1] = na_fit((NA_LAB){.5,0,0}, comfort_special[0], 5.2, 0);
    comfort_special[2] = comfort_special[0];
    comfort_special[3] = comfort_special[1];
    comfort_special[4] = na_fit((NA_LAB){.5,0,0}, p->colors[15], 5.2, 0);
}

static void comfort_diagnostics(const PALETTE *p)
{
    uint16_t fixed = ARGS.THEME ? (uint16_t)~ARGS.OVERRIDE_COLORS : 0;
    if (ARGS.STATIC_BG) fixed |= 1;
    if (ARGS.STATIC_FG) fixed |= 1u<<15;
    RGB bg = p->colors[0];
    for (int i = 1; i < 16; i++) {
        double target = comfort_target(i);
        double ratio = na_contrast(p->colors[i], bg);
        if (ratio + .000001 < target)
            warn("--comfort-mode: color%d/background contrast %.2f:1 is below %.1f:1 (%s; fixed values preserved)",
                 i, ratio, target, fixed & (1u<<i) ? "fixed text constraint" : "fixed background constraint");
    }
    if (na_contrast(comfort_special[0], bg) < 5.2)
        warn("--comfort-mode: accent/background cannot reach 5.2:1 with fixed background (%.2f:1)", na_contrast(comfort_special[0], bg));
    if (na_contrast(comfort_special[4], p->colors[15]) < 5.2)
        warn("--comfort-mode: cursor_text/cursor cannot reach 5.2:1 with fixed cursor (%.2f:1)", na_contrast(comfort_special[4], p->colors[15]));
}

static int comfort_resolve(const char *name, RGB *out)
{
    if (!ARGS.COMFORT_MODE) return 0;
    for (int i = 0; i < 5; i++) if (!strcmp(name, comfort_special_names[i])) {
        *out = comfort_special[i]; return 1;
    }
    return 0;
}

/* JSON mode uses a writer so filenames containing quotes/control characters
 * remain valid JSON. Classic output continues through its original template. */
static char *comfort_json_escape(const char *s)
{
    char *escaped = malloc(strlen(s)*6 + 1);
    if (!escaped) err("Cannot allocate JSON string");
    char *q = escaped;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '"' || *p == '\\') { *q++ = '\\'; *q++ = *p; }
        else if (*p < 32) { snprintf(q, 7, "\\u%04x", *p); q += 6; }
        else *q++ = *p;
    }
    *q = '\0';
    return escaped;
}

static void comfort_json_string(const char *s)
{
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '"' || *p == '\\') printf("\\%c", *p);
        else if (*p < 32) printf("\\u%04x", *p);
        else putchar(*p);
    }
    putchar('"');
}

static void comfort_json(PALETTE p)
{
    printf("{\n  \"wallpaper\": "); comfort_json_string(ARGS.IMAGE);
    printf(",\n  \"alpha\": \"100\",\n  \"special\": {\n");
    const char *aliases[] = {"background", "foreground", "cursor", "border"};
    for (int i = 0; i < 9; i++) {
        RGB c = i < 4 ? p.colors[i == 0 ? 0 : 15] : comfort_special[i-4];
        printf("    \"%s\": \"#%02x%02x%02x\"%s\n", i < 4 ? aliases[i] : comfort_special_names[i-4], c.R, c.G, c.B, i == 8 ? "" : ",");
    }
    printf("  },\n  \"colors\": {\n");
    for (int i = 0; i < 16; i++) {
        RGB c = p.colors[i];
        printf("    \"color%d\": \"#%02x%02x%02x\"%s\n", i, c.R, c.G, c.B, i == 15 ? "" : ",");
    }
    printf("  }\n}\n");
}
