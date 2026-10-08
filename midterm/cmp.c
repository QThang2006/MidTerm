/*
 * cmp.c - sắp xếp.
 *
 * Mỗi hàm so sánh là một thứ tự toàn phần (nếu bằng nhau thì so theo tên),
 * và -r chỉ việc đổi dấu kết quả.
 */
#include <stdlib.h>
#include <string.h>

#include "cmp.h"

/* Thành viên timespec tên là st_mtim trên Linux/FreeBSD/OpenBSD và
 * st_mtimespec trên NetBSD/macOS. */
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#define TS_M(st) ((st)->st_mtim)
#define TS_C(st) ((st)->st_ctim)
#define TS_A(st) ((st)->st_atim)
#else
#define TS_M(st) ((st)->st_mtimespec)
#define TS_C(st) ((st)->st_ctimespec)
#define TS_A(st) ((st)->st_atimespec)
#endif

struct timespec
file_time(const struct stat *st)
{
	switch (fl.time) {
	case TIME_C:
		return TS_C(st);
	case TIME_A:
		return TS_A(st);
	default:
		return TS_M(st);
	}
}

static int
direction(int result)
{
	return fl.reverse ? -result : result;
}

static int
name_order(const struct fileinfo *a, const struct fileinfo *b)
{
	return strcmp(a->name, b->name);	/* theo thứ tự byte, không theo locale */
}

static int
cmp_by_name(const void *pa, const void *pb)
{
	return direction(name_order(pa, pb));
}

/* File lớn nhất đứng trước. */
static int
cmp_by_size(const void *pa, const void *pb)
{
	const struct fileinfo *a = pa, *b = pb;

	if (a->st.st_size != b->st.st_size)
		return direction(a->st.st_size > b->st.st_size ? -1 : 1);
	return direction(name_order(a, b));
}

/* File mới thay đổi gần nhất đứng trước. */
static int
cmp_by_time(const void *pa, const void *pb)
{
	const struct fileinfo *a = pa, *b = pb;
	struct timespec ta = file_time(&a->st), tb = file_time(&b->st);

	if (ta.tv_sec != tb.tv_sec)
		return direction(ta.tv_sec > tb.tv_sec ? -1 : 1);
	if (ta.tv_nsec != tb.tv_nsec)
		return direction(ta.tv_nsec > tb.tv_nsec ? -1 : 1);
	return direction(name_order(a, b));
}

void
sort_files(struct fileinfo *list, size_t n)
{
	if (fl.unsorted || n < 2)
		return;
	if (fl.sort == BY_SIZE)
		qsort(list, n, sizeof(*list), cmp_by_size);
	else if (fl.sort == BY_TIME)
		qsort(list, n, sizeof(*list), cmp_by_time);
	else
		qsort(list, n, sizeof(*list), cmp_by_name);
}
