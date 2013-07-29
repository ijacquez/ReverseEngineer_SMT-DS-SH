#include <errno.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>

#define BLOCK_TYPE_GLYPHS               4
#define BLOCK_TYPE_KERNING_PAIRS        5

#define SWAP_16(x) ((((x) >> 8) & 0x00FF) | (((x) & 0x00FF) << 8))
#define SWAP_32(x) (((x) >> 16) | (((x) & 0x0000FFFF) << 16))

typedef struct {
        uint32_t id;
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        int16_t x_offs;
        int16_t y_offs;
        int16_t x_advance;
        uint8_t page;
        uint8_t channel;
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

typedef struct {
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        int16_t x_offs;
        int16_t y_offs;
        int16_t x_advance;
} __attribute__ ((packed)) glyph;

static int usage(char *[]);
static char *progname(char *[]);
static bool seek_block_type(FILE *, int, uint8_t *, int32_t *);

static void *swap(void *, uint32_t);

int
main(int argc, char *argv[])
{
        char *fnt_file;
        char *bin_file;
        FILE *fp;
        int ret;

        ret = 0;

        if (argc < 3) {
                return usage(argv);
        }

        fnt_file = argv[1];
        if ((fp = fopen(fnt_file, "rb")) == NULL) {
                (void)fprintf(stderr, "%s: %s\n", progname(argv),
                    strerror(errno));

                return 1;
        }

        bin_file = argv[2];

        bmfont bmfont;

        if ((fread(&bmfont, 1, sizeof(bmfont), fp)) < 0) {
                (void)fprintf(stderr, "%s: %s\n", progname(argv),
                    strerror(errno));

                return 1;
        }

        (void)printf("%c%c%c ver. %d\n", bmfont.magic[0], bmfont.magic[1],
            bmfont.magic[2], bmfont.version);

        FILE *bfp;

        if ((bfp = fopen(bin_file, "w+b")) == NULL) {
                (void)fprintf(stderr, "%s: %s\n", progname(argv),
                    strerror(errno));

                return 1;
        }

        uint8_t type;
        int32_t size;

        if (!(seek_block_type(fp, BLOCK_TYPE_GLYPHS, &type, &size))) {
                ret = 1;

                goto exit;
        }

        glyph **glyphs;
        uint16_t glyphcnt;
        uint16_t glyphidx;

        glyphcnt = size / sizeof(bmfont_glyph);
        glyphs = (glyph **)malloc(sizeof(glyph) * 256);

        for (glyphidx = 0; glyphidx < 256; glyphidx++) {
                glyphs[glyphidx] = (glyph *)malloc(sizeof(glyph));

                memset(glyphs[glyphidx], 0x00, sizeof(glyph));
        }

        uint16_t glyphcnt_sw;

        glyphcnt_sw = SWAP_16(glyphcnt);
        fwrite(&glyphcnt_sw, sizeof(glyphcnt_sw), 1, bfp);

        for (glyphidx = 0; glyphidx < glyphcnt; glyphidx++) {
                bmfont_glyph t_glyph;
                glyph *glyph;

                if ((fread(&t_glyph, sizeof(bmfont_glyph), 1, fp)) < 0) {
                        (void)fprintf(stderr, "%s: %s\n", progname(argv),
                            strerror(errno));

                        goto exit;
                }

                glyph = glyphs[t_glyph.id];

                glyph->x = SWAP_16(t_glyph.x);
                glyph->y = SWAP_16(t_glyph.y);
                glyph->width = SWAP_16(t_glyph.width);
                glyph->height = SWAP_16(t_glyph.height);
                glyph->x_offs = SWAP_16(t_glyph.x_offs);
                glyph->y_offs = SWAP_16(t_glyph.y_offs);
                glyph->x_advance = SWAP_16(t_glyph.x_advance);
        }

        for (glyphidx = 0; glyphidx < 256; glyphidx++) {
                fwrite(glyphs[glyphidx], sizeof(glyph), 1, bfp);
        }

        if (!(seek_block_type(fp, BLOCK_TYPE_KERNING_PAIRS, &type, &size))) {
                ret = 1;

                goto exit;
        }

        bmfont_kerning_pair **kerning_pairs;
        uint16_t kpcnt;
        uint16_t kpidx;

        kpcnt = size / sizeof(bmfont_kerning_pair);
        kerning_pairs =
            (bmfont_kerning_pair **)malloc(sizeof(bmfont_kerning_pair) * kpcnt);

        uint16_t kpcnt_sw;

        kpcnt_sw = SWAP_16(kpcnt);
        fwrite(&kpcnt_sw, sizeof(kpcnt_sw), 1, bfp);

        for (kpidx = 0; kpidx < kpcnt; kpidx++) {
                bmfont_kerning_pair *kp;

                kp = (bmfont_kerning_pair *)malloc(sizeof(bmfont_kerning_pair));
                fread(kp, sizeof(bmfont_kerning_pair), 1, fp);

                kerning_pairs[kpidx] = kp;

                kp->first = SWAP_32(kp->first);
                kp->second = SWAP_32(kp->second);
                kp->amount = SWAP_16(kp->amount);

                fwrite(kp, sizeof(bmfont_kerning_pair), 1, bfp);
        }

        (void)printf("Completed\n");

exit:
        for (glyphidx = 0; glyphidx < glyphcnt; glyphidx++) {
                free(glyphs[glyphidx]);
        }

        free(glyphs);

        for (kpidx = 0; kpidx < kpcnt; kpidx++) {
                free(kerning_pairs[kpidx]);
        }

        free(kerning_pairs);

        fclose(fp);
        fclose(bfp);

        return ret;
}

static int
usage(char *argv[])
{
        (void)fprintf(stderr, "%s: [fnt-file] [bin-file]\n", progname(argv));

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
        } while (true);
}

static void *
swap(void *buf, uint32_t size)
{
        uint16_t *bufp;
        uint16_t *newp;
        uint32_t i;

        bufp = (uint16_t *)buf;
        newp = (uint16_t *)malloc(size / sizeof(uint16_t));

        memset(newp, 0x00, size);

        for (i = 0; i < (size / sizeof(uint16_t)); i++) {
                newp[i] = SWAP_16(bufp[i]);
        }

        return newp;
}
