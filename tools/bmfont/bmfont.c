#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#define BLOCK_TYPE_GLYPHS               4
#define BLOCK_TYPE_KERNING_PAIRS        5

typedef struct {
        uint32_t id;
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        int16_t x_offs;
        int16_t y_offs;
        uint16_t x_advance;
} __attribute__ ((packed)) bmfont_glyph;

typedef struct {
        uint32_t first;
        uint32_t second;
        int16_t amount;
} __attribute__ ((packed)) bmfont_kerning_pair;

typedef struct {
        char magic[3];
        uint8_t version;
} __attribute__ ((packed)) bmfont;

static char *progname(char *[]);
static bool seek_block_type(FILE *, int, uint8_t *, int32_t *);

int
main(int argc, char *argv[])
{
        FILE *fp;

        if ((fp = fopen(argv[1], "rb")) == NULL) {
                (void)fprintf(stderr, "%s: %s\n", progname(argv),
                    strerror(errno));

                return 1;
        }

        bmfont bmfont;

        if ((fread(&bmfont, 1, sizeof(bmfont), fp)) < 0) {
                (void)fprintf(stderr, "%s: %s\n", progname(argv),
                    strerror(errno));

                return 1;
        }

        (void)printf("%s\n", bmfont.magic);

        uint8_t type;
        int32_t size;

        if ((seek_block_type(fp, BLOCK_TYPE_GLYPHS, &type, &size))) {
                (void)printf("type: %d, size: %d\n", type, size);
        }

        bmfont_glyph **glyphs;
        size_t glyphcnt;
        uint32_t glyphidx;

        glyphcnt = size / sizeof(bmfont_glyph);
        glyphs = (bmfont_glyph **)malloc(sizeof(bmfont_glyph) * glyphcnt);

        (void)printf("%lu\n", glyphcnt);

        for (glyphidx = 0; glyphidx < glyphcnt; glyphidx++) {
                bmfont_glyph *glyph;

                glyph = (bmfont_glyph *)malloc(sizeof(bmfont_glyph));
                fread(glyph, sizeof(bmfont_glyph), 1, fp);

                glyphs[glyphidx] = glyph;
        }

        if ((seek_block_type(fp, BLOCK_TYPE_KERNING_PAIRS, &type, &size))) {
                (void)printf("type: %d, size: %d\n", type, size);
        }

        bmfont_kerning_pair **kerning_pairs;
        size_t kpcnt;
        uint32_t kpidx;

        kpcnt = size / sizeof(bmfont_kerning_pair);
        kerning_pairs =
            (bmfont_kerning_pair **)malloc(sizeof(bmfont_kerning_pair) * kpcnt);

        for (kpidx = 0; kpidx < kpcnt; kpidx++) {
                bmfont_kerning_pair *kp;

                kp = (bmfont_kerning_pair *)malloc(sizeof(bmfont_kerning_pair));
                fread(kp, sizeof(bmfont_kerning_pair), 1, fp);

                kerning_pairs[kpidx] = kp;
        }

        for (glyphidx = 0; glyphidx < glyphcnt; glyphidx++) {
                free(glyphs[glyphidx]);
        }

        free(glyphs);

        for (kpidx = 0; kpidx < kpcnt; kpidx++) {
                free(kerning_pairs[kpidx]);
        }

        free(kerning_pairs);

        return 0;
}

static char *
progname(char *argv[])
{
        static char *cached_progname = NULL;

        if (cached_progname == NULL) {
                cached_progname = basename(argv[0]);
        }

        return cached_progname;
}

static bool
seek_block_type(FILE *fp, int type, uint8_t *out_type, int32_t *out_size)
{
        rewind(fp);
        fseek(fp, sizeof(bmfont), SEEK_SET);

        do {
                uint8_t rtype;
                int32_t rsize;

                fread(&rtype, sizeof(rtype), 1, fp);
                fread(&rsize, sizeof(rsize), 1, fp);

                if (rtype == type) {
                        *out_type = rtype;
                        *out_size = rsize;

                        return true;
                }

                if (feof(fp)) {
                        return false;
                }

                fseek(fp, rsize, SEEK_CUR);

                continue;
        } while (true);
}
