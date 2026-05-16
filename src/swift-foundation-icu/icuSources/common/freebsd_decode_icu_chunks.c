/*
 * freebsd-launchd-mach (2026-05-15)
 *
 * Decode swift-foundation-icu's pre-built CLDR data hex chunks
 * (icu_packaged_main_data.{0,1,2,3}.inc.h, ~50 MB hex source each)
 * into one raw binary blob (~40 MB).
 *
 * Why: each .inc.h holds a #define expanding to ~10 million
 * comma-separated 0xXX literals. Compiling the .cpp that
 * concatenates them as a uint8_t[] array peaks at 6-10 GB resident
 * in clang's parser -- AST cost dominates. The build-on-low-end-
 * systems rule pins the CI VM to 8 GB, so this build mode is
 * structurally infeasible.
 *
 * The fix: decode the chunks to a binary file at build time, then
 * .incbin the binary into a tiny .S file. Assembler reads the .bin
 * directly into the .o data section. Compile peak drops to ~150 MB
 * (just the assembler holding 40 MB of bytes).
 *
 * Usage: decode_icu_chunks chunk0.inc.h chunk1.inc.h chunk2.inc.h
 *        chunk3.inc.h output.bin
 *
 * Tiny state machine:
 *   - skip C++ comments (// to end of line)
 *   - skip the #define directive line up to the first 0xXX
 *   - on each "0x" or "0X" prefix, read 1-2 hex digits, write byte
 *   - ignore everything else (whitespace, commas, backslash
 *     line continuations, the macro identifier itself)
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int
decode_one(const char *path, FILE *out)
{
	FILE *in = fopen(path, "r");
	if (!in) {
		perror(path);
		return 1;
	}
	int c;
	size_t bytes_written = 0;
	while ((c = fgetc(in)) != EOF) {
		if (c == '/') {
			int n = fgetc(in);
			if (n == '/') {
				while ((c = fgetc(in)) != EOF && c != '\n')
					;
			} else if (n != EOF) {
				ungetc(n, in);
			}
		} else if (c == '0') {
			int n = fgetc(in);
			if (n == 'x' || n == 'X') {
				int h1 = fgetc(in);
				int h2 = fgetc(in);
				if (isxdigit(h1) && isxdigit(h2)) {
					char buf[3] = { (char)h1, (char)h2, '\0' };
					unsigned long b = strtoul(buf, NULL, 16);
					unsigned char byte = (unsigned char)b;
					if (fwrite(&byte, 1, 1, out) != 1) {
						perror("fwrite");
						fclose(in);
						return 1;
					}
					bytes_written++;
				} else {
					if (h2 != EOF) ungetc(h2, in);
					if (h1 != EOF) ungetc(h1, in);
				}
			} else if (n != EOF) {
				ungetc(n, in);
			}
		}
	}
	fclose(in);
	fprintf(stderr, "decode_icu_chunks: %s -> %zu bytes\n", path, bytes_written);
	return 0;
}

int
main(int argc, char **argv)
{
	if (argc < 3) {
		fprintf(stderr,
		    "usage: %s in1.inc.h [in2.inc.h ...] out.bin\n",
		    argv[0]);
		return 1;
	}
	const char *out_path = argv[argc - 1];
	FILE *out = fopen(out_path, "wb");
	if (!out) {
		perror(out_path);
		return 1;
	}
	for (int i = 1; i < argc - 1; i++) {
		if (decode_one(argv[i], out) != 0) {
			fclose(out);
			return 1;
		}
	}
	fclose(out);
	return 0;
}
