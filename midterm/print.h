/*
 * print.h - phần in kết quả.
 */
#ifndef PRINT_H
#define PRINT_H

#include <stddef.h>
#include "ls.h"

/* In một tên file, tuân theo -q / -w. */
void print_name(const char *name);

/* In dòng "dir:" (dùng với -R và khi có nhiều operand). */
void print_header(const char *path);

/*
 * In các file trong 'list', mỗi file một dòng, kèm các tuỳ chọn -i -s -l -n
 * -h -k -F nếu có.  'in_dir' được đặt khi danh sách là nội dung một thư mục
 * (khi đó dòng "total" được in nếu cần).
 */
void print_files(const struct fileinfo *list, size_t n, int in_dir);

#endif /* PRINT_H */
