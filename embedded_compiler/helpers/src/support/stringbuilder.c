#include "stringbuilder.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

stringbuilder *init_builder()
{
    stringbuilder *sb = malloc(sizeof(stringbuilder));
    sb->allocated = 16; // start small but nonzero
    sb->writtenlen = 0;
    sb->data = malloc(sb->allocated);
    return sb;
}

void free_builder(stringbuilder *b)
{
    if (!b)
        return;
    free(b->data);
    free(b);
}

/* ensure capacity for at least amt more bytes */
static void ensure_capacity(stringbuilder *b, int amt)
{
    if (b->writtenlen + amt <= b->allocated)
        return;

    size_t new_alloc = (b->allocated + amt) * 2; // geometric growth
    char *newdata = realloc(b->data, new_alloc);
    b->data = newdata;
    b->allocated = new_alloc;
}

void append(stringbuilder *b, const char *str)
{
    int len = strlen(str);
    ensure_capacity(b, len);
    memcpy(b->data + b->writtenlen, str, len);
    b->writtenlen += len;
}

/* read entire file into heap-allocated buffer with debug logs */
char *read_file(const char *fullpath, int *out_len)
{
    if (!fullpath)
    {
        return NULL;
    }

    FILE *f = fopen(fullpath, "rb");
    if (!f)
    {
        return NULL;
    }

    if (fseek(f, 0, SEEK_END) != 0)
    {
        fclose(f);
        return NULL;
    }

    long size = ftell(f);
    if (size < 0)
    {
        fclose(f);
        return NULL;
    }

    rewind(f);

    char *buf = (char *)malloc((size_t)size + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, (size_t)size, f);
    if (read_bytes != (size_t)size)
    {
        fclose(f);
        free(buf);
        return NULL;
    }

    fclose(f);
    buf[size] = '\0'; // null-terminate
    if (out_len)
        *out_len = (int)size;

    return buf;
}

void append_bytes(stringbuilder *b, const char *src, int len)
{
    ensure_capacity(b, len);
    memcpy(b->data + b->writtenlen, src, len);
    b->writtenlen += len;
}

void append_byte(stringbuilder *b, char c)
{
    ensure_capacity(b, 1);
    b->data[b->writtenlen++] = c;
}

char *safestr(const char *str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str) + 1;
    char *out = malloc(len);
    strcpy(out, str);
    return out;
}

char *strndup(const char *s, size_t n)
{
    size_t len = 0;
    while (len < n && s[len])
        len++;
    char *copy = malloc(len + 1);
    memcpy(copy, s, len);
    copy[len] = '\0';
    return copy;
}
