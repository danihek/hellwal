/*  hell_colors - v0.0.1 - MIT LICENSE
 *
 *  This is header only stb style INSPIRED library ;
 *  It was created for my needs, use at your own risk.
 *
 * It contains couple of useful functions
 * to help with manual colors manipulation
 *
 *  --------------------------------------
 *
 * To start add this to you code and enjoy
 *
 *  #define HELL_COLORS_IMPLEMENTATION
 *  #include "hell_colors.h"
 *
*/

#ifndef HELL_COLORS_H
#define HELL_COLORS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Version information */
#define HELL_COLORS_VERSION "0.0.1"

/* Configurable Options */
#ifndef HELL_COLORS_DEF
#define HELL_COLORS_DEF static inline
#else
#define HELL_COLORS_DEF extern
#endif

#include <math.h>
#include <stdio.h>
#include <stdint.h>

/* 
 * Structures
 */

/* RGB, what can I say huh
 * it speaks for itself */
typedef struct
{
    uint8_t R; // Red   [0, 255]
    uint8_t G; // Green [0, 255]
    uint8_t B; // Blue  [0, 255]
} RGB;

typedef struct {
    float H; // Hue        [0, 360]
    float S; // Saturation [0, 1]
    float L; // Luminance  [0, 1]
} HSL;

/* 
 * Function declarations
 */

HELL_COLORS_DEF void print_rgb(RGB col);

HELL_COLORS_DEF float calculate_luminance(RGB c);
HELL_COLORS_DEF float calculate_color_distance(RGB a, RGB b);
HELL_COLORS_DEF float wcag_luminance_channel(uint8_t value);
HELL_COLORS_DEF float wcag_calculate_luminance(RGB c);

HELL_COLORS_DEF uint8_t clamp_uint8(int value);
HELL_COLORS_DEF int compare_luminance(RGB a, RGB b);

HELL_COLORS_DEF float calculate_contrast_ratio(RGB a, RGB b);
HELL_COLORS_DEF uint8_t meets_min_text_contrast(RGB a, RGB b);

HELL_COLORS_DEF HSL rgb_to_hsl(RGB color);
HELL_COLORS_DEF RGB hsl_to_rgb(HSL hsl);
HELL_COLORS_DEF RGB clamp_rgb(RGB color);
HELL_COLORS_DEF RGB darken_color(RGB color, float factor);
HELL_COLORS_DEF RGB lighten_color(RGB color, float factor);
HELL_COLORS_DEF RGB saturate_color(RGB color, float factor);
HELL_COLORS_DEF RGB adjust_luminance(RGB color, float factor);
HELL_COLORS_DEF RGB blend_colors(const RGB color1, const RGB color2, float blend_factor);
HELL_COLORS_DEF RGB blend_with_brightness(RGB bright_color, RGB mix_color, float mix_ratio);

#ifdef HELL_COLORS_IMPLEMENTATION
/* Function definitions */

/* Writes color as block to stdout - no new lines by itself */
HELL_COLORS_DEF void print_rgb(RGB col)
{
    /* Write color from as colored block */
    fprintf(stdout, "\x1b[48;2;%d;%d;%dm \033[0m", col.R, col.G, col.B);
    fprintf(stdout, "\x1b[48;2;%d;%d;%dm \033[0m", col.R, col.G, col.B);
}

/* calculate how bright is color from RGB
 * https://en.wikipedia.org/wiki/Relative_luminance
 */
HELL_COLORS_DEF float calculate_luminance(RGB c)
{
    return (0.2126 * c.R + 0.7152 * c.G + 0.0722 * c.B);
}

/* Converts an 8bit (0-255) rgb channel value to a format that can be used
 * to calculate the relative luminance of the color, according to the
 * WCAG's definition.
 *
 * See: https://www.w3.org/WAI/GL/wiki/Relative_luminance
 */
HELL_COLORS_DEF float wcag_luminance_channel(uint8_t value)
{
    const float fraction = value / 255.0f;
    return (fraction <= 0.03928f) ? fraction / 12.92f
                                  : powf((fraction + 0.055f) / 1.055f, 2.4f);
}

/* Calculate the WCAG's definition of relative luminance for a color.
 * https://www.w3.org/WAI/GL/wiki/Relative_luminance
 */
HELL_COLORS_DEF float wcag_calculate_luminance(RGB c)
{
    const float r = wcag_luminance_channel(c.R);
    const float g = wcag_luminance_channel(c.G);
    const float b = wcag_luminance_channel(c.B);

    return (0.2126f * r) + (0.7152f * g) + (0.0722f * b);
}

/* compare luminance of two RGB colors
 * returns  1 if a > b
 * returns -1 if a < b
 * returns 0 if a == b
 */
HELL_COLORS_DEF int compare_luminance(RGB a, RGB b)
{
    float lum_a = calculate_luminance(a);
    float lum_b = calculate_luminance(b);

    if (lum_a < lum_b)
        return -1;
    else if (lum_a > lum_b)
        return 1;
    else
        return 0;
}

/* Calculates the contrast ratio between two colors.
 *
 * See WCAG standards:
 * - [Technique G18](https://www.w3.org/WAI/WCAG21/Techniques/general/G18)
 */
HELL_COLORS_DEF float calculate_contrast_ratio(RGB a, RGB b)
{
    const float lum_a = wcag_calculate_luminance(a);
    const float lum_b = wcag_calculate_luminance(b);

    if (lum_a == lum_b)
        return 1.0f; /* No need to do more calculations */

    float lighter;
    float darker;

    if (lum_a > lum_b)
    {
        lighter = lum_a;
        darker = lum_b;
    }
    else
    {
        lighter = lum_b;
        darker = lum_a;
    }

    return (lighter + 0.05) / (darker + 0.05);
}

/* Check if the contrast ratio between a color and the background satisfies the minimum
 * contrast standard for meaningful text (such as body text).
 *
 * See WCAG standards:
 * - [1.4.3 Contrast (Minimum)](https://www.w3.org/WAI/WCAG21/Understanding/contrast-minimum)
 *
 * returns 1 if the contrast ratio is at least 4.5:1
 * returns 0 if the contrast ratio is less than 4.5:1
 */
HELL_COLORS_DEF uint8_t meets_min_text_contrast(RGB a, RGB b)
{
    return calculate_contrast_ratio(a, b) >= 4.5;
}

/* check Euclidean distance between two colors to ensure diversity */
HELL_COLORS_DEF float calculate_color_distance(RGB a, RGB b)
{
    return sqrtf(powf(a.R - b.R, 2) + powf(a.G - b.G, 2) + powf(a.B - b.B, 2));
}

/* Convert RGB to HSL */
HELL_COLORS_DEF HSL rgb_to_hsl(RGB color)
{
    float r = color.R / 255.0f;
    float g = color.G / 255.0f;
    float b = color.B / 255.0f;
    
    float max_val = fmaxf(r, fmaxf(g, b));
    float min_val = fminf(r, fminf(g, b));
    float delta = max_val - min_val;

    HSL hsl;
    hsl.L = max_val;

    if (max_val != 0)
    {
        hsl.S = delta / max_val; // Saturation
    }
    else
    {
        hsl.S = 0;
        hsl.H = -1;
        return hsl;
    }

    if (delta == 0)
    {
        hsl.H = 0;
    }
    else
    {
        if (r == max_val)
        {
            hsl.H = (g - b) / delta;
        }
        else if (g == max_val)
        {
            hsl.H = 2 + (b - r) / delta;
        }
        else
        {
            hsl.H = 4 + (r - g) / delta;
        }

        hsl.H *= 60;

        if (hsl.H < 0)
        {
            hsl.H += 360;
        }
    }
    return hsl;
}

/* Convert HSL to RGB */
HELL_COLORS_DEF RGB hsl_to_rgb(HSL hsl)
{
    float C = (1 - fabsf(2 * hsl.L - 1)) * hsl.S;
    float X = C * (1 - fabsf(fmodf(hsl.H / 60.0f, 2) - 1));
    float m = hsl.L - C / 2;

    float r = 0, g = 0, b = 0;

    if (hsl.H >= 0 && hsl.H < 60) {
        r = C, g = X, b = 0;
    } else if (hsl.H >= 60 && hsl.H < 120) {
        r = X, g = C, b = 0;
    } else if (hsl.H >= 120 && hsl.H < 180) {
        r = 0, g = C, b = X;
    } else if (hsl.H >= 180 && hsl.H < 240) {
        r = 0, g = X, b = C;
    } else if (hsl.H >= 240 && hsl.H < 300) {
        r = X, g = 0, b = C;
    } else if (hsl.H >= 300 && hsl.H < 360) {
        r = C, g = 0, b = X;
    }

    RGB rgb;
    rgb.R = (uint8_t)((r + m) * 255);
    rgb.G = (uint8_t)((g + m) * 255);
    rgb.B = (uint8_t)((b + m) * 255);

    return rgb;
}

/* avoid exceeding max value */
HELL_COLORS_DEF uint8_t clamp_uint8(int value)
{
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (uint8_t)value;
}

/* avoid exceeding max value for each */
HELL_COLORS_DEF RGB clamp_rgb(RGB color)
{
    return (RGB)
    {
        .R = clamp_uint8(color.R),
        .G = clamp_uint8(color.G),
        .B = clamp_uint8(color.B)
    };
}

/* adjust luminance of a color */
HELL_COLORS_DEF RGB adjust_luminance(RGB color, float factor)
{
    return (RGB){
        .R = (uint8_t)fminf(color.R * factor, 255),
        .G = (uint8_t)fminf(color.G * factor, 255),
        .B = (uint8_t)fminf(color.B * factor, 255)
    };
}

/* lighten a color by a factor */
HELL_COLORS_DEF RGB lighten_color(RGB color, float factor)
{
    return adjust_luminance(color, 1.0f + factor);
}

/* darken a color by a factor */
HELL_COLORS_DEF RGB darken_color(RGB color, float factor)
{
    return adjust_luminance(color, 1.0f - factor);
}

/* saturate a color, based on a factor */
HELL_COLORS_DEF RGB saturate_color(RGB color, float factor)
{
    float max_val = fmaxf(color.R, fmaxf(color.G, color.B));
    color.R = (uint8_t)(color.R + (max_val - color.R) * factor);
    color.G = (uint8_t)(color.G + (max_val - color.G) * factor);
    color.B = (uint8_t)(color.B + (max_val - color.B) * factor);
    return color;
}

/* blend two colors together based on a blend factor */
HELL_COLORS_DEF RGB blend_colors(RGB c1, RGB c2, float weight) {
    weight = (weight < 0.0f) ? 0.0f : (weight > 1.0f) ? 1.0f : weight; // Clamp weight
    return clamp_rgb((RGB){
        .R = (int)(c1.R * (1 - weight) + c2.R * weight),
        .G = (int)(c1.G * (1 - weight) + c2.G * weight),
        .B = (int)(c1.B * (1 - weight) + c2.B * weight)
    });
}

/* 
 * blend two colors together based on a blend factor,
 * bright mode - leave little color accent on white surface
 */
HELL_COLORS_DEF RGB blend_with_brightness(RGB bright_color, RGB mix_color, float mix_ratio)
{
    if (mix_ratio < 0.0f) mix_ratio = 0.0f;
    if (mix_ratio > 1.0f) mix_ratio = 1.0f;

    RGB blended;
    blended.R = bright_color.R + mix_ratio * (mix_color.R - bright_color.R);
    blended.G = bright_color.G + mix_ratio * (mix_color.G - bright_color.G);
    blended.B = bright_color.B + mix_ratio * (mix_color.B - bright_color.B);

    uint8_t max_channel = blended.R > blended.G ? 
                          (blended.R > blended.B ? blended.R : blended.B) : 
                          (blended.G > blended.B ? blended.G : blended.B);

    if (max_channel > 0 && max_channel < 255)
    {
        float adjustment = 255.0f / (float)max_channel;
        blended.R = (uint8_t)(blended.R * adjustment);
        blended.G = (uint8_t)(blended.G * adjustment);
        blended.B = (uint8_t)(blended.B * adjustment);
    }

    return blended;
}

#endif /* HELL_COLORS_IMPLEMENTATION */

#ifdef __cplusplus
}
#endif

#endif /* HELL_COLORS_H */
