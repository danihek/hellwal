/*  hellwal - MIT LICENSE
 *
 * changelog v1.0.4:
 *  - Addition to Homebrew thanks to @vaygr https://github.com/Homebrew/homebrew-core/pull/222859
 *
 * changelog v1.0.4:
 *  - added --version flag
 *  - added --skip-luminance-sort flag, it makes palette more similar to pywal, and less predictable. Some people may want that.
 *  - changed visuals of color blocks - they are 3 spaces wide, it's more visible now
 *  - added --preview and --preview-small cmdline argument
 *  - added alpha variable, you can set alpha by providing it right after keyword in templates (e.g. "%% color1.hex alpha=0.5%%") - requested in issues by @chinh4thepro
 *  - added --skip-term-colors - it skips setting colors(printing escape codes) to the terminals - requested in issues by @SherLock707
 *  - added new templates for zathuram mako, dwl and fuzzel thanks to @amolinae06
 *  - separeted color related functions from hellwal.c into its own header only library
 *
 *   I forgot to acknowledge v1.0.3:
 *  - thanks for fixing makefile and ivaild free usage: @MalcolmReed-ent
 *  - thanks for rofi template @Nashluffy
 *
 * changelog v1.0.3:
 *  - added --json (-j) mode - it prints colors in stdout in json format, README
 *  - fixed --help, I forgot to add some --cmdline args
 *  - new NEON MODE
 *  - added hellwm template
 *  - added fish template
 *  - changed: README typo
 *  - changed: Makefile - Fixed the order of compiler flags
 *  - changed: hellwal.c - Removed problematic free(pal) line
 *
 * changelog v1.0.2:
 *  - changed ~~arc4random()~~ to rand(), because it's not available on all platforms
 *  - changed rand() to ~~arc4random()~~
 *  - created palettes caching
 *  - added '--no-cache' cmd line argument
 *  - fixed applying addtional arguments for themes
 *
 * changelog v1.0.1:
 *  - fixed loading incorrect number of channels from image, this casued unmatched palette
 *  - added proper support for light mode
 *  - added --debug cmd line argument for more verbose output
 *  - added --dark-offset and --bright-offset cmd line arguments
 *  - added --inverted cmd line argument, it inverts color palette colors
 *  - added --color -c argument. Now we have 3 modes: dark, light and color
 *  - added --gray-scale argument to manipulate 'grayness' of color palette
 *  - added --static-background arg to set static background color
 *  - added --static-foreground arg to set static foreground color
 *
 * changelog v1.0.0:
 *  - first decent verision to release.
 *  - combined couple of ways into one for generating colorpalette
 *  - added bins to obtains more precise colors
 *  - fixed: half of palette is no more white randomly
 *  - get rid of unecessary functions
 *
 * ----------------------------------------------------------------------------------------------------
 *
 *  changelog v0.0.7:
 *  - improved colors, gen_palette() function is re-designed (again)
 *  - changed wallpaper gen_palette algorithm to median cut
 *  - fixed some templates
 *
 *  changelog v0.0.6:
 *  - improved colors, gen_palette() function is re-designed
 *  - added in ./assests example script how you can use hellwal
 *  - removed colors.sh
 *
 *  changelog v0.0.5:
 *  - fixed .hex, .rgb while using keyword like (background, foreground etc.)
 *  - themes adjustments
 *  - added rgb template
 *  - changed directories errors to warn, to allow to use same variable as path
 *
 *  changelog v0.0.4:
 *   - combined logging palletes into one
 *   - support for --random (pics random image or template from specified directory)
 *   - support for executing scripts after running hellwal
 *   - added terminal template, adjusted colors.sh
 *   - better --help
 *
 *  changelog v0.0.3:
 *   - removed TEMPLATE_ARG, OUTPUT_NAME_ARG - I've come to conclusion that it's useless.
 *   - changed how 'variables' are parsed. Now index of colors ex. "color|0|.hex" is depracated. Use "color0.hex" instead.
 *   - added support for static themes, for example you can add gruvbox, catppuccin etc.
 *   - added THEME_ARG, THEME_FOLDER_ARG
 *   - added keyword "wallpaper" for wallpaper path.
 *   - added example themes in ./themes folder
 *
 *  changelog v0.0.2:
 *   - actually working and better generated colorschemes
 *   - silent output: -q or --quiet
 *   - variable |w|, that recognized in templates returns IMAGE_ARG path
 *   - support for both light/dark mode
 *   - added more example templates
 */

#include <math.h>
#include <time.h>
#include <glob.h>
#include <fcntl.h>
#include <stdio.h>
#include <libgen.h>
#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define HELL_PARSER_IMPLEMENTATION
#include "hell_parser.h"

#define HELL_COLORS_IMPLEMENTATION
#include "hell_colors.h"

/***
 * MACROS
 ***/

/* just palette size */
#define PALETTE_SIZE 16
#define BINS 8

/* set default value for global char* variables */
#define SET_DEF(x, s) \
    if (x == NULL) \
        x = home_full_path(s); \
    else \
        x = home_full_path(x);

/* if verision is somehow not defined,
 * set verision to 69 */
#ifndef VERSION
#define VERSION "6.9.6"
#endif

/***
 * STRUCTURES
 ***/

/* IMG
 * 
 * image structure that contains all image data,
 * we need to create color palette
 */
typedef struct
{
    /* With 3 channels it goes from 0 -> size:
     * pixels[0] = Red;
     * pixels[1] = Green;
     * pixels[2] = Blue;
     * pixels[3] = Red;
     * .. And so on
     *
     * Also, we are using uint8_t instead of RGB structure
     * here, only because I dont know how to do it.
     */
    uint8_t *pixels; 
    size_t size; /* Size is width * height * channels(3) */

    unsigned width;
    unsigned height;
} IMG;

/* PALETTE
 *
 * stores all RGB colors */
typedef struct
{
    RGB colors[PALETTE_SIZE];
} PALETTE;

/* TEMPLATE
 *
 * helps read and write and store processed buffer*/
typedef struct
{
    char *name;
    char *path;
    char *content;
} TEMPLATE;

/* COLOR_TYPES - helps to manage colors within the code */
enum COLOR_TYPES { HEX_t, RGB_t, R_t, G_t, B_t };

/***
 * GLOBAL VARIABLES
 ***/

/* these are being set in set_args() */
char HELLWAL_DELIM = '%';
char HELLWAL_DELIM_COUNT = 2;

/* image path, which will be used to create PALETTE */
char *IMAGE_ARG = NULL;

/* quiet arg, if NULL you will get verbose output,
 * otherwise its going to print everthing normally 
 */
char *QUIET_ARG = NULL;

/* if used, it will skip
 * setting colors to terminals */
char *SKIP_TERM_COLORS = NULL;

/* if used, colors will not be sorted by luminance,
 * aka brightness, so colors are harder to control,
 * but some people may want it */
char *SKIP_LUMINANCE_SORTING_ARG = NULL;

/* darkmode, lightmode and colormode - darkmode is default */
char *DARK_ARG  = NULL;
char *LIGHT_ARG = NULL;
char *COLOR_ARG = NULL;

/* folder that contains templates */
char *TEMPLATE_FOLDER_ARG = NULL;

/* 
 * output folder for generated templates,
 * default one is in ~/.cache/hellwal/
 */
char *OUTPUT_ARG = NULL;

/* ouputs json formatted colors to stdout,
 * without using templates.
 * https://www.reddit.com/r/unixporn/comments/1h9aawb/comment/m12o2og/
 */
char *JSON_ARG = NULL;

/* name of theme in THEME_FOLDER_ARG,
 * or (in case was not found) path to file */
char *THEME_ARG = NULL;

/* folder that contains static themes */
char *THEME_FOLDER_ARG  = NULL;

/* pick random THING (image or theme) of provided folder path:
 *   - provide path to IMG using *IMAGE_ARG
 *   - or *THEME_FOLDER_ARG
 */
char *RANDOM_ARG = NULL;

/* enables more verbose output, to see what's going on */
char *DEBUG_ARG = NULL;

/* run script after successfull hellwal job */
char *SCRIPT_ARG = NULL;

/* do not cache palette, do not read cached palettes */
char *NO_CACHE_ARG = NULL;

/* invert color palette colors */
char *INVERT_ARG = NULL;

/* neon-like color palette colors */
char *NEON_MODE_ARG = NULL;

/* Set Static Background colors */
RGB *STATIC_BG_ARG = NULL;

/* Set Static Foreground colors */
RGB *STATIC_FG_ARG = NULL;

/* make sure colors contrast well with the background */
char *CHECK_CONTRAST_ARG = NULL;

/* defines 'grayness' of colorpalette */
float GRAY_SCALE_ARG = -1;

/* defines offsets to manipulate darkness and brightness */
float BRIGHTNESS_OFFSET_ARG = -1;
float DARKNESS_OFFSET_ARG = -1;
float OFFSET_GLOBAL = 0;

/* default color template to save cached themes */
char *CACHE_TEMPLATE = "\
\\%\\%wallpaper = %% wallpaper %% \\%\\%\n\
\
\\%\\%background = #%% background %% \\%\\%\n\
\\%\\%foreground = #%% foreground %% \\%\\%\n\
\\%\\%cursor = #%% cursor %% \\%\\%\n\
\\%\\%border = #%% border %% \\%\\%\n\
\
\\%\\%color0 = #%% color0.hex %% \\%\\%\n\
\\%\\%color1 = #%% color1.hex %% \\%\\%\n\
\\%\\%color2 = #%% color2.hex %% \\%\\%\n\
\\%\\%color3 = #%% color3.hex %% \\%\\%\n\
\\%\\%color4 = #%% color4.hex %% \\%\\%\n\
\\%\\%color5 = #%% color5.hex %% \\%\\%\n\
\\%\\%color6 = #%% color6.hex %% \\%\\%\n\
\\%\\%color7 = #%% color7.hex %% \\%\\%\n\
\\%\\%color8 = #%% color8.hex %% \\%\\%\n\
\\%\\%color9 = #%% color9.hex %% \\%\\%\n\
\\%\\%color10 = #%% color10.hex %% \\%\\%\n\
\\%\\%color11 = #%% color11.hex %% \\%\\%\n\
\\%\\%color12 = #%% color12.hex %% \\%\\%\n\
\\%\\%color13 = #%% color13.hex %% \\%\\%\n\
\\%\\%color14 = #%% color14.hex %% \\%\\%\n\
\\%\\%color15 = #%% color15.hex %% \\%\\%\n";

/* default color template to save cached themes */
char *JSON_TEMPLATE = 
"{\n"
"  \"wallpaper\": \"%% wallpaper %%\",\n"
"  \"alpha\": \"100\",\n"
"  \"special\": {\n"
"    \"background\": \"#%% background %%\",\n"
"    \"foreground\": \"#%% foreground %%\",\n"
"    \"cursor\": \"#%% cursor %%\",\n"
"    \"border\": \"#%% border %%\"\n"
"  },\n"
"  \"colors\": {\n"
"    \"color0\": \"#%% color0.hex %%\",\n"
"    \"color1\": \"#%% color1.hex %%\",\n"
"    \"color2\": \"#%% color2.hex %%\",\n"
"    \"color3\": \"#%% color3.hex %%\",\n"
"    \"color4\": \"#%% color4.hex %%\",\n"
"    \"color5\": \"#%% color5.hex %%\",\n"
"    \"color6\": \"#%% color6.hex %%\",\n"
"    \"color7\": \"#%% color7.hex %%\",\n"
"    \"color8\": \"#%% color8.hex %%\",\n"
"    \"color9\": \"#%% color9.hex %%\",\n"
"    \"color10\": \"#%% color10.hex %%\",\n"
"    \"color11\": \"#%% color11.hex %%\",\n"
"    \"color12\": \"#%% color12.hex %%\",\n"
"    \"color13\": \"#%% color13.hex %%\",\n"
"    \"color14\": \"#%% color14.hex %%\",\n"
"    \"color15\": \"#%% color15.hex %%\"\n"
"  }\n"
"}\n";

/*** 
 * FUNCTIONS DECLARATIONS
 ***/

/* args */
int set_args(int argc, char *argv[]);

/* utils */
float clamp_float(float value, float min, float max);

int is_between_01_float(const char *str);
int _compare_luminance_qsort(const void *a, const void *b);

char *rand_file(char *path);
char *home_full_path(const char* path);

void check_output_dir(char *path);
void remove_whitespaces(char *str);
void run_script(const char *script);
void hellwal_usage(const char *name);
void remove_extra_whitespaces(char *str);

/* logging */
void eu(const char *format, ...);
void err(const char *format, ...);
void warn(const char *format, ...);
void log_c(const char *format, ...);

/* IMG */
IMG *img_load(char *filename);
void img_free(IMG *img);

/* color related stuff */
int hex_to_rgb(const char *hex, RGB *p);
int get_channel(RGB *colors, size_t start, size_t end, int channel);
int is_color_too_similar(RGB *palette, int num_colors, RGB new_color);

RGB apply_offsets(RGB c);
RGB apply_grayscale(RGB c);
RGB bin_to_color(int r_bin, int g_bin, int b_bin);
RGB average_color(IMG *img, size_t start, size_t end);

void print_color(RGB c);
void print_term_colors();
void print_term_colors_small();
void median_cut(RGB *colors, size_t *starts, size_t *ends, size_t *num_boxes, size_t target_boxes);

/* term, set for all active terminals ANSI escape codes */
void set_term_colors(PALETTE pal);

/* palettes */
PALETTE gen_palette(IMG *img);
PALETTE get_color_palette(PALETTE p);

int is_color_palette_var(char *name);
int check_cached_palette(char *filepath, PALETTE *p);
void palette_write_cache(char *filepath, PALETTE *p);
char *process_variable_alpha(char *color, char *value, enum COLOR_TYPES type);
char *palette_color(PALETTE pal, unsigned c, enum COLOR_TYPES type);
char *process_addtional_variables(char *color, char *right_token, enum COLOR_TYPES type);

void invert_palette(PALETTE *p);
void print_palette(PALETTE pal);
void reverse_palette(PALETTE *palette);
void palette_handle_neon_mode(PALETTE *p);
void palette_handle_dark_mode(PALETTE *p);
void palette_handle_light_mode(PALETTE *p);
void palette_handle_color_mode(PALETTE *p);
void apply_addtional_arguments(PALETTE *p);
void sort_palette_by_luminance(PALETTE *palette);
void check_palette_contrast(PALETTE *palette);

/* templates */
char *load_file(char *filename);
void process_templating(PALETTE pal);
size_t template_write(TEMPLATE *t, char *dir);
enum COLOR_TYPES parse_color_type(const char *str);
void process_template(TEMPLATE *t, PALETTE pal);
TEMPLATE **get_template_structure_dir(const char *dir_path, size_t *_size);

/* themes */
char *load_theme(char *themename);
PALETTE process_themeing(char *theme);
int process_theme(char *t, PALETTE *pal);

/*** 
 * FUNCTIONS DECLARATIONS
 ***/

/* prints usage to stdout */
void hellwal_usage(const char *name)
{
    printf("Usage:\n");
    printf("  %s -i <image> [OPTIONS]\n\n", name);
    printf("Options:\n");
    printf("  -i, --image <image>                Set image file\n");
    printf("  -d, --dark                         Set dark mode (default)\n");
    printf("  -l, --light                        Set light mode\n");
    printf("  -c, --color                        Enable colorized mode (experimental)\n");
    printf("  -v, --invert                       Invert colors in the palette\n");
    printf("  -m, --neon-mode                    Enhance colors for a neon effect\n");
    printf("  -r, --random                       Pick random image or theme\n");
    printf("  -q, --quiet                        Suppress output\n");
    printf("  -j, --json                         Prints colors to stdout in json format, it's skipping templates\n");
    printf("  -s, --script             <script>  Execute script after running hellwal\n");
    printf("  -f, --template-folder    <dir>     Set folder containing templates\n");
    printf("  -o, --output             <dir>     Set output folder for generated templates\n");
    printf("  -t, --theme              <file>    Set theme file or name\n");
    printf("  -k, --theme-folder       <dir>     Set folder containing themes\n");
    printf("  -g, --gray-scale         <value>   Apply grayscale filter   (0-1) (float)\n");
    printf("  -n, --dark-offset        <value>   Adjust darkness offset   (0-1) (float)\n");
    printf("  -b, --bright-offset      <value>   Adjust brightness offset (0-1) (float)\n");
    printf("  --check-contrast                   Ensure colors are readable against the background\n");
    printf("  --preview                          Preview current terminal colorscheme\n");
    printf("  --preview-small                    Preview current terminal colorscheme - small factor\n");
    printf("  --debug                            Enable debug mode\n");
    printf("  --version                          Print version and exit\n");
    printf("  --no-cache                         Disable caching\n");
    printf("  --skip-term-colors                 Skip setting colors to the terminal\n");
    printf("  --skip-luminance-sort              Skip sorting colors before applying\n");
    printf("  --static-background \"#hex\"         Set static background color\n");
    printf("  --static-foreground \"#hex\"         Set static foreground color\n");
    printf("  -h, --help                         Display this help and exit\n\n");
    printf("Defaults:\n");
    printf("  Template folder: ~/.config/hellwal/templates\n");
    printf("  Theme folder: ~/.config/hellwal/themes\n");
    printf("  Output folder: ~/.cache/hellwal/\n\n");
}

/* set given arguments */
int set_args(int argc, char *argv[])
{
    int j = 0; /* 'int i' from for loop counter to track incomplete option error */

    for (int i = 1; i < argc; i++)
    {
        j = i;

        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            hellwal_usage(argv[0]);
            exit(EXIT_SUCCESS);
        }
        else if ((strcmp(argv[i], "--image") == 0 || strcmp(argv[i], "-i") == 0))
        {
            if (i + 1 < argc)
                IMAGE_ARG = argv[++i];
            else {
                argc = -1;
            }
        }
        else if (strcmp(argv[i], "--version") == 0)
        {
            printf("%s\n",VERSION);
            exit(EXIT_SUCCESS);
        }
        else if ((strcmp(argv[i], "--dark") == 0 || strcmp(argv[i], "-d") == 0))
        {
            /* anything other than NULL, makes dark mode */
            DARK_ARG = "";
        }
        else if ((strcmp(argv[i], "--light") == 0 || strcmp(argv[i], "-l") == 0))
        {
            LIGHT_ARG = "";
        }
        else if ((strcmp(argv[i], "--color") == 0 || strcmp(argv[i], "-c") == 0))
        {
            COLOR_ARG = "";
        }
        else if ((strcmp(argv[i], "--invert") == 0 || strcmp(argv[i], "-v") == 0))
        {
            INVERT_ARG = "";
        }
        else if ((strcmp(argv[i], "--neon-mode") == 0 || strcmp(argv[i], "-m") == 0))
        {
            NEON_MODE_ARG = "";
        }
        else if (strcmp(argv[i], "--check-contrast") == 0)
        {
            CHECK_CONTRAST_ARG = "";
        }
        else if ((strcmp(argv[i], "--random") == 0 || strcmp(argv[i], "-r") == 0))
        {
            RANDOM_ARG = "";
        }
        else if ((strcmp(argv[i], "--json") == 0 || strcmp(argv[i], "-j") == 0))
        {
            JSON_ARG = "";
            QUIET_ARG = "";
        }
        else if ((strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0))
        {
            QUIET_ARG = "";
        }
        else if (strcmp(argv[i], "--skip-term-colors") == 0)
        {
            SKIP_TERM_COLORS = "";
        }
        else if (strcmp(argv[i], "--skip-luminance-sort") == 0)
        {
            SKIP_LUMINANCE_SORTING_ARG = "";
        }
        else if (strcmp(argv[i], "--debug") == 0)
        {
            DEBUG_ARG = "";
        }
        else if (strcmp(argv[i], "--preview") == 0)
        {
            print_term_colors();
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(argv[i], "--preview-small") == 0)
        {
            print_term_colors_small();
            exit(EXIT_SUCCESS);
        }
        else if (strcmp(argv[i], "--no-cache") == 0)
        {
            NO_CACHE_ARG = "";
        }
        else if ((strcmp(argv[i], "--template-folder") == 0 || strcmp(argv[i], "-f") == 0))
        {
            if (i + 1 < argc)
                TEMPLATE_FOLDER_ARG = argv[++i];
            else {
                argc = -1;
            }
        }
        else if ((strcmp(argv[i], "--output") == 0 || strcmp(argv[i], "-o") == 0))
        {
            if (i + 1 < argc)
                OUTPUT_ARG = argv[++i];
            else {
                argc = -1;
            }
        }
        else if ((strcmp(argv[i], "--theme") == 0 || strcmp(argv[i], "-t") == 0))
        {
            if (i + 1 < argc)
                THEME_ARG = argv[++i];
            else {
                argc = -1;
            }
        }
        else if ((strcmp(argv[i], "--theme-folder") == 0 || strcmp(argv[i], "-k") == 0))
        {
            if (i + 1 < argc)
                THEME_FOLDER_ARG = argv[++i];
            else {
                argc = -1;
            }
        }
        else if ((strcmp(argv[i], "--script") == 0 || strcmp(argv[i], "-s") == 0))
        {
            if (i + 1 < argc)
                SCRIPT_ARG = argv[++i];
            else
                argc = -1;
        }
        else if ((strcmp(argv[i], "--dark-offset") == 0 || strcmp(argv[i], "-n") == 0))
        {
            if (i + 1 < argc)
            {
                if (is_between_01_float(argv[++i]))
                    DARKNESS_OFFSET_ARG = strtod(argv[i], NULL);
                else
                    warn("Dark offset value have to be floating point number between 0-1!, skipping argument.");
            }
            else
                argc = -1;
        }
        else if ((strcmp(argv[i], "--bright-offset") == 0 || strcmp(argv[i], "-b") == 0))
        {
            if (i + 1 < argc)
            {
                if (is_between_01_float(argv[++i]))
                    BRIGHTNESS_OFFSET_ARG = strtod(argv[i], NULL);
                else
                    warn("Bright offset value have to be floating point number between 0-1!, skipping argument.");
            }
            else
                argc = -1;
        }
        else if ((strcmp(argv[i], "--gray-scale") == 0 || strcmp(argv[i], "-g") == 0))
        {
            if (i + 1 < argc)
            {
                if (is_between_01_float(argv[++i]))
                    GRAY_SCALE_ARG = strtod(argv[i], NULL);
                else
                    warn("Grayscale value have to be floating point number between 0-1!, skipping argument.");
            }
            else
                argc = -1;
        }
        else if (strcmp(argv[i], "--static-background") == 0)
        {
            if (i + 1 < argc)
            {
                STATIC_BG_ARG = calloc(1, sizeof(RGB));
                if (!hex_to_rgb(argv[++i], STATIC_BG_ARG))
                {
                    free(STATIC_BG_ARG);
                    err("Failed to parse static background: %s", argv[i-1]);
                }
            }
            else
            {
                argc = -1;
            }
        }
        else if (strcmp(argv[i], "--static-foreground") == 0)
        {
            if (i + 1 < argc)
            {
                STATIC_FG_ARG = calloc(1, sizeof(RGB));
                if (!hex_to_rgb(argv[++i], STATIC_FG_ARG))
                {
                    free(STATIC_FG_ARG);
                    err("Failed to parse static foreground: %s", argv[i-1]);
                }
            }
            else
            {
                argc = -1;
            }
        }
        else {
            eu("Unknown option: %s", argv[i]);
        }
    }

    if (argc == -1)
        err("Incomplete option: %s", argv[j]);

    /* handle needed arguments and warns - idk how to do this the other way */
    if (RANDOM_ARG != NULL && (THEME_FOLDER_ARG == NULL && IMAGE_ARG == NULL))
        err("you have to specify --image to provide image folder or --theme-folder to use RANDOM");

    if (IMAGE_ARG == NULL && THEME_ARG == NULL && ((THEME_FOLDER_ARG == NULL || TEMPLATE_FOLDER_ARG == NULL) && RANDOM_ARG == NULL))
        err("You have to provide image file or theme!:  --image,  --theme, \n\t");

    if ((THEME_ARG != NULL || THEME_FOLDER_ARG != NULL) && IMAGE_ARG != NULL)
    {
        if (THEME_FOLDER_ARG != NULL)
            err("you cannot use both --image and --theme-folder");
        else
            err("you cannot use both --image and --theme");
    }

    if (RANDOM_ARG != NULL && THEME_ARG != NULL)
        warn("specified theme is not used: \"%s\"", THEME_ARG);

    if (THEME_ARG == NULL && THEME_FOLDER_ARG != NULL && RANDOM_ARG == NULL)
        warn("specified theme folder is not used: \"%s\", you also have to provide --theme", THEME_FOLDER_ARG);

    /* if RANDOM, get file */
    if (RANDOM_ARG != NULL)
    {
        if (IMAGE_ARG != NULL)
            IMAGE_ARG = rand_file(IMAGE_ARG);
        else
            THEME_ARG = rand_file(THEME_FOLDER_ARG);
    }

    /* set offset values - you can provide both, but they will interfier with each other */
    if (DARKNESS_OFFSET_ARG != -1)
        OFFSET_GLOBAL -= DARKNESS_OFFSET_ARG;
    if (BRIGHTNESS_OFFSET_ARG != -1)
        OFFSET_GLOBAL += BRIGHTNESS_OFFSET_ARG;

    /* If not specified, set default ones */
    SET_DEF(OUTPUT_ARG, "~/.cache/hellwal/");
    SET_DEF(THEME_FOLDER_ARG , "~/.config/hellwal/themes");
    SET_DEF(TEMPLATE_FOLDER_ARG, "~/.config/hellwal/templates");

    return 0;
}

// prints current terminal colors
void print_term_colors()
{
    for (int i = 0; i < PALETTE_SIZE; i++)
    {
        // set foreground color
        printf("\033[38;5;%dm", i);
        printf(" FG %2d ", i);

        // reset and set background color
        printf("\033[0m");
        printf("\033[48;5;%dm", i);
        printf(" BG %2d ", i);

        // reset again
        printf("\033[0m\n");
    }
    printf("\n");

    print_term_colors_small();

    // reset at the end
    printf("\033[0m\n");
}

void print_term_colors_small()
{
    for (int i=0; i<PALETTE_SIZE; i++)
    {
        if (i == PALETTE_SIZE/2)
            printf("\n");

        //printf("\033[38;5;%dm", i); // if you want to set foreground color
        printf("\033[48;5;%dm", i);   // if you want to set background color
        printf("   ");                // two spaces as a "block" of a color
    }
    // reset at the end
    printf("\033[0m\n");
}

/* Writes color as block to stdout - it does not perform new line by itself */
void print_color(RGB col)
{
    if (QUIET_ARG != NULL)
        return;

    char *color_block = "   ";                  // color_block is 3 spaces wide
                                                // color_block + ↓↓↓↓↓↓
    /* Write color from as colored block */
    fprintf(stdout, "\x1b[48;2;%d;%d;%dm%s\033[0m", col.R, col.G, col.B, color_block);
}

/* 
 * prints to stderr formatted output and exits with EXIT_FAILURE,
 * also prints hellwal_usage()
 */
void eu(const char *format, ...)
{
    fprintf(stderr, "\033[91m[ERROR]: ");

    va_list ap;
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    fprintf(stderr, "\033[0m\n");

    hellwal_usage("hellwal");

    exit(EXIT_FAILURE);
}

/* prints to stderr formatted output and exits with EXIT_FAILURE */
void err(const char *format, ...)
{
    /*
     * ignores QUIET_ARG
     *
     if (QUIET_ARG != NULL)
     return;
     */

    va_list ap;
    va_start(ap, format);

    fprintf(stderr, "\033[91m[ERROR]: ");
    vfprintf(stderr, format, ap);
    fprintf(stderr, "\033[0m");

    va_end(ap);
    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}

/* prints to stderr formatted output, but not exits */
void warn(const char *format, ...)
{
    if (QUIET_ARG != NULL)
        return;

    va_list ap;
    va_start(ap, format);
    fprintf(stderr, "\033[93m[WARN]: ");

    vfprintf(stderr, format, ap);
    fprintf(stderr, "\033[0m");
    va_end(ap);
    fprintf(stderr, "\n");
}

/* prints formatted output to stdout with colors */
void log_c(const char *format, ...)
{
    if (QUIET_ARG != NULL)
        return;

    va_list ap;
    va_start(ap, format);

    const char *default_color = "\033[96m";

    fprintf(stdout, "%s[INFO]: ", default_color);

    vfprintf(stdout, format, ap);
    fprintf(stdout, "\033[0m");

    va_end(ap);
    fprintf(stdout, "\n");
}

/* get full path from '~/' or other relative paths */
char* home_full_path(const char* path)
{
    if (path[0] == '~')
    {
        const char* home = getenv("HOME");
        if (home)
        {
            char* full_path = malloc(strlen(home) + strlen(path) + 1);
            if (full_path)
            {
                sprintf(full_path, "%s%s", home, path + 1);
                return full_path;
            }
        }
    }
    return strdup(path);
}

/* 
 * checks if output directory exists,
 * if not creates it
 */
void check_output_dir(char *path)
{
    struct stat st;
    if (stat(path, &st) == -1)
        mkdir(path, 0700);
}

/* run script from given path */
void run_script(const char *script)
{
    if (script == NULL)
        return;

    log_c("Starting script: %s", script);
    size_t exit_code = system(script);
    if (exit_code == 0)
        log_c("Script \"%s\" exited with code: %d", script, exit_code);
    else
        err("Script \"%s\" exited with code: %d", script, exit_code);
}


/* avoid exceeding max value for float */
float clamp_float(float value, float min, float max)
{
    return value < min ? min : (value > max ? max : value);
}

/* get random file from given path */
char *rand_file(char *path)
{
    DIR *dir = opendir(path);
    if (dir == NULL)
        err("Cannot access directory %s: ", path);

    struct dirent *entry;
    char **files = NULL;
    size_t count = 0;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG)
        {
            files = realloc(files, sizeof(char *) * (count + 1));
            if (files == NULL)
            {
                perror("Memory allocation failed");
                closedir(dir);
                return NULL;
            }
            files[count++] = strdup(entry->d_name);
        }
    }
    closedir(dir);

    if (count == 0) {
        free(files);
        err("No files found in directory: %s\n", path);
    }

    srand((unsigned int)(time(NULL) ^ getpid()));
    size_t r_idx = rand() % count;
    char *choosen = calloc(1, strlen(path) + strlen(files[r_idx]) + 2);
    sprintf(choosen, "%s/%s", path, strdup(files[r_idx]));

    for (size_t i = 0; i < count; i++)
        free(files[i]);
    free(files);

    return choosen;
}

/* removes whitespaces from buffer */
void remove_whitespaces(char *str)
{
    if (!str) return;

    char *read = str;
    char *write = str;

    while (*read) {
        if (*read != ' ' && *read != '\t' && *read != '\n' && *read != '\r') {
            *write++ = *read;
        }
        read++;
    }
    *write = '\0';
}

/* make sure that buffer does not
 * start or ends with space, and
 * only occurs once between characters
 */
void remove_extra_whitespaces(char *str)
{
    int read = 0, write = 0;
    int space_found = 0;
    
    while (str[read] == ' ' || str[read] == '\t' || str[read] == '\n' || str[read] == '\r') read++;
    
    while (str[read])
    {
        if (str[read] == ' ' || str[read] == '\t' || str[read] == '\n' || str[read] == '\r')
        {
            if (!space_found)
            {
                str[write++] = ' ';
                space_found = 1;
            }
        }
        else
        {
            str[write++] = str[read];
            space_found = 0;
        }
        read++;
    }
    
    if (write > 0 && str[write - 1] == ' ')
    {
        write--;
    }
    
    str[write] = '\0';
}

int is_between_01_float(const char *str)
{
    char *end;
    double number = strtod(str, &end);

    if (*end != '\0')
        return 0;

    return (number >= 0.0 && number <= 1.0);
}

int _compare_luminance_qsort(const void *a, const void *b)
{
    RGB *color_a = (RGB *)a;
    RGB *color_b = (RGB *)b;

    if (color_a == NULL || color_b == NULL) return 1;

    return compare_luminance(*color_a, *color_b);
}

/* sort palette by luminance to spread out colors */
void sort_palette_by_luminance(PALETTE *palette)
{
    if (SKIP_LUMINANCE_SORTING_ARG != NULL) return;

    qsort(palette->colors, PALETTE_SIZE/2, sizeof(RGB), _compare_luminance_qsort);
}

/* function to reverse the palette, used when light mode is specified */
void reverse_palette(PALETTE *palette)
{
    for (int i = 0; i < PALETTE_SIZE / 2; i++)
    {
        RGB temp = palette->colors[i];

        palette->colors[i] = palette->colors[PALETTE_SIZE - 1 - i];
        palette->colors[PALETTE_SIZE - 1 - i] = temp;
    }
}

/* Get channel */
int get_channel(RGB *colors, size_t start, size_t end, int channel)
{
    uint8_t min = 255, max = 0;
    for (size_t i = start; i < end; i++)
    {
        uint8_t value = ((uint8_t *)&colors[i])[channel];
        if (value < min) min = value;
        if (value > max) max = value;
    }
    return max - min;
}

/* ensure that new color is not too similar to existing colors in the palette */
int is_color_too_similar(RGB *palette, int num_colors, RGB new_color)
{
    for (int i = 0; i < num_colors; i++)
    {
        if (calculate_color_distance(palette[i], new_color) < 35)
            return 1;  // Color is too similar
    }
    return 0;
}

/* calculate the average color of a given pixel range in an image */
RGB average_color(IMG *img, size_t start, size_t end) 
{
    long sum_r = 0, sum_g = 0, sum_b = 0;
    size_t count = end - start;
    if (count <= 0) count++;

    for (size_t i = start; i < end; i++)
    {
        sum_r += img->pixels[i * 3];
        sum_g += img->pixels[i * 3 + 1];
        sum_b += img->pixels[i * 3 + 2];
    }

    return clamp_rgb(
    (RGB)
    {
        .R = (int)(sum_r / count),
        .G = (int)(sum_g / count),
        .B = (int)(sum_b / count)
    });
}

/* convert bin indices to an rgb color */
RGB bin_to_color(int r_bin, int g_bin, int b_bin)
{
    return clamp_rgb(
    (RGB)
    {
        .R = r_bin * (256 / BINS),
        .G = g_bin * (256 / BINS),
        .B = b_bin * (256 / BINS)
    });
}

void invert_palette(PALETTE *p)
{
    if (p == NULL)
        return;

    for (int i = 0; i < PALETTE_SIZE; i++)
    {
        p->colors[i].R = 255 - p->colors[i].R;
        p->colors[i].G = 255 - p->colors[i].G;
        p->colors[i].B = 255 - p->colors[i].B;
    }
}

RGB apply_grayscale(RGB c)
{
    return saturate_color(c, GRAY_SCALE_ARG);
}

/* if user provided OFFSET value, apply it to color */
RGB apply_offsets(RGB c)
{
    if (OFFSET_GLOBAL == 0)
        return c;

    if (OFFSET_GLOBAL < 0)
        c = darken_color(c, -1 * OFFSET_GLOBAL);
    else if (OFFSET_GLOBAL > 0)
        c = lighten_color(c, 0.25f + OFFSET_GLOBAL);

    return c;
}

/* part of median algo */
size_t partition_colors(RGB *colors, size_t start, size_t end, int channel, uint8_t pivot)
{
    size_t left = start, right = end - 1;
    while (left <= right)
    {
        while (((uint8_t *)&colors[left])[channel] <= pivot && left < end) left++;
        while (((uint8_t *)&colors[right])[channel] > pivot && right > start) right--;
        if (left < right)
        {
            RGB temp = colors[left];
            colors[left] = colors[right];
            colors[right] = temp;
        }
    }
    return left;
}

/* perform median cut to partition the color space */
void median_cut(RGB *colors, size_t *starts, size_t *ends, size_t *num_boxes, size_t target_boxes) 
{
    while (*num_boxes < target_boxes)
    {
        size_t largest_segment_index = 0;
        int largest_range = 0;

        for (size_t i = 0; i < *num_boxes; i++)
        {
            size_t start = starts[i];
            size_t end = ends[i];
            int range_r = get_channel(colors, start, end, 0);
            int range_g = get_channel(colors, start, end, 1);
            int range_b = get_channel(colors, start, end, 2);
            int max_range = fmax(range_r, fmax(range_g, range_b));
            if (max_range > largest_range) {
                largest_range = max_range;
                largest_segment_index = i;
            }
        }

        size_t start = starts[largest_segment_index];
        size_t end = ends[largest_segment_index];
        int channel = (largest_range == get_channel(colors, start, end, 0)) ? 0 :
                      (largest_range == get_channel(colors, start, end, 1)) ? 1 : 2;

        size_t mid = (end - start) / 2;
        uint8_t pivot = ((uint8_t *)&colors[start + mid])[channel];

        size_t median = partition_colors(colors, start, end, channel, pivot);

        // Update the segments
        starts[largest_segment_index] = start;
        ends[largest_segment_index] = median;
        starts[*num_boxes] = median;
        ends[*num_boxes] = end;
        (*num_boxes)++;
    }
}

/* Compares every color in the palette against the background and checks their
 * contrast ratio. If it is too low, increases the contrast by lightening or darkening
 * the colors until the contrast ratio is met.
 *
 * Based heavily on wallust's contrast checking implementation.
 */
void check_palette_contrast(PALETTE *palette)
{
    if (palette == NULL)
        return;

    RGB *const bg = &palette->colors[0];
    RGB *const fg = &palette->colors[PALETTE_SIZE - 1];

    uint8_t dark_bg = compare_luminance(*bg, *fg) == 1 ? 0 : 1;
    uint8_t changed_bg = 0;

    int i = 0;

    /* Ensure contrast between the foreground and background */
    if (STATIC_BG_ARG == NULL || STATIC_FG_ARG == NULL)
    {
        while (!meets_min_text_contrast(*fg, *bg) && i < 10)
        {
            if (STATIC_BG_ARG == NULL)
            {
                *bg = dark_bg ? darken_color(*bg, 0.15) : lighten_color(*bg, 0.15);
                changed_bg = 1;
            }

            if (STATIC_FG_ARG == NULL)
                *fg = dark_bg ? lighten_color(*fg, 0.15) : darken_color(*fg, 0.15);

            i++;
        }
    }

    /* Ensure contrast for all other colors */
    for (int j = 1; j < PALETTE_SIZE - 1; j++)
    {
        RGB *const color = &palette->colors[j];
        i = 0;

        while (!meets_min_text_contrast(*color, *bg) && i < 5)
        {
            if (STATIC_BG_ARG == NULL && !changed_bg)
            {
                *bg = dark_bg ? darken_color(*bg, 0.15) : lighten_color(*bg, 0.15);
                changed_bg = 1;
            }

            *color = dark_bg ? lighten_color(*color, 0.15) : darken_color(*color, 0.15);
            i++;
        }
    }
}

PALETTE get_color_palette(PALETTE p)
{
    if (THEME_ARG)
    {
        p = process_themeing(THEME_ARG); /* if true, program end's here */
    }
    else
    {
        if (!check_cached_palette(IMAGE_ARG, &p)) {
            IMG *img = img_load(IMAGE_ARG);
            p = gen_palette(img);
            palette_write_cache(IMAGE_ARG, &p);
            img_free(img);
        }
    }

    return p;
}

void apply_addtional_arguments(PALETTE *p)
{
    sort_palette_by_luminance(p);
    for (int i = PALETTE_SIZE / 2; i < PALETTE_SIZE; i++)
        p->colors[i] = lighten_color(p->colors[i - PALETTE_SIZE / 2], 0.25f);

    /* Handle dark/light or color mode */
    if (THEME_ARG == NULL &&
            (LIGHT_ARG == NULL && COLOR_ARG == NULL && DARK_ARG == NULL))
        DARK_ARG = "";
    
    if (NEON_MODE_ARG != NULL)
        palette_handle_neon_mode(p);
    if (DARK_ARG  != NULL)
        palette_handle_dark_mode(p);
    if (LIGHT_ARG != NULL)
        palette_handle_light_mode(p);
    if (COLOR_ARG != NULL)
        palette_handle_color_mode(p);

    /* invert palette, if INVERT_ARG */
    if (INVERT_ARG != NULL)
        invert_palette(p);

    if (OFFSET_GLOBAL != 0 || GRAY_SCALE_ARG != -1)
    {
        for (int i = 0; i < PALETTE_SIZE; i++)
        {
            /* apply offsets */
            if (OFFSET_GLOBAL != 0)
                p->colors[i] = apply_offsets(p->colors[i]);

            /* apply grayscale value */
            if (GRAY_SCALE_ARG != -1)
                p->colors[i] = apply_grayscale(p->colors[i]);
        }
    }

    if (STATIC_BG_ARG != NULL)
        p->colors[0] = *STATIC_BG_ARG;
    if (STATIC_FG_ARG != NULL)
        p->colors[PALETTE_SIZE-1] = *STATIC_FG_ARG;

    if (CHECK_CONTRAST_ARG != NULL)
        check_palette_contrast(p);
}

void palette_handle_color_mode(PALETTE *p)
{
    if (p == NULL)
        return;

    RGB temp_color_0  = darken_color(p->colors[0], 0.1);
    RGB temp_color_7  = lighten_color(p->colors[14], 0.2);
    RGB temp_color_14 = darken_color(p->colors[7], 0.3);
    RGB temp_color_15 = lighten_color(p->colors[15], 0.5);

    p->colors[0] = temp_color_7;
    p->colors[7] = temp_color_0;
    p->colors[14] = temp_color_15;
    p->colors[15] = temp_color_14;
}

void palette_handle_light_mode(PALETTE *p)
{
    if (p == NULL)
        return;

    RGB temp_color_0 =  blend_with_brightness(saturate_color(lighten_color(p->colors[15], 0.6), 0.6), p->colors[5], 0.2f);
    RGB temp_color_7 =  darken_color(p->colors[7], 0.5);
    RGB temp_color_13 = lighten_color(p->colors[13], 0.6);
    RGB temp_color_14 = darken_color(p->colors[14], 0.5);
    RGB temp_color_15 = lighten_color(p->colors[0], 0.4);

    reverse_palette(p);

    p->colors[0] = temp_color_0;
    p->colors[7] = temp_color_7;
    p->colors[13] = temp_color_13;
    p->colors[14] = temp_color_14;
    p->colors[15] = temp_color_15;
}

void palette_handle_dark_mode(PALETTE *p)
{
    if (p==NULL)
        return;

    p->colors[0] = darken_color(p->colors[0], 0.7f);    // BG
    p->colors[15] = lighten_color(p->colors[15], 0.5f); // FG
    p->colors[7] = lighten_color(p->colors[7], 0.4f);   // Term text
}

void palette_handle_neon_mode(PALETTE *p)
{
        if (DEBUG_ARG != NULL)
            log_c("Applying NEON MODE:\n\n");

        for (int i = 0; i < PALETTE_SIZE/2; i++)
        {
            if (i > 0)
            {
                HSL hsl = rgb_to_hsl(p->colors[i]);

                if (hsl.L <= 0.1f || hsl.L >= 0.9f)
                    continue;

                hsl.S = clamp_float(hsl.S * 1.25f, 0.6f, 1.0f);
                hsl.L = clamp_float(hsl.L * 1.15f, 0.3f, 0.9f);

                p->colors[i] = hsl_to_rgb(hsl);
            }

            if (DEBUG_ARG != NULL)
            {
                print_color(p->colors[i]);
                printf(" | (%d, %d, %d)\n", p->colors[i].R, p->colors[i].G, p->colors[i].B);
            }
        }
        if (DEBUG_ARG != NULL)
            log_c("\n");
}

PALETTE gen_palette(IMG *img)
{
    PALETTE palette;
    size_t total_pixels = img->size / 3;
    RGB *all_colors = (RGB *)img->pixels;
    int num_colors = 0;

    size_t starts[PALETTE_SIZE / 2] = {0};
    size_t ends[PALETTE_SIZE / 2] = {total_pixels};
    size_t num_boxes = 1;

    median_cut(all_colors, starts, ends, &num_boxes, PALETTE_SIZE / 2);

    int histogram[BINS][BINS][BINS] = {{{0}}};
    for (size_t i = 0; i < total_pixels; i++)
    {
        int r_bin = img->pixels[i * 3] / (256 / BINS);
        int g_bin = img->pixels[i * 3 + 1] / (256 / BINS);
        int b_bin = img->pixels[i * 3 + 2] / (256 / BINS);

        histogram[r_bin][g_bin][b_bin] = (histogram[r_bin][g_bin][b_bin] < INT_MAX) ? histogram[r_bin][g_bin][b_bin] + 1 : INT_MAX;
    }

    typedef struct {
        int count;
        int r_bin, g_bin, b_bin;
    } BinCount;

    BinCount top_bins[(PALETTE_SIZE / 2)] = {0};

    for (int r = 0; r < BINS; r++)
    {
        for (int g = 0; g < BINS; g++)
        {
            for (int b = 0; b < BINS; b++)
            {
                int count = histogram[r][g][b];
                if (count > top_bins[(PALETTE_SIZE / 2) - 1].count)
                {
                    top_bins[(PALETTE_SIZE / 2) - 1] = (BinCount){count, r, g, b};
                    for (int k = (PALETTE_SIZE / 2) - 1; k > 0 && top_bins[k].count > top_bins[k - 1].count; k--)
                    {
                        BinCount temp = top_bins[k];
                        top_bins[k] = top_bins[k - 1];
                        top_bins[k - 1] = temp;
                    }
                }
            }
        }
    }

    for (size_t i = 0; i < PALETTE_SIZE / 2; i++)
    {
        RGB avg_color = average_color(img, starts[i], ends[i]);
        RGB bin_color = bin_to_color(top_bins[i].r_bin, top_bins[i].g_bin, top_bins[i].b_bin);
        RGB blended_colors = blend_colors(avg_color, bin_color, 0.5f);

        if (DEBUG_ARG != NULL)
        {
            printf("(%d, %d, %d)", avg_color.R, avg_color.G, avg_color.B);
            printf(" - ");
            printf("(%d, %d, %d)", bin_color.R, bin_color.G, bin_color.B);
            printf(" = ");
            printf("(%d, %d, %d)", blended_colors.R, blended_colors.G, blended_colors.B);
            printf("\n");

            print_color(avg_color);
            printf(" - ");
            print_color(bin_color);
            printf(" = ");
            print_color(blended_colors);

            printf("\nLUM: %f", calculate_luminance(blended_colors));
            printf("\n\n");
        }

        palette.colors[num_colors++] = blended_colors;
    }

    size_t sample_points[] = {
        0,
        (img->height - 1) * img->width,
        (img->height - 1) * img->width + (img->width - 1),
        (img->height / 2) * img->width + (img->width / 2),
        (img->height / 4) * img->width + (img->width / 4),
        (img->height / 4) * img->width + 3 * (img->width / 4),
        3 * (img->height / 4) * img->width + (img->width / 4),
        3 * (img->height / 4) * img->width + 3 * (img->width / 4)
    };

    for (size_t j = 0; j < sizeof(sample_points) / sizeof(sample_points[0]) && num_colors < PALETTE_SIZE; j++)
    {
        size_t i = sample_points[j];
        if (i + 3 >= img->size) continue;

        RGB new_color = {img->pixels[i], img->pixels[i + 1], img->pixels[i + 2]};

        if (!is_color_too_similar(palette.colors, num_colors, new_color))
            new_color = palette.colors[num_colors++] = new_color;
        else
            new_color = blend_colors(new_color, palette.colors[j], 0.5);

        palette.colors[num_colors++] = new_color;
    }

    if (DEBUG_ARG != NULL)
        log_c("\n---\n");

    return palette;
}

/* Writes palete to stdout */
void print_palette(PALETTE pal)
{
    if (QUIET_ARG != NULL)
        return;

    for (size_t i=0; i<PALETTE_SIZE; i++)
    {
        print_color(pal.colors[i]);
        if (i+1 == PALETTE_SIZE/2) printf("\n");
    }
    printf("\n");
}

/* 
 * Write color palette to buffer, hex or rgb by setting up type
 */
char *palette_color(PALETTE pal, unsigned c, enum COLOR_TYPES type)
{
    if (c > PALETTE_SIZE - 1)
        return NULL;

    char *hex_fmt = "%02x%02x%02x";
    char *rgb_fmt = "%d, %d, %d";
    char *buffer = (char*)malloc(64);

    switch (type) {
    case HEX_t:
        sprintf(buffer, hex_fmt, pal.colors[c].R, pal.colors[c].G, pal.colors[c].B);
        break;
    case RGB_t:
        sprintf(buffer, rgb_fmt, pal.colors[c].R, pal.colors[c].G, pal.colors[c].B);
        break;
    case R_t:
        sprintf(buffer, "%d", pal.colors[c].R);
        break;
    case G_t:
        sprintf(buffer, "%d", pal.colors[c].G);
        break;
    case B_t:
        sprintf(buffer, "%d", pal.colors[c].B);
        break;
    }

    return buffer;
}

/* print gray-scale palettes */
void print_palette_gray_scale_iter(IMG *img, size_t iter)
{
    if (img == NULL) return;

    PALETTE pal;

    for (size_t i = 0; i < iter; i++)
    {
        GRAY_SCALE_ARG = (float)i == 0 ? 0 : (float)((float)i/iter);

        sort_palette_by_luminance(&pal);

        printf("GRAYSCALE VALUE: %f\n", GRAY_SCALE_ARG);
        print_palette(pal);
    }

    GRAY_SCALE_ARG = -1.f;
}

/* Load image file using stb, return IMG structure */
IMG *img_load(char *filename)
{
    if (IMAGE_ARG == NULL)
        err("No image provided");
    log_c("Loading image %s", IMAGE_ARG);

    int width, height;
    int numberOfChannels;
    int forcedNumberOfChannels = 3;

    uint8_t *imageData = stbi_load(filename, &width, &height, &numberOfChannels, forcedNumberOfChannels);

    if (imageData == 0) err("Error while loading the file: %s", filename);

    IMG *img = malloc(sizeof(IMG));

    img->size = width * height * forcedNumberOfChannels;
    img->pixels = imageData;

    log_c("Loaded!");
    return img;
}

/* free all allocated stuff in IMG */
void img_free(IMG *img)
{
    stbi_image_free(img->pixels);
    free(img);
}

/* 
 * overides default terminal color variables from PALETTE
 * This works thanks to this (and chatgpt):
 * - https://github.com/dylanaraps/paleta
 * - https://github.com/alacritty/alacritty/issues/656
 * - http://pod.tst.eu/http://cvs.schmorp.de/rxvt-unicode/doc/rxvt.7.pod#XTerm_Operating_System_Commands
 *
 * Manipulate special colors.
 *   10 = foreground,
 *   11 = background,
 *   12 = cursor foreground,
 *   13 = mouse foreground,
 *   708 = terminal border background
 *
 *   \033]{index};{color}\007
 */
void set_term_colors(PALETTE pal)
{
    size_t succ = 0;
    if (SKIP_TERM_COLORS == NULL)
    {
        char buffer[1024];
        size_t offset = 0;

        const char *fmt_s = "\033]%d;#%s\033\\";
        const char *fmt_p = "\033]4;%d;#%s\033\\";

        /* Create the sequences */
        for (unsigned i = 0; i < PALETTE_SIZE; i++)
        {
            const char *color = palette_color(pal, i, HEX_t);
            offset += snprintf(buffer + offset, sizeof(buffer) - offset, fmt_p, i, color);
        }
        const char *bg_color     = palette_color(pal, 0,  HEX_t);
        const char *fg_color     = palette_color(pal, PALETTE_SIZE - 1, HEX_t);
        const char *cursor_color = palette_color(pal, PALETTE_SIZE - 1, HEX_t);
        const char *border_color = palette_color(pal, PALETTE_SIZE - 1, HEX_t);

        fg_color = fg_color ? fg_color : "FFFFFF";             /* Default to white */
        bg_color = bg_color ? bg_color : "000000";             /* Default to black */
        cursor_color = cursor_color ? cursor_color : "FFFFFF"; /* Default to white */
        border_color = border_color ? border_color : "000000"; /* Default to black */

        offset += snprintf(buffer + offset, sizeof(buffer) - offset, fmt_s, 10, fg_color);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, fmt_s, 11, bg_color);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, fmt_s, 12, cursor_color);
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, fmt_s, 708, border_color);

        /* Broadcast the sequences to all terminal devices */
        glob_t globbuf;
        if (glob("/dev/pts/*", GLOB_NOSORT, NULL, &globbuf) == 0)
        {
            for (size_t i = 0; i < globbuf.gl_pathc; i++)
            {
                FILE *f = fopen(globbuf.gl_pathv[i], "w");
                if (f)
                {
                    fwrite(buffer, 1, offset, f);
                    fclose(f);
                    succ++;
                }
            }
            globfree(&globbuf);
        }

        /*
         * Only write to stdout if it's a terminal,
         * to not conflict with other programs like jq
         */
        if (isatty(STDOUT_FILENO))
        {
            fwrite(buffer, 1, offset, stdout);
            succ++;
        }

    }
    log_c("Set colors to [%d] terminals!", succ);
}

/* cache wallpaper color palette */
void palette_write_cache(char *filepath, PALETTE *p)
{
    if (NO_CACHE_ARG != NULL || JSON_ARG != NULL)
        return;

    if (filepath == NULL)
        return;

    char *filename = basename(filepath);

    size_t cache_file_len = strlen(filename) + strlen(".hellwal") + 1;
    char *cache_file = (char*)malloc(cache_file_len);
    snprintf(cache_file, cache_file_len, "%s.hellwal", filename);

    size_t cache_dir_len = strlen(OUTPUT_ARG) + strlen("/cache/") + 1;
    char *cache_dir = (char*)malloc(cache_dir_len);
    snprintf(cache_dir, cache_dir_len, "%s/cache/", OUTPUT_ARG);

    char *full_cache_path = (char*)malloc(strlen(cache_dir) + strlen(cache_file) + 1);
    snprintf(full_cache_path, strlen(cache_dir) + strlen(cache_file) + 1, "%s%s", cache_dir, cache_file);

    /* create dir if not exits */
    check_output_dir(cache_dir);

    /* create and process template */
    TEMPLATE t;
    t.content = CACHE_TEMPLATE;
    t.path = full_cache_path;
    t.name = cache_file;

    process_template(&t, *p);
    template_write(&t, cache_dir);

    free(cache_file);
    free(cache_dir);
    free(full_cache_path);
}

/* if wallpaper was previously computed, if so, just load it */
int check_cached_palette(char *filepath, PALETTE *p)
{
    if (NO_CACHE_ARG != NULL)
        return 0;

    if (filepath == NULL)
        return 0;

    char *filename = basename(filepath);

    size_t cache_file_len = strlen(filename) + strlen(".hellwal") + 1;
    char *cache_file = (char*)malloc(cache_file_len);
    snprintf(cache_file, cache_file_len, "%s.hellwal", filename);

    size_t cache_dir_len = strlen(OUTPUT_ARG) + strlen("/cache/") + 1;
    char *cache_dir = (char*)malloc(cache_dir_len);
    snprintf(cache_dir, cache_dir_len, "%s/cache/", OUTPUT_ARG);

    char *full_cache_path = (char*)malloc(strlen(cache_dir) + strlen(cache_file) + 1);
    snprintf(full_cache_path, strlen(cache_dir) + strlen(cache_file) + 1, "%s%s", cache_dir, cache_file);

    /* create dir if not exits */
    check_output_dir(cache_dir);

    char *theme = load_file(full_cache_path);

    int result = 1;
    if (theme == NULL)
    {
        warn("Failed to open file: %s", full_cache_path);
        result = 0;
    }
    else
    {
        /* cached palette is stored as theme */
        result = process_theme(theme, p);
    }

    free(cache_file);
    free(cache_dir);
    free(full_cache_path);

    return result;
}

char *process_addtional_variables(char *color, char *right_token, enum COLOR_TYPES type)
{
    if (color == NULL) return NULL;
    if (right_token == NULL) return color;
    char *result = NULL;

    hell_parser_t *p = hell_parser_create(right_token);
    if (hell_parser_delim(p, '=', 1) == HELL_PARSER_OK)
    {
        size_t var_size = p->pos - 1;
        size_t val_size = p->length - p->pos + 1;

        char *variable = calloc(1, var_size);
        char *value = calloc(1, val_size);

        strncpy(variable, p->input, var_size);
        strncpy(value, p->input + p->pos + 1, val_size);

        if (!strcmp(variable, "alpha"))
        {
            result = process_variable_alpha(color, value, type);
        }
    }

    if (result != NULL)
        return result;

     return color;
}

char *process_variable_alpha(char *color, char *value, enum COLOR_TYPES type)
{
    if ((type != HEX_t && type != RGB_t) || !is_between_01_float(value))
        return color;

    const float alpha_value = strtod(value, NULL);
    char *buffer = NULL;

    if (type == HEX_t)
    {
        buffer = malloc(sizeof(color) + 3); // 3 because 2 next hex aplha numbers
                                            // and 1 null termination
        int a = (int)(alpha_value * 255);
        sprintf(buffer, "%s%02x", color, a);
    }
    else // we can safely assume it's rgb
    {
        buffer = malloc(sizeof(color) + sizeof(",    "));
        sprintf(buffer, "%s, %f", color, alpha_value);
    }

    if (buffer != NULL)
        return buffer;

    return color;
}

/* 
 * Loads content from file file to buffer,
 * returns buffer if succeded, otherwise NULL
 */
char *load_file(char *filename)
{
    FILE *file = fopen(filename, "r");
    if (file == NULL)
        return NULL;

    fseek(file, 0, SEEK_END);
    int size = ftell(file);
    rewind(file);

    char *buffer = (char*)malloc(size + 1); // +1 for null terminator
    if (buffer == NULL)
    {
        warn("Failed to allocate memory for file buffer");
        fclose(file);
        return NULL;
    }

    int bytes_read = fread(buffer, 1, size, file);
    if (bytes_read != size)
    {
        perror("Failed to read the complete file");
        free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[size] = '\0';

    fclose(file);

    return buffer;
}

/* 
 * reads content of all given and found templates paths,
 * and writes to specified or default output folder.
 */
void process_templating(PALETTE pal)
{
    if (JSON_ARG != NULL)
    {
        TEMPLATE t;
        t.content = JSON_TEMPLATE;
        t.path = "";
        t.name = "";

        /* printf json template to stdout */
        process_template(&t, pal);
        fprintf(stdout, "%s",t.content);

        return;
    }

    if (DEBUG_ARG != NULL)
        log_c("Processing templates: ");

    TEMPLATE **templates;
    size_t templates_count, t_success = 0;

    /* Process templates loaded from folder */
    templates = get_template_structure_dir(TEMPLATE_FOLDER_ARG, &templates_count);
    if (templates == NULL) return;

    check_output_dir(OUTPUT_ARG);
    for (size_t i = 0; i < templates_count; i++)
    {
        process_template(templates[i], pal);
        t_success += template_write(templates[i], OUTPUT_ARG);
    }

    log_c("Processed [%d/%d] templates!", t_success, templates_count);
}

enum COLOR_TYPES parse_color_type(const char *str)
{
    if (!strcmp(str, "rgb"))
        return RGB_t;

    switch (str[0]) {
    case 'r': return R_t;
    case 'g': return G_t;
    case 'b': return B_t;
    }

    return HEX_t;
}

/* load t->path file to buffer and replaces content between delim with colors from PALETTE colors */
void process_template(TEMPLATE *t, PALETTE pal)
{
    if (t == NULL) {
        t->content = NULL;
        return;
    }

    if (DEBUG_ARG != NULL)
        log_c("  - generating template buffer: %s", t->name);

    char *template_buffer = calloc(1, 1);
    size_t template_size = 0;
    int last_pos = 0;
    int buffrd_pos = 0;

    char *file_content = NULL;
    if (t->content == NULL)
    {
        file_content = load_file(t->path);
        if (file_content == NULL)
        {
            warn("Failed to open file: %s", t->path);
            return;
        }
    }
    else
    {
        file_content = t->content;
    }

    hell_parser_t *p = hell_parser_create(file_content);
    if (p == NULL)
    {
        t->content = NULL;
        return;
    }

    int skip = 0;
    while (!hell_parser_eof(p))
    {
        char ch;
        if (hell_parser_next(p, &ch) == HELL_PARSER_OK)
        {
            if (ch == HELLWAL_DELIM)
            {
                /* escape delim */
                if (skip == 1)
                {
                    template_size += 2;
                    template_buffer = realloc(template_buffer, template_size);
                    if (template_buffer == NULL) return;

                    strncat(template_buffer, p->input + p->pos - 1, 1);
                    buffrd_pos = p->pos;

                    skip = 0;
                }
                else
                {
		    int is_double_delim = 0;
		    if (p->pos < p->length && p->input[p->pos] == HELLWAL_DELIM)
                    {
                        is_double_delim = 1;
                    }
                    
                    if (!is_double_delim)
                    {
                        /* It's a single %, just add it to the buffer and continue */
                        int size_before_delim = p->pos - buffrd_pos;
                        if (size_before_delim > 0)
                        {
                            template_size += size_before_delim + 1;
                            template_buffer = realloc(template_buffer, template_size);
                            strncat(template_buffer, p->input + buffrd_pos, size_before_delim);
                        }
                        buffrd_pos = p->pos;
                        continue;
                    }

                    p->pos -= 1;
                    int idx = 0;
                    size_t len = 0;
                    char *var_arg = NULL;
                    char *delim_buf = NULL;
                    enum COLOR_TYPES type = HEX_t;

                    last_pos = p->pos + 1;

                    int size_before_delim = last_pos - buffrd_pos - 1;
                    if (size_before_delim > 0)
                    {
                        template_size += size_before_delim + 1;
                        template_buffer = realloc(template_buffer, template_size);
                        strncat(template_buffer, p->input + buffrd_pos, size_before_delim);
                    }

                    /* extract hellwal template style from templates:
                     *
                     * example:
                     *    ```template.hellwal
                     *
                     *      random text here
                     *      some randome values
                     *
                     *      %% color1.hex %%  \
                     *      %% color2.hex %%   → -- this is being proccessed by hell_parser
                     *      %% color3.hex %%  /
                     *
                     *    ```
                     *
                     */
                    if (hell_parser_delim_buffer_between(p, HELLWAL_DELIM, HELLWAL_DELIM_COUNT, &delim_buf) == HELL_PARSER_OK)
                    {
                        remove_extra_whitespaces(delim_buf);
                        hell_parser_t *pdt = hell_parser_create(delim_buf);
                        char *L_TOKEN = NULL;
                        char *R_TOKEN = NULL;

                        if (pdt == NULL)
                            err("Failed to allocate parser");

                        /* tokenize it by ' ':
                         *
                         *  %%color0.hex alpha=0.1%    -   this will turn into: 
                         *    L_TOKEN = "color0.hex";
                         *    R_TOKEN = "alpha=1.1";
                         *
                         * --------------------------------------------------
                         *
                         *  %%color4 alpha=0.5%%
                         *    L_TOKEN = "color4";
                         *    R_TOKEN = "alpha=0.5";
                         *
                         * --------------------------------------------------
                         *
                         *  %%color7%%"   -   hell_parser_delim will fail and go to else statement
                         *    L_TOKEN = "color7";
                         *    R_TOKEN = NULL;
                         *
                         */

                        if (hell_parser_delim(pdt, ' ', 1) == HELL_PARSER_OK) // TODO: make it work with other variables
                                                                              // (that dont exist yet) - for now this function
                                                                              // assumes that there are no more than 2 variables
                                                                              // provided (e.g. color1 + alpha for example)
                        {
                            size_t l_size = pdt->pos - 1;
                            size_t r_size = pdt->length - pdt->pos;

                            char *left  = calloc(1, l_size);
                            char *right = calloc(1, r_size);

                            strncpy(left, pdt->input, l_size);
                            strncpy(right, pdt->input + pdt->pos, r_size);

                            if (DEBUG_ARG != NULL)
                                log_c("\nLEFT: %s\nRIGHT: %s\n", left, right);

                            L_TOKEN = left;
                            R_TOKEN = right;

                            if (!strcmp(L_TOKEN, "") ||
                                    (R_TOKEN != NULL && !strcmp(R_TOKEN, "")))
                            {
                                free(R_TOKEN);
                                R_TOKEN = NULL;
                                L_TOKEN = delim_buf;
                            }
                            
                            if (DEBUG_ARG != NULL)
                            {
                                log_c("--------------------------------------------------");
                                log_c("L_TOKEN: %s", L_TOKEN);
                                log_c("R_TOKEN: %s", R_TOKEN);
                            }
                        }
                        else
                        {
                            L_TOKEN = delim_buf;
                        }

                        hell_parser_t *pd = hell_parser_create(L_TOKEN);
                        if (pd == NULL)
                            err("Failed to allocate parser");

                        if (hell_parser_delim(pd, '.', 1) == HELL_PARSER_OK)
                        {
                            size_t l_size = pd->pos - 1;
                            size_t r_size = pd->length - pd->pos;

                            char *left  = calloc(1, l_size);
                            char *right = calloc(1, r_size);

                            strncpy(left, pd->input, l_size);
                            strncpy(right, pd->input + pd->pos, r_size);

                            remove_whitespaces(left);
                            remove_whitespaces(right);

                            /*
                             * Checks if the color keyword is valid.
                             * Also checks if there is a color type, and defaults to hex if not.
                             */
                            idx = is_color_palette_var(left);
                            if (idx != -1 && pd->pos < pd->length)
                            {
                                type = parse_color_type(right);
                                var_arg = palette_color(pal, idx, type);
                            }
                            else if (!strcmp(left, "foreground") || !strcmp(left, "cursor") || !strcmp(left, "border"))
                            {
                                type = parse_color_type(right);
                                var_arg = palette_color(pal, PALETTE_SIZE - 1, type);
                            }
                            else if (!strcmp(left, "background"))
                            {
                                type = parse_color_type(right);
                                var_arg = palette_color(pal, 0, type);
                            }

                            free(left);
                            free(right);
                        }
                        /* check if an argument stands for wallpaper path */
                        else if (!strcmp(delim_buf, "wallpaper"))
                        {
                            if (IMAGE_ARG != NULL)
                            {
                                len = strlen(IMAGE_ARG);
                                var_arg = IMAGE_ARG;
                            }
                            else if (THEME_ARG)
                                var_arg = THEME_ARG;
                            else
                                var_arg = "";
                        }
                        /* check other keywords */
                        else if (!strcmp(delim_buf, "foreground") || !strcmp(delim_buf, "cursor") || !strcmp(delim_buf, "border"))
                            var_arg = palette_color(pal, PALETTE_SIZE - 1, HEX_t);
                        else if (!strcmp(delim_buf, "background"))
                            var_arg = palette_color(pal, 0, HEX_t);
                        else
                        {
                            /* '.' was not found, try to find color, put hex by default on it */
                            idx = is_color_palette_var(L_TOKEN);

                            if (idx != -1)
                                var_arg = palette_color(pal, idx, HEX_t);
                        }

                        if (var_arg != NULL)
                        {
                            // process alpha if provided by R_TOKEN
                            var_arg = process_addtional_variables(var_arg, R_TOKEN, type);

                            len = strlen(var_arg);
                            template_size += len + 1;
                            template_buffer = realloc(template_buffer, template_size);
                            if (template_buffer == NULL) return;
                            strcat(template_buffer, var_arg);
                        }

                        /* Update last read buffer position */
                        buffrd_pos = p->pos;

                        skip = 0;
                        free(delim_buf);
                        hell_parser_destroy(pd);
                        hell_parser_destroy(pdt);
                    }
                }
            }
            /* this is for escaping template delim */
            else if (ch == '\\')
            {
                skip = 1;
            }
            else
                skip = 0;
        }
    }

    /* 
     * If content of template does not end on delim,
     * add rest of the content
     */
    if ((size_t)buffrd_pos < p->length)
    {
        size_t size = p->length - buffrd_pos;

        template_size += size + 1;
        template_buffer = realloc(template_buffer, template_size);
        if (template_buffer == NULL) 
            return;

        strncat(template_buffer, p->input + buffrd_pos, size);
    }

    hell_parser_destroy(p);

    t->content = template_buffer;
}

/* return array of TEMPLATE structure of files in directory */
TEMPLATE **get_template_structure_dir(const char *dir_path, size_t *_size)
{
    if (dir_path == NULL) return NULL;

    TEMPLATE **t_arr = NULL;
    size_t size = 0;

    DIR *dir = opendir(dir_path);
    if (dir == NULL)
    {
        warn("Cannot access directory: %s", dir_path);
        return NULL;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            size_t path_len = strlen(dir_path) + strlen(entry->d_name) + 2; // +2 for '/' and '\0'
            char *full_path = malloc(path_len);
            if (full_path == NULL)
            {
                perror("Failed to allocate memory for file path");
                closedir(dir);
                return NULL;
            }
            snprintf(full_path, path_len, "%s/%s", dir_path, entry->d_name);

            TEMPLATE **new_t_arr = realloc(t_arr, (size + 1) * sizeof(TEMPLATE*));
            if (new_t_arr == NULL)
            {
                perror("Failed to allocate memory for templates array");
                free(full_path);
                closedir(dir);
                free(t_arr);
                return NULL;
            }
            t_arr = new_t_arr;

            t_arr[size] = calloc(1 ,sizeof(TEMPLATE));
            t_arr[size]->path = full_path;
            t_arr[size]->name = strdup(entry->d_name);

            size++;
        }
    }

    closedir(dir);

    if (_size)
        *_size = size;

    return t_arr;
}

/* 
 * write generated template to dir,
 * returns 1 on success
 */
size_t template_write(TEMPLATE *t, char *dir)
{
    if (t == NULL || dir == NULL) return 0;
    if (t->content == NULL) return 0;

    char* path = malloc(strlen(dir) + strlen(t->name) + 1);
    if (path)
        sprintf(path, "%s%s", dir, t->name);
    else
        return 0;

    FILE *f = fopen(path, "w");
    if (f == NULL)
    {
        warn("Cannot write to file: %s", path);
        free(path);
        return 0;
    }

    fprintf(f, "%s", t->content);
    
    if (DEBUG_ARG != NULL)
        log_c("  - wrote template to: %s\n", path);

    fclose(f);
    free(path);

    return 1;
}

/*
 * check in directory for theme with provided name,
 * if not exist, try to open it as a path */
char *load_theme(char *themename)
{
    log_c("Loading static theme: %s", THEME_ARG);
    DIR *dir = opendir(THEME_FOLDER_ARG);
    char *t = NULL;
    struct dirent *entry;

    if (dir == NULL)
        warn("Cannot access directory: %s", THEME_FOLDER_ARG);
    else
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type == DT_REG && strcmp(entry->d_name, themename) == 0)
            {
                size_t path_len = strlen(THEME_FOLDER_ARG) + strlen(entry->d_name) + 2; // +2 for '/' and '\0'
                char *path = malloc(path_len);
                if (path == NULL) {
                    perror("Failed to allocate memory for file path");
                    closedir(dir);
                    return NULL;
                }

                sprintf(path, "%s/%s", THEME_FOLDER_ARG, entry->d_name); 
                t = load_file(path);
                
                if (t != NULL)
                    return t;
            }
        }
    }
    t = load_file(themename);
    if (t != NULL)
        return t;

    warn("Failed to open file: %s", themename);
    return NULL;
}

int hex_to_rgb(const char *hex, RGB *p)
{
    if (!hex || hex[0] != '#' || (strlen(hex) != 7 && strlen(hex) != 4)) {
        return 0;
    }

    if (strlen(hex) == 7) {
        // #RRGGBB format
        p->R = (int)strtol(hex + 1, NULL, 16) >> 16 & 0xFF;
        p->G = (int)strtol(hex + 3, NULL, 16) >> 8 & 0xFF;
        p->B = (int)strtol(hex + 5, NULL, 16) & 0xFF;
    } else if (strlen(hex) == 4) {
        // #RGB format (expand each digit to 2 digits)
        p->R = (int)strtol((char[]){hex[1], hex[1], '\0'}, NULL, 16);
        p->G = (int)strtol((char[]){hex[2], hex[2], '\0'}, NULL, 16);
        p->B = (int)strtol((char[]){hex[3], hex[3], '\0'}, NULL, 16);
    }

    return 1;
}

int is_color_palette_var(char *name)
{
    for (size_t i = 0; i < PALETTE_SIZE; i++) {
        char *col = calloc(1, 16);
        sprintf(col, "color%lu", i);

        if (strcmp(col, name) == 0)
            return i;
    }
    return -1;
}

/* process theme, return color palette - return 0 on error */
int process_theme(char *t, PALETTE *pal)
{
    if (t == NULL)
        return 0;

    int processed_colors = 0;
    hell_parser_t *p = hell_parser_create(t);

    if (p == NULL) 
        err("Failed to create parser");

    while (!hell_parser_eof(p))
    {
        char ch;
        if (hell_parser_next(p, &ch) == HELL_PARSER_OK)
        {
            if (ch == HELLWAL_DELIM)
            {
                p->pos -= 1;  
                char *delim_buf = NULL;

                if (hell_parser_delim_buffer_between(p, HELLWAL_DELIM, HELLWAL_DELIM_COUNT, &delim_buf) == HELL_PARSER_OK)
                {
                    hell_parser_t *pd = hell_parser_create(delim_buf);
                    if (pd == NULL)
                        err("Failed to allocate parser");

                    if (hell_parser_delim(pd, '=', 1) == HELL_PARSER_OK) {
                        size_t var_size = pd->pos - 1;
                        size_t val_size = pd->length - pd->pos + 1;

                        char *variable = calloc(1, var_size);
                        char *value = calloc(1, val_size);

                        strncpy(variable, pd->input, var_size);
                        strncpy(value, pd->input + pd->pos + 1, val_size);

                        remove_whitespaces(variable);
                        remove_whitespaces(value);

                        RGB p;
                        if (hex_to_rgb(value, &p))
                        {
                            int idx = is_color_palette_var(variable);
                            if (idx != -1) {
                                pal->colors[idx] = p;
                                processed_colors++;
                            }
                        }
                        else
                        {
                            free(variable);
                            free(value);
                        }
                    }

                    free(delim_buf);
                }
            }
        }
    }
    hell_parser_destroy(p);

    if (processed_colors == PALETTE_SIZE)
        return 1;

    return 0;
}

PALETTE process_themeing(char *theme)
{
    char *t = load_theme(theme);
    PALETTE pal;

    if (t!=NULL)
    {
        if (!process_theme(t, &pal))
            err("Not enough colors were specified in color palette: %s", theme);
    }
    else
        err("Theme not found: %s", theme);

    return pal;
}

/***
 * MAIN
 ***/
int main(int argc, char **argv)
{
    /* read cmd line arguments, and set default ones */
    if (set_args(argc,argv) != 0)
        err("arguments error");

    /* generate palette from image or theme */
    PALETTE pal = get_color_palette(pal);

    /* apply theme'ing options liek --light, --color, --gray-scale 0.5 */
    apply_addtional_arguments(&pal);

    /* print palette colors as blocks*/
    print_palette(pal);

    /* set terminal colors using ANSI escape codes */
    set_term_colors(pal);

    /* read template files, process them and write results to --output */
    process_templating(pal);

    /* Run script or command from --script argument */
    run_script(SCRIPT_ARG);

    return 0;
}
