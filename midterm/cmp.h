/*
 * cmp.h - thứ tự sắp xếp các file.
 */
#ifndef CMP_H
#define CMP_H

#include <stddef.h>
#include <time.h>
#include "ls.h"

/* Sắp xếp mảng theo -t, -S, -r (không làm gì nếu có -f). */
void sort_files(struct fileinfo *list, size_t n);

/* Mốc thời gian được chọn bởi -c / -u (mặc định: lần sửa cuối). */
struct timespec file_time(const struct stat *st);

#endif /* CMP_H */
