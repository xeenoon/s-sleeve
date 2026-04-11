#ifndef ASSET_WRITER_H
#define ASSET_WRITER_H

#include <stdio.h>

int asset_writer_write_prolog(FILE *file);
int asset_writer_write_asset(FILE *file, const char *symbol, const char *tag, const char *text);
int asset_writer_write_epilog(FILE *file);

#endif
