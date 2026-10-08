/*
 * util.h - các hàm phụ trợ: cấp phát an toàn, chuỗi quyền, định dạng kích thước.
 */
#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <sys/types.h>

void *xrealloc(void *p, size_t n);
char *xstrdup(const char *s);

/* Giống strmode(3): ghi ví dụ "drwxr-xr-x" (10 ký tự + NUL) vào buf[11]. */
void mode_string(mode_t mode, char buf[11]);

/* Giống humanize_number(3): "512B", "1.5K", "23M". */
void humanize(unsigned long long bytes, char *buf, size_t len);

/* Kích thước (byte) của đơn vị dùng cho -s: 1024 nếu có -k, không thì $BLOCKSIZE hoặc 512. */
unsigned long long block_unit(void);

#endif /* UTIL_H */
