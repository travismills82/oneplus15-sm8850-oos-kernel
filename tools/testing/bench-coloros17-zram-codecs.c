/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Host-only 4K page codec screening. Not a kernel ZRAM, QPACE or power
 * benchmark. Never read device memory or change ZRAM on the handset.
 */
#define _POSIX_C_SOURCE 200809L
#include <lz4.h>
#include <zstd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PAGE_BYTES 4096
#define OUT_BYTES (PAGE_BYTES * 2)

static uint64_t now_ns(void)
{
	struct timespec now;

	if (clock_gettime(CLOCK_MONOTONIC, &now))
		exit(2);
	return (uint64_t)now.tv_sec * 1000000000 + now.tv_nsec;
}

static int benchmark(const unsigned char *input, size_t n, int use_zstd)
{
	unsigned char out[OUT_BYTES], roundtrip[PAGE_BYTES];
	ZSTD_CCtx *enc = use_zstd ? ZSTD_createCCtx() : NULL;
	ZSTD_DCtx *dec = use_zstd ? ZSTD_createDCtx() : NULL;
	uint64_t cns = 0, dns = 0, compressed = 0, pages = 0;
	unsigned int same = 0;

	if (use_zstd && (!enc || !dec))
		return 1;
	for (size_t i = 0; i + PAGE_BYTES <= n; i += PAGE_BYTES) {
		const unsigned char *page = input + i;
		uint64_t begin;
		size_t bytes, recovered;
		int all_zero = 1;

		/* All-zero pages trivially bypass codec work in Linux ZRAM. */
		for (size_t j = 0; j < PAGE_BYTES; j++)
			if (page[j]) {
				all_zero = 0;
				break;
			}
		if (all_zero) {
			same++;
			continue;
		}
		begin = now_ns();
		if (use_zstd)
			bytes = ZSTD_compressCCtx(enc, out, sizeof(out),
						 page, PAGE_BYTES, 1);
		else
			bytes = LZ4_compress_default((const char *)page, (char *)out,
						     PAGE_BYTES, sizeof(out));
		cns += now_ns() - begin;
		if (!bytes || ZSTD_isError(bytes) || bytes > sizeof(out))
			return 1;
		begin = now_ns();
		if (use_zstd)
			recovered = ZSTD_decompressDCtx(dec, roundtrip, PAGE_BYTES, out, bytes);
		else
			recovered = LZ4_decompress_safe((const char *)out, (char *)roundtrip,
						       bytes, PAGE_BYTES);
		dns += now_ns() - begin;
		if (recovered != PAGE_BYTES || memcmp(page, roundtrip, PAGE_BYTES))
			return 1;
		compressed += bytes < PAGE_BYTES ? bytes : PAGE_BYTES;
		pages++;
	}
	ZSTD_freeCCtx(enc);
	ZSTD_freeDCtx(dec);
	if (!pages)
		return 1;
	printf("%s\t%llu\t%u\t%llu\t%.1f\t%.1f\n", use_zstd ? "zstd-1" : "lz4",
	       (unsigned long long)pages, same, (unsigned long long)compressed,
	       (double)cns / pages, (double)dns / pages);
	return 0;
}

int main(int argc, char **argv)
{
	long length;
	unsigned char *data;
	FILE *file;
	int status;

	if (argc != 2) {
		fprintf(stderr, "usage: %s synthetic-or-authorized-corpus\n", argv[0]);
		return 2;
	}
	file = fopen(argv[1], "rb");
	if (!file || fseek(file, 0, SEEK_END) ||
	    (length = ftell(file)) < PAGE_BYTES || length > 256 * 1024 * 1024 ||
	    fseek(file, 0, SEEK_SET))
		return 2;
	data = malloc(length);
	if (!data || fread(data, 1, length, file) != (size_t)length || fclose(file))
		return 2;
	puts("algorithm\t4k_pages\tzero_pages_skipped\tcapped_bytes\tcompress_ns_per_page\tdecompress_ns_per_page");
	status = benchmark(data, length, 0) || benchmark(data, length, 1);
	free(data);
	return status;
}
