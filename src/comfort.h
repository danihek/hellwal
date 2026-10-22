/* 
 * Opt-in perceptual extraction. MIT license, like hellwal.
 * OKLab matrices: https://bottosson.github.io/posts/oklab/
 * Contrast: https://www.w3.org/TR/WCAG22/#dfn-relative-luminance
 * This module has no CLI, filesystem, or global-state dependencies.
 */
#ifndef HELLWAL_COMFORT_MODE_H
#define HELLWAL_COMFORT_MODE_H

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define NA_SAMPLES 65536u
#define NA_CLUSTERS 16
#define NA_ITERATIONS 24

typedef struct { double L, a, b; } NA_LAB;
typedef struct { NA_LAB lab; unsigned weight; } NA_CLUSTER;
typedef struct { NA_CLUSTER colors[NA_CLUSTERS]; unsigned count, samples; } NA_EXTRACT;

static double na_clamp(double v, double lo, double hi)
{
    return fmax(lo, fmin(hi, v));
}

static double na_linear(double v)
{
    return v <= 0.04045 ? v / 12.92 : pow((v + 0.055) / 1.055, 2.4);
}

static double na_srgb(double v)
{
    return v <= 0.0031308 ? 12.92 * v : 1.055 * pow(v, 1.0 / 2.4) - 0.055;
}

static NA_LAB na_from_rgb(RGB c)
{
    double r = na_linear(c.R / 255.0), g = na_linear(c.G / 255.0), b = na_linear(c.B / 255.0);
    double l = cbrt(0.4122214708*r + 0.5363325363*g + 0.0514459929*b);
    double m = cbrt(0.2119034982*r + 0.6806995451*g + 0.1073969566*b);
    double s = cbrt(0.0883024619*r + 0.2817188376*g + 0.6299787005*b);
    return (NA_LAB){0.2104542553*l + 0.7936177850*m - 0.0040720468*s,
                       1.9779984951*l - 2.4285922050*m + 0.4505937099*s,
                       0.0259040371*l + 0.7827717662*m - 0.8086757660*s};
}

static void na_to_linear(NA_LAB c, double out[3])
{
    double l = c.L + 0.3963377774*c.a + 0.2158037573*c.b;
    double m = c.L - 0.1055613458*c.a - 0.0638541728*c.b;
    double s = c.L - 0.0894841775*c.a - 1.2914855480*c.b;
    l *= l*l; m *= m*m; s *= s*s;
    out[0] =  4.0767416621*l - 3.3077115913*m + 0.2309699292*s;
    out[1] = -1.2684380046*l + 2.6097574011*m - 0.3413193965*s;
    out[2] = -0.0041960863*l - 0.7034186147*m + 1.7076147010*s;
}

static double na_chroma(NA_LAB c) { return hypot(c.a, c.b); }

static NA_LAB na_cap(NA_LAB c, double cap)
{
    double chroma = na_chroma(c);
    /* Suppress numerical chroma in genuinely neutral images. */
    double scale = chroma < 0.00001 ? 0 : fmin(1, cap / chroma);
    c.a *= scale; c.b *= scale;
    return c;
}

static int na_in_gamut(NA_LAB c)
{
    double v[3]; na_to_linear(c, v);
    return v[0] >= -1e-7 && v[0] <= 1.0000001 &&
           v[1] >= -1e-7 && v[1] <= 1.0000001 &&
           v[2] >= -1e-7 && v[2] <= 1.0000001;
}

/* Reduce only chroma, preserving OKLCH hue and lightness. */
static RGB na_to_rgb(NA_LAB c)
{
    c.L = na_clamp(c.L, 0, 1);
    if (!na_in_gamut(c)) {
        double lo = 0, hi = 1;
        for (int i = 0; i < 24; i++) {
            double mid = (lo + hi) / 2;
            NA_LAB probe = {c.L, c.a * mid, c.b * mid};
            if (na_in_gamut(probe)) lo = mid; else hi = mid;
        }
        c.a *= lo; c.b *= lo;
    }
    double v[3]; na_to_linear(c, v);
    return (RGB){(uint8_t)lround(255 * na_clamp(na_srgb(v[0]), 0, 1)),
                 (uint8_t)lround(255 * na_clamp(na_srgb(v[1]), 0, 1)),
                 (uint8_t)lround(255 * na_clamp(na_srgb(v[2]), 0, 1))};
}

/* Quantization can increase chroma slightly. Back off chroma at fixed hue/L
 * until the emitted bytes also respect the cap, not just the floating seed. */
static RGB na_rgb_capped(NA_LAB c, double cap)
{
    c = na_cap(c, cap);
    for (int i = 0; i < 32; i++) {
        RGB rgb = na_to_rgb(c);
        if (na_chroma(na_from_rgb(rgb)) <= cap + 1e-7) return rgb;
        c.a *= .98; c.b *= .98;
    }
    c.a = c.b = 0;
    return na_to_rgb(c);
}

static double na_luminance(RGB c)
{
    return .2126*na_linear(c.R/255.0) + .7152*na_linear(c.G/255.0) + .0722*na_linear(c.B/255.0);
}

static double na_contrast(RGB a, RGB b)
{
    double x = na_luminance(a), y = na_luminance(b);
    return (fmax(x, y) + .05) / (fmin(x, y) + .05);
}

/* Bounded lightness search; every decision uses the final quantized RGB.
 * Try the side with the most available contrast. Return its endpoint if the
 * requested target is impossible (the caller diagnoses fixed constraints).
 */
static RGB na_fit(NA_LAB seed, RGB bg, double target, double cap)
{
    seed = na_cap(seed, cap);
    int light = na_contrast((RGB){255,255,255}, bg) > na_contrast((RGB){0,0,0}, bg);
    double lo = light ? na_from_rgb(bg).L : 0;
    double hi = light ? 1 : na_from_rgb(bg).L;
    seed.L = light ? hi : lo;
    RGB result = na_rgb_capped(seed, cap);
    if (na_contrast(result, bg) < target) return result;
    for (int i = 0; i < 28; i++) {
        seed.L = (lo + hi) / 2;
        RGB probe = na_rgb_capped(seed, cap);
        int passes = na_contrast(probe, bg) >= target;
        if (passes) result = probe;
        if (passes == light) hi = seed.L; else lo = seed.L;
    }
    return result;
}

static double na_distance(NA_LAB a, NA_LAB b)
{
    double L = a.L-b.L, x = a.a-b.a, y = a.b-b.b;
    return L*L + x*x + y*y;
}

/* Deterministic stratified spatial samples, compressed into 5-bit RGB bins.
 * Store actual channel sums, not bin centers, so solids and grays stay exact.
 */
static int na_extract(const uint8_t *pixels, unsigned width, unsigned height, NA_EXTRACT *out)
{
    typedef struct { unsigned r, g, b, n; } BIN;
    BIN *bins = calloc(32768, sizeof(*bins));
    NA_CLUSTER *points = calloc(32768, sizeof(*points));
    if (!bins || !points || !width || !height) { free(bins); free(points); return 0; }
    *out = (NA_EXTRACT){0};
    /* Aspect-aware grid with at most 65536 cells; center sampling avoids
     * repeatedly selecting the same image column with a flattened stride. */
    unsigned nx = (unsigned)fmin(NA_SAMPLES, fmin(width, fmax(1, floor(sqrt((double)NA_SAMPLES * width / height)))));
    unsigned ny = (unsigned)fmin(height, NA_SAMPLES / nx);
    for (unsigned y = 0; y < ny; y++) {
        size_t py = ((uint64_t)(2*y+1)*height) / (2*ny);
        for (unsigned x = 0; x < nx; x++) {
            size_t px = ((uint64_t)(2*x+1)*width) / (2*nx);
            const uint8_t *p = pixels + (py * width + px)*3;
            unsigned idx = ((unsigned)(p[0]>>3)<<10) | ((unsigned)(p[1]>>3)<<5) | (p[2]>>3);
            bins[idx].r += p[0]; bins[idx].g += p[1]; bins[idx].b += p[2]; bins[idx].n++;
        }
    }
    unsigned n = 0, largest = 0;
    for (unsigned i = 0; i < 32768; i++) if (bins[i].n) {
        BIN b = bins[i];
        RGB rgb = {(b.r+b.n/2)/b.n, (b.g+b.n/2)/b.n, (b.b+b.n/2)/b.n};
        points[n] = (NA_CLUSTER){na_from_rgb(rgb), b.n};
        if (b.n > points[largest].weight) largest = n;
        out->samples += b.n;
        n++;
    }
    free(bins);
    out->count = n < NA_CLUSTERS ? n : NA_CLUSTERS;
    out->colors[0].lab = points[largest].lab;
    for (unsigned k = 1; k < out->count; k++) {
        double best = -1; unsigned chosen = 0;
        for (unsigned i = 0; i < n; i++) {
            double d = 4;
            for (unsigned j = 0; j < k; j++) d = fmin(d, na_distance(points[i].lab, out->colors[j].lab));
            if (d > best) { best = d; chosen = i; }
        }
        out->colors[k].lab = points[chosen].lab;
    }
    for (int iteration = 0; iteration < NA_ITERATIONS; iteration++) {
        NA_LAB sums[NA_CLUSTERS] = {{0}};
        unsigned weights[NA_CLUSTERS] = {0};
        for (unsigned i = 0; i < n; i++) {
            unsigned closest = 0; double best = 4;
            for (unsigned k = 0; k < out->count; k++) {
                double d = na_distance(points[i].lab, out->colors[k].lab);
                if (d < best) { best = d; closest = k; }
            }
            double w = points[i].weight;
            sums[closest].L += points[i].lab.L*w;
            sums[closest].a += points[i].lab.a*w;
            sums[closest].b += points[i].lab.b*w;
            weights[closest] += points[i].weight;
        }
        double movement = 0;
        for (unsigned k = 0; k < out->count; k++) {
            out->colors[k].weight = weights[k];
            if (!weights[k]) continue;
            NA_LAB center = {sums[k].L/weights[k], sums[k].a/weights[k], sums[k].b/weights[k]};
            movement += na_distance(center, out->colors[k].lab);
            out->colors[k].lab = center;
        }
        if (movement < 1e-12) break;
    }
    /* Drop empty clusters, keeping deterministic order. */
    unsigned used = 0;
    for (unsigned k = 0; k < out->count; k++)
        if (out->colors[k].weight) out->colors[used++] = out->colors[k];
    out->count = used;
    free(points);
    return 1;
}

/* Population-aware perceptual separation; additional slots are tonal variants
 * of available hues, never synthetic hue rotations. */
static void na_representatives(const NA_EXTRACT *ex, NA_LAB colors[6], NA_LAB *background, NA_LAB *accent, int dominant, int sort)
{
    unsigned largest = 0, chosen[6] = {0};
    *background = (NA_LAB){0}; *accent = (NA_LAB){.5,0,0};
    double best_accent = 0;
    for (unsigned i = 0; i < ex->count; i++) {
        NA_CLUSTER c = ex->colors[i];
        double fraction = (double)c.weight/ex->samples;
        if (c.weight > ex->colors[largest].weight) largest = i;
        background->L += c.lab.L*fraction;
        background->a += c.lab.a*fraction;
        background->b += c.lab.b*fraction;
        double score = sqrt(fraction)*fmin(na_chroma(c.lab), .14);
        if (fraction >= .005 && na_chroma(c.lab) >= .025 && score > best_accent) {
            best_accent = score; *accent = c.lab;
        }
    }
    if (dominant) *background = ex->colors[largest].lab;
    for (unsigned k = 0; k < 6; k++) {
        double best = -1; unsigned candidate = largest;
        if (k < ex->count) {
            for (unsigned i = 0; i < ex->count; i++) {
                int used = 0; double d = 1;
                for (unsigned j = 0; j < k; j++) {
                    if (chosen[j] == i) used = 1;
                    d = fmin(d, na_distance(ex->colors[i].lab, colors[j]));
                }
                double score = sqrt((double)ex->colors[i].weight/ex->samples) * sqrt(d);
                if (!used && score > best) { candidate = i; best = score; }
            }
            chosen[k] = candidate;
            colors[k] = ex->colors[candidate].lab;
        } else {
            colors[k] = colors[k % ex->count];
            colors[k].L = na_clamp(colors[k].L + .035*(k+1), .1, .9);
            colors[k].a *= .9; colors[k].b *= .9;
        }
    }
    if (sort) for (int i = 1; i < 6; i++) {
        NA_LAB c = colors[i]; int j = i;
        while (j > 0 && colors[j-1].L > c.L) { colors[j] = colors[j-1]; j--; }
        colors[j] = c;
    }
}
#endif
