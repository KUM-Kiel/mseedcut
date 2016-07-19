#include <stdio.h>
#include <stdlib.h>
#include "options.h"
#include "progress.h"

#define error(...) { \
  fprintf(stderr, __VA_ARGS__); \
  r = 1; \
  goto end; \
}

static const char *program = "mseedcut";
static void usage(const char *opt, const char *arg, int l)
{
  fprintf(stderr, "Usage: %s <MiniSEED File>\n\n"
    "This program cuts a MiniSEED file into pieces of 64MB.\n"
    "The files are named after the original with a three digit\n"
    "number appended to their name.\n\n"
    "Be careful to only use it on MiniSEED files, as normal\n"
    "SEED files have a header, which would end up only in the\n"
    "first file.\n\n"
    "As a second limitation this program only works with files\n"
    "with a blocksize of 4096 bytes, which is the most common\n"
    "for MiniSEED.\n\n"
    "Author: Lukas Joeressen <info@kum-kiel.de>\n"
    "http://www.kum-kiel.de/\n", program);
  exit(1);
}

static int numbered_filename(char *out, int outlen, const char *filename, int number)
{
  int lastslash = -1, lastdot = -1;
  int i = 0;
  while (filename[i]) {
    if (filename[i] == '.') {
      lastdot = i;
    } else if (filename[i] == '/') {
      lastslash = i;
    }
    ++i;
  }
  i = snprintf(out, outlen, "%s", filename);
  if (lastdot > lastslash + 1) {
    i = snprintf(out + lastdot, outlen - lastdot, "-%03d", number);
    snprintf(out + lastdot + i, outlen - lastdot - i, "%s", filename + lastdot);
  } else {
    snprintf(out + i, outlen - i, "-%03d", number);
  }
  return 0;
}

#define BLOCKSIZE (4096)
#define FILESIZE (16384 * BLOCKSIZE)

int main(int argc, char **argv)
{
  FILE *in = 0, *out = 0;
  Progress p = 0;
  char buffer[BLOCKSIZE];
  size_t i = 0, filesize;
  int r = 0, n = 0;
  char str[256];

  /* Parse options. */
  program = argv[0];
  parse_options(&argc, &argv, OPTIONS(
    FLAG_CALLBACK('h', "help", usage)
  ));
  if (argc < 2) usage(0, 0, 0);

  if (!(in = fopen(argv[1], "rb")))
    error("Could not open file '%s'.\n", argv[1]);

  fseek(in, 0, SEEK_END);
  filesize = ftell(in);
  rewind(in);

  if (filesize % BLOCKSIZE)
    error("This file does not consist of 4K blocks.\n");

  if (filesize <= FILESIZE)
    error("This file should be small enough.\n");

  /* Cut the file. */
  p = progress_new(filesize);

  for (i = 0; i < filesize; i += BLOCKSIZE) {
    if (!in || fread(buffer, sizeof(buffer), 1, in) < 1)
      error("Could not read at offset %lld.\n", (long long) i);
    progress_update(p, i);

    if (i % FILESIZE == 0) {
      ++n;
      numbered_filename(str, sizeof(str), argv[1], n);
      if (out) fclose(out);
      if (!(out = fopen(str, "wb")))
        error("Could not open file '%s'.\n", str)
    }

    if (!out || fwrite(buffer, BLOCKSIZE, 1, out) < 1)
      error("Could not write at offset %lld.\n", (long long) i);
  }

end:
  progress_finish(p, i);
  if (in) fclose(in);
  if (out) fclose(out);

  return r;
}
