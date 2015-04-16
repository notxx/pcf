#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include "pcf.h"

#define PCF_PROPERTIES          (1<<0)
#define PCF_ACCELERATORS        (1<<1)
#define PCF_METRICS             (1<<2)
#define PCF_BITMAPS             (1<<3)
#define PCF_INK_METRICS         (1<<4)
#define PCF_BDF_ENCODINGS       (1<<5)
#define PCF_SWIDTHS             (1<<6)
#define PCF_GLYPH_NAMES         (1<<7)
#define PCF_BDF_ACCELERATORS    (1<<8)

#define PCF_DEFAULT_FORMAT      0x00000000
#define PCF_INKBOUNDS           0x00000200
#define PCF_ACCEL_W_INKBOUNDS   0x00000100
#define PCF_COMPRESSED_METRICS  0x00000100

#define PCF_BYTE_MASK           (1<<2)
#define DEBUG 0
#define bread32_0(f, e) bread32(u32_0, f, e)
#define bread32_1(f, e) bread32(u32_1, f, e)
#define bread16_0(f, e) bread16(u16_0, f, e)
#define bread16_1(f, e) bread16(u16_1, f, e)

static uint8 bread[4];

uint32 u32_0(uint8 * buf) {
	uint32 result = 0;
	for (int i = 3; i >= 0; i--) {
		result <<= 8;
		result |= buf[i];
	}
	return result;
}

uint32 u32_1(uint8 * buf) {
	uint32 result = 0;
	for (int i = 0; i < 4; i++) {
		result <<= 8;
		result |= buf[i];
	}
	return result;
}

uint16 u16_0(uint8 * buf) {
	uint16 result = 0;
	for (int i = 1; i >= 0; i--) {
		result <<= 8;
		result |= buf[i];
	}
	return result;
}

uint16 u16_1(uint8 * buf) {
	uint16 result = 0;
	for (int i = 0; i < 2; i++) {
		result <<= 8;
		result |= buf[i];
	}
	return result;
}

uint32 bread32(uint32 (*r)(), FILE * f, error * err) {
	if (fread(bread, 1, 4, f) < 0) {
		*err = "fread error";
	}
	return r(bread);
}

uint32 bread16(uint16 (*r)(), FILE * f, error * err) {
	if (fread(bread, 1, 2, f) < 0) {
		*err = "fread error";
	}
	return r(bread);
}

/**
 * 读取unicode与字模偏移表。
 */
void pcf_encoding_load(encoding_table * t, FILE * f, error * err) {
#if (DEBUG)
	printf("%s offset: %d\n", __FUNCTION__, t->table->offset);
#endif
	if (fseek(f, t->table->offset, SEEK_SET) < 0) { *err = "fseek error"; return; }
	t->format = bread32_0(f, err);
#if (DEBUG)
	printf("%s format: %04X %04X\n", __FUNCTION__, t->format, t->table->format);
#endif
	if (*err) { return; }
	t->minCharOrByte2 = bread16_1(f, err);
	if (*err) { return; }
	t->maxCharOrByte2 = bread16_1(f, err);
	if (*err) { return; }
	t->minByte1 = bread16_1(f, err);
	if (*err) { return; }
	t->maxByte1 = bread16_1(f, err);
	if (*err) { return; }
	t->defChar = bread16_1(f, err);
	if (*err) { return; }
	int32 size = t-> index_count = (t->maxCharOrByte2 - t->minCharOrByte2 + 1) * (t->maxByte1 - t->minByte1 + 1);
	int16 * indexes = t->indexes = malloc(sizeof(int16) * size);
	for (int i = 0; i < size; i++) {
		indexes[i] = bread16_1(f, err);
		if (*err) { return; }
	}
	if ((t->maxCharOrByte2 - t->minCharOrByte2) == 0xff
			&& (t->maxByte1 - t->minByte1) == 0xff)
		t->direct = 1;
	else
		t->direct = 0;
#if (DEBUG)
	printf("encoding table: %d %d %d %d %d\n", t->minCharOrByte2, t->maxCharOrByte2, t->minByte1, t->maxByte1, size);
#endif
}


/**
 * 查找unicode字符对应的字模偏移。
 */
uint32 pcf_encoding_find(encoding_table * t, uint32 r, error * err) {
	if (t->direct) {
#if (DEBUG)
		printf("%s direct\n", __FUNCTION__);
#endif
		return t->indexes[r];
	}
	uint8 b1 = 0xff & r, b2 = (r >> 8) & 0xff;
	uint32 off = 0;
	if (b2 == 0) {
		off = b1 - t->minCharOrByte2;
	} else {
		off = (b2 - (uint32)t->minByte1) * (t->maxCharOrByte2 - t->minCharOrByte2 + 1)
			+ (b1 - t->minCharOrByte2);
	}
#if (DEBUG)
	printf("encoding_lookup0: %x %x %x %x\n", r, off, b1, b2);
	printf("encoding_lookup1: %x %x %x %x\n", t->minCharOrByte2, t->maxCharOrByte2, t->minByte1, t->maxByte1);
	printf("encoding_lookup3: %x %x\n", off, t->indexes[off]);
#endif
	return t->indexes[off];
}

/**
 * 读取字形偏移表。
 */
void pcf_metric_load(metric_table * t, FILE * f, error * err) {
#if (DEBUG)
	printf("%s offset: %d\n", __FUNCTION__, t->table->offset);
#endif
	if (fseek(f, t->table->offset, SEEK_SET) < 0) { *err = "fseek error"; return; }
	t->format = bread32_0(f, err);
#if (DEBUG)
	printf("%s format: %04X %04X\n", __FUNCTION__, t->format, t->table->format);
#endif
	if (*err) { return; }
	if (t->table->format & PCF_COMPRESSED_METRICS) {
		t->count = (t->table->format & PCF_BYTE_MASK)
			? bread16_1(f, err) : bread16_0(f, err);
#if (DEBUG)
		printf("%s compressed count: %d\n", __FUNCTION__, t->count);
#endif
	} else {
		t->count = (t->table->format & PCF_BYTE_MASK)
			? bread32_1(f, err) : bread32_0(f, err);
#if (DEBUG)
		printf("%s uncompressed count: %d\n", __FUNCTION__, t->count);
#endif
	}
	if (*err) { return; }
}

/**
 * 读取字形。
 */
void pcf_metric_find(metric_table * t, FILE * f, uint32 i, metric_entry * me, error * err) {
#if (DEBUG)
	printf("%s i: %d, count: %d\n", __FUNCTION__, i, t->count);
#endif
	if (i > t->count) { *err = "readMetricEntry: out of range"; return; }
	if (t->table->format & PCF_COMPRESSED_METRICS) {
		if (fseek(f, (uint64)t->table->offset + 6 + (uint64)i * 5, 0) < 0) { *err = "fseek error"; return; }
		uint8 b[5];
		fread(b, sizeof(uint8), sizeof(b), f);
		me->left_sided_bearing = b[0];
		me->left_sided_bearing -= 0x80;
		me->right_side_bearing = b[1];
		me->right_side_bearing -= 0x80;
		me->character_width = b[2];
		me->character_width -= 0x80;
		me->character_ascent = b[3];
		me->character_ascent -= 0x80;
		me->character_descent = b[4];
		me->character_descent -= 0x80;
	} else {
		if (fseek(f, (uint64)t->table->offset + 8 + (uint64)i * 12, 0) < 0) { *err = "fseek error"; return; }
		uint16 b[6];
		fread(b, sizeof(uint16), sizeof(b), f);
		me->left_sided_bearing = b[0];
		me->right_side_bearing = b[1];
		me->character_width = b[2];
		me->character_ascent = b[3];
		me->character_descent = b[4];
		me->character_attributes = b[5];
	}
}

/**
 * 读取字模偏移表。
 */
void pcf_bitmap_load(bitmap_table * t, FILE * f, error * err) {
#if (DEBUG)
	printf("%s offset: %d\n", __FUNCTION__, t->table->offset);
#endif
	if (fseek(f, t->table->offset, 0) < 0) { *err = "fseek error"; return; }
	t->format = bread32_0(f, err);
#if (DEBUG)
	printf("%s format: %04X %04X\n", __FUNCTION__, t->format, t->table->format);
#endif
	if (*err) { return; }
	int32 count = t->count = bread32_1(f, err);
	if (*err) { return; }
	int32 * offsets = t->offsets = malloc(sizeof(int32) * count);
	for (int i = 0; i < count; i++) {
		offsets[i] = bread32_1(f, err);
		if (*err) { return; }
	}
	int32 * bs = t->bitmapSizes;
	for (int i = 0; i < 4; i++) {
		bs[i] = bread32_1(f, err);
		if (*err) { return; }
	}
#if (DEBUG)
	printf("bitmapSizes: %d %d %d %d, %x\n", bs[0], bs[1], bs[2], bs[3], t->format);
#endif
}

/**
 * 读取字模。
 */
void pcf_bitmap_find(bitmap_table * t, FILE * f, uint32 i, bitmap * b, error * err) {
	if (i + 1 > t->count) { *err = "readData: out of range"; return; }
	uint64 off = (uint64)t->table->offset + (uint64)(8 + 4 * t->count + 16);
	off += (uint64)t->offsets[i];
	uint32 size = t->offsets[i + 1] - t->offsets[i];
	if (size < 0) { *err = "readData: invalid offsets"; return; }
	if (fseek(f, off, 0) < 0) { *err = "fseek error"; return; }
	uint8 * data = b->data = malloc(size);
	b->length = size;
	fread(data, size, 1, f);
}

/**
 * 释放字模占用内存。
 */
void pcf_bitmap_free(bitmap * b) {
#if (DEBUG)
	printf("%s: %p\n", __FUNCTION__, b);
#endif
	free(b->data);
}

/**
 * 打开字体文件并读取其目录。
 */
void pcf_open(pf * pf, char * path, error * err) {
	if (pf->init) { return; } // 已经初始化
	FILE * file = fopen(path, "r");
	if (!file) { *err = "file not found"; return; }
	pf->f = file;
	file_header fh;
	fread(&fh, sizeof(fh), 1, file);
#if (DEBUG)
	printf("%02X%02X%02X%02X\n", fh.header[0], fh.header[1], fh.header[2], fh.header[3]);
	printf("tableCount: %d\n", fh.table_count);
#endif
	toc_entry * toc_metrics, * toc_bitmaps, * toc_encoding;
	for (int i = 0; i < fh.table_count; i++) {
		toc_entry * toc = malloc(sizeof(toc_entry)); // minor memory leak
		fread(toc, sizeof(toc_entry), 1, file);
//		printf("toc->type:%d\n", toc->type);
		switch (toc->type) {
		case PCF_METRICS:
			toc_metrics = toc;
		case PCF_BITMAPS:
			toc_bitmaps = toc;
		case PCF_BDF_ENCODINGS:
			toc_encoding = toc;
		}
	}
	if (!toc_metrics || !toc_bitmaps) {
		*err = "metrics or bitmap toc not found";
		printf("%s\n", *err);
		return;
	}
	if (!toc_encoding) {
		*err = "encoding toc not found";
		printf("%s\n", *err);
		return;
	}
	metric_table * mt = pf->metric = malloc(sizeof(metric_table));
	mt->table = toc_metrics;
	pcf_metric_load(mt, file, err);
	if (*err) { return; }
#if (DEBUG)
	printf("metrics: %d\n", mt->count);
#endif
	bitmap_table * bt = pf->bitmap = malloc(sizeof(bitmap_table));
	bt->table = toc_bitmaps;
	pcf_bitmap_load(bt, file, err);
	if (*err) { return; }
#if (DEBUG)
	printf("bitmaps: %d\n", bt->count);
#endif
	encoding_table * et = pf->encoding = malloc(sizeof(encoding_table));
	et->table = toc_encoding;
	pcf_encoding_load(et, file, err);
	if (*err) { return; }
	pf->init = 1;
}

/**
 * 释放字体占用内存。TODO
 */
void pcf_free(pf * pf) {
}

/**
 * 寻找unicode字符对应的字模和字形信息。
 */
void pcf_lookup(pf * pf, uint32 r, bitmap * b, metric_entry * me, error * err) {
	uint32 i = pcf_encoding_find(pf->encoding, r, err);
	if (*err) { return; }
	pcf_metric_find(pf->metric, pf->f, i, me, err);
	if (*err) { return; }
	pcf_bitmap_find(pf->bitmap, pf->f, i, b, err);
	if (*err) { return; }
//	*stride = 4;
}

/**
 * 寻找字模并在命令行显示。
 */
void pcf_dump_ascii(pf * pf, char * path, uint32 r, error * err) {
	pcf_open(pf, path, err);
	if (*err) { return; }

	bitmap b;
	metric_entry me;
//	int32 stride;
	pcf_lookup(pf, r, &b, &me, err);
	if (*err) { return; }
#if (DEBUG)
	printf("lookup: %p %d\n", b.data, b.length);
//	printf("lookup: MetricEntry %p\n", me);
#endif
	for (int i = 0, stride = 4; i < b.length; i += stride) {
		for (int j = 0; j < stride; j++) {
			char bits = b.data[i + j];
			for (int k = 7; k >= 0; k--) {
				if (j * 8 + 7 - k > me.character_width) { continue; }
				if (bits & (1<<k)) {
					printf("\x1b[47m ");
				} else {
					printf("\x1b[0m ");
				}
			}
		}
		printf("\n");
	}
	pcf_bitmap_free(&b);
}
