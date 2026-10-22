/* Shared, versioned cache of final palettes, grouped by appearance arguments. */
#ifndef HELLWAL_PALETTE_CACHE_H
#define HELLWAL_PALETTE_CACHE_H

static uint64_t cache_hash(uint64_t h, const void *data, size_t size)
{
    const unsigned char *p = data;
    for (size_t i = 0; i < size; i++) { h ^= p[i]; h *= UINT64_C(1099511628211); }
    return h;
}

static char *palette_cache_path(void)
{
    if (ARGS.NO_CACHE) return NULL;
    uint64_t identity = UINT64_C(14695981039346656037);
    if (ARGS.IMAGE) {
        FILE *f = fopen(ARGS.IMAGE, "rb");
        if (!f) return NULL;
        unsigned char buffer[16384]; size_t n;
        while ((n = fread(buffer, 1, sizeof(buffer), f))) identity = cache_hash(identity, buffer, n);
        int ok = !ferror(f);
        fclose(f);
        if (!ok) return NULL;
    }
    if (ARGS.THEME) {
        /* Resolve the same named-theme precedence without emitting duplicate
         * load diagnostics while computing identity. */
        char *theme = NULL;
        if (!strchr(ARGS.THEME, '/')) {
            char *candidate = join_path(ARGS.THEME_FOLDER, ARGS.THEME);
            struct stat st;
            if (!lstat(candidate, &st) && S_ISREG(st.st_mode)) theme = load_file(candidate);
            free(candidate);
        }
        if (!theme) theme = load_file(ARGS.THEME);
        if (!theme) return NULL;
        identity = cache_hash(identity, "\0theme\0", 7);
        identity = cache_hash(identity, theme, strlen(theme));
        free(theme);
    }
    /* Normalize implicit dark, but preserve combinations: classic applies
     * dark, light and color in that order. Output-only flags do not matter. */
    int dark = ARGS.DARK_MODE || (!ARGS.THEME && !ARGS.LIGHT_MODE && !ARGS.COLOR_MODE);
    char options[512];
    RGB bg = ARGS.STATIC_BG ? *ARGS.STATIC_BG : (RGB){0};
    RGB fg = ARGS.STATIC_FG ? *ARGS.STATIC_FG : (RGB){0};
    snprintf(options, sizeof(options),
             "v2/%s/comfort=%u/dlc=%d%u%u/neon=%u/invert=%u/sort=%u/contrast=%u/dominant=%u/gray=%a/darken=%a/brighten=%a/theme=%d/override=%u/bg=%d:%02x%02x%02x/fg=%d:%02x%02x%02x",
             VERSION, ARGS.COMFORT_MODE, dark, ARGS.LIGHT_MODE, ARGS.COLOR_MODE,
             ARGS.NEON_MODE, ARGS.INVERT, ARGS.SKIP_LUMINANCE_SORTING,
             ARGS.COMFORT_MODE ? 0 : ARGS.CHECK_CONTRAST, ARGS.DOMINANT_BACKGROUND,
             (double)ARGS.GRAY_SCALE, (double)ARGS.DARKNESS_OFFSET, (double)ARGS.BRIGHTNESS_OFFSET,
             ARGS.THEME != NULL, ARGS.OVERRIDE_COLORS,
             ARGS.STATIC_BG != NULL, bg.R, bg.G, bg.B, ARGS.STATIC_FG != NULL, fg.R, fg.G, fg.B);
    uint64_t arguments = cache_hash(UINT64_C(14695981039346656037), options, strlen(options));
    char folder[80], filename[32];
    snprintf(folder, sizeof(folder), "%s-%s-%016llx", ARGS.COMFORT_MODE ? "comfort" : "classic",
             ARGS.COLOR_MODE ? "color" : ARGS.LIGHT_MODE ? "light" : dark ? "dark" : "theme",
             (unsigned long long)arguments);
    snprintf(filename, sizeof(filename), "%016llx.palette", (unsigned long long)identity);
    char *root = join_path(ARGS.OUTPUT, "cache");
    char *dir = join_path(root, folder);
    char *path = join_path(dir, filename);
    free(root); free(dir);
    return path;
}

static int palette_cache_read(const char *path, PALETTE *pal)
{
    if (!path) return 0;
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    unsigned version, comfort, tonal;
    int ok = fscanf(f, "HELLWAL_PALETTE %u %u %u", &version, &comfort, &tonal) == 3 &&
             version == 2 && comfort == ARGS.COMFORT_MODE && tonal <= 1;
    PALETTE candidate = {0}; RGB special[5] = {{0}};
    for (unsigned i = 0; ok && i < (comfort ? 21u : 16u); i++) {
        unsigned r, g, b;
        ok = fscanf(f, "%u %u %u", &r, &g, &b) == 3 && r <= 255 && g <= 255 && b <= 255;
        if (ok) *(i < 16 ? &candidate.colors[i] : &special[i-16]) = (RGB){r,g,b};
    }
    int ch;
    do { ch = fgetc(f); } while (ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t');
    ok = ok && ch == EOF && !ferror(f);
    fclose(f);
    if (ok) {
        *pal = candidate;
        comfort_tonal_variants = tonal;
        if (comfort) memcpy(comfort_special, special, sizeof(special));
    }
    return ok;
}

static void palette_cache_write(const char *path, const PALETTE *pal)
{
    if (!path) return;
    char *dir = strdup(path);
    if (!dir) return;
    *strrchr(dir, '/') = '\0';
    int ready = check_output_dir(dir);
    free(dir);
    if (!ready) return;
    char *temporary = malloc(strlen(path) + 8);
    if (!temporary) return;
    sprintf(temporary, "%s.XXXXXX", path);
    int fd = mkstemp(temporary);
    if (fd < 0) { free(temporary); return; }
    FILE *f = fdopen(fd, "w");
    if (!f) { close(fd); unlink(temporary); free(temporary); return; }
    int ok = fprintf(f, "HELLWAL_PALETTE 2 %u %d\n", ARGS.COMFORT_MODE, comfort_tonal_variants) > 0;
    for (int i = 0; i < (ARGS.COMFORT_MODE ? 21 : 16); i++) {
        RGB c = i < 16 ? pal->colors[i] : comfort_special[i-16];
        if (fprintf(f, "%u %u %u\n", c.R, c.G, c.B) < 0) ok = 0;
    }
    if (fclose(f)) ok = 0;
    if (ok && rename(temporary, path)) ok = 0;
    if (!ok) unlink(temporary);
    free(temporary);
}

typedef struct { uint64_t files, bytes, allocated; int errors; } CACHE_REMOVED;

/* Work relative to open directories and never traverse symbolic links. */
static void cache_remove_at(int parent, const char *name, CACHE_REMOVED *result)
{
    struct stat st;
    if (fstatat(parent, name, &st, AT_SYMLINK_NOFOLLOW)) {
        if (errno != ENOENT) { fprintf(stderr, "Cannot inspect cache %s: %s\n", name, strerror(errno)); result->errors++; }
        return;
    }
    if (S_ISDIR(st.st_mode)) {
        int fd = openat(parent, name, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
        if (fd < 0) { fprintf(stderr, "Cannot open cache %s: %s\n", name, strerror(errno)); result->errors++; return; }
        DIR *dir = fdopendir(fd);
        if (!dir) { close(fd); result->errors++; return; }
        struct dirent *entry;
        for (;;) {
            errno = 0;
            entry = readdir(dir);
            if (!entry) { if (errno) result->errors++; break; }
            if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
                cache_remove_at(fd, entry->d_name, result);
        }
        closedir(dir);
        if (unlinkat(parent, name, AT_REMOVEDIR)) {
            fprintf(stderr, "Cannot remove cache directory %s: %s\n", name, strerror(errno)); result->errors++;
        } else result->allocated += (uint64_t)st.st_blocks * 512;
    } else if (unlinkat(parent, name, 0)) {
        fprintf(stderr, "Cannot remove cache file %s: %s\n", name, strerror(errno)); result->errors++;
    } else {
        result->files++;
        result->bytes += st.st_size;
        if (st.st_nlink == 1) result->allocated += (uint64_t)st.st_blocks * 512;
    }
}

static int palette_cache_clear(void)
{
    CACHE_REMOVED removed = {0};
    int fd = open(ARGS.OUTPUT, O_RDONLY | O_DIRECTORY);
    if (fd < 0 && errno != ENOENT) {
        fprintf(stderr, "Cannot open cache output %s: %s\n", ARGS.OUTPUT, strerror(errno)); return 1;
    }
    if (fd >= 0) {
        cache_remove_at(fd, "cache", &removed);
        /* Also remove extraction caches left by pre-release builds. */
        cache_remove_at(fd, "cache-oklab-v1", &removed);
        close(fd);
    }
    FILE *report = ARGS.JSON ? stderr : stdout;
    fprintf(report, "%s cache: %llu files, %llu bytes of cached data removed; %llu bytes (%.2f KiB) disk space reclaimed.\n",
            removed.errors ? "Partially cleared" : "Cleared", (unsigned long long)removed.files,
            (unsigned long long)removed.bytes, (unsigned long long)removed.allocated, removed.allocated / 1024.0);
    if (ARGS.JSON) printf("{\"files_removed\":%llu,\"bytes_removed\":%llu,\"disk_bytes_reclaimed\":%llu,\"errors\":%d}\n",
                          (unsigned long long)removed.files, (unsigned long long)removed.bytes,
                          (unsigned long long)removed.allocated, removed.errors);
    return removed.errors ? 1 : 0;
}
#endif
