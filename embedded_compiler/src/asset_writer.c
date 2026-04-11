#include "asset_writer.h"

#include <string.h>

#include "log.h"

int asset_writer_write_prolog(FILE *file) {
  if (file == NULL) {
    return 1;
  }

  LOG_TRACE("asset_writer_write_prolog\n");
  return fprintf(file,
                 "#include <Arduino.h>\n"
                 "#include \"web_page.h\"\n\n") < 0;
}

int asset_writer_write_asset(FILE *file, const char *symbol, const char *tag, const char *text) {
  if (file == NULL || symbol == NULL || tag == NULL || text == NULL) {
    return 1;
  }

  LOG_TRACE("asset_writer_write_asset symbol=%s tag=%s bytes=%zu\n", symbol, tag, strlen(text));
  if (fprintf(file, "const char %s[] PROGMEM = R\"%s(\n%s\n)%s\";\n\n", symbol, tag, text, tag) < 0) {
    return 1;
  }

  return 0;
}

int asset_writer_write_epilog(FILE *file) {
  if (file == NULL) {
    return 1;
  }

  LOG_TRACE("asset_writer_write_epilog\n");
  return fflush(file) != 0;
}
