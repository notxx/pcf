typedef unsigned long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef signed int int32;
typedef signed short int16;
typedef signed char int8;
typedef char * error;

typedef struct bitmap {
	uint8 * data;
	uint32 length;
} bitmap;

typedef struct tocEntry {
	uint32 type;	/* See below, indicates which table */
	uint32 format;	/* See below, indicates how the data are formatted in the table */
	uint32 size;	/* In bytes */
	uint32 offset;	/* from start of file */
} toc_entry;

typedef struct fileHeader {
	char header[4];	/* always "\1fcp" */
	int table_count;
} file_header;

typedef struct metricEntry {
	int16 left_sided_bearing;
	int16 right_side_bearing;
	int16 character_width;
	int16 character_ascent;
	int16 character_descent;
	uint16 character_attributes;
} metric_entry;

typedef struct metricTable {
	toc_entry * table;
	int32 format;
	int32 count;
} metric_table;

typedef struct bitmapTable {
	toc_entry * table;
	int32 format;
	int32 count;
	int32 * offsets;
	int32 bitmapSizes [4];
} bitmap_table;

typedef struct encodingTable {
	toc_entry * table;
	int32 format;
	int16 minCharOrByte2;
	int16 maxCharOrByte2;
	int16 minByte1;
	int16 maxByte1;
	int16 defChar;
	int16 * indexes;
	int32 index_count;
	uint8 direct; // 编码空间与字形/字模空间为直接映射
} encoding_table;

typedef struct pf {
	encoding_table * encoding;
	bitmap_table * bitmap;
	metric_table *metric;
	FILE * f;
	uint8 init;
} pf;

/**
 * 打开字体文件并读取其目录。
 */
void pcf_open(pf * pf, char * path, error * err);

/**
 * 寻找unicode字符对应的字模和字形信息。
 */
void pcf_lookup(pf * pf, uint32 r, bitmap * b, metric_entry * me, error * err);

/**
 * 释放字模占用内存。
 */
void pcf_bitmap_free(bitmap * b);

/**
 * 释放字体占用内存。
 */
void pcf_free(pf * pf);

/**
 * 寻找字模并在命令行显示。
 */
void pcf_dump_ascii(pf * pf, char * path, uint32 r, error * err);
