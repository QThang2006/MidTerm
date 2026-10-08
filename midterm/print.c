/*
 * print.c - định dạng kết quả in ra.
 *
 * print_files() chạy hai lượt: lượt đầu định dạng mọi trường chỉ để đo giá
 * trị rộng nhất của mỗi cột, lượt sau in các dòng với độ rộng đó để các cột
 * thẳng hàng.
 */
#include <ctype.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __linux__
#include <sys/sysmacros.h>	/* major(), minor() */
#endif

#include "cmp.h"
#include "print.h"
#include "util.h"

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define FIELD_LEN	64
#define SIX_MONTHS	15778476L	/* giây */

typedef unsigned long long ull;

/* Độ rộng các cột của một danh sách thư mục. */
struct widths {
	size_t inode, blocks, nlink, owner, group, size;
};

/* ------------------------------------------------------------------ */
/* tên file                                                          */
/* ------------------------------------------------------------------ */

void
print_name(const char *name)
{
	for (; *name != '\0'; name++) {
		unsigned char c = (unsigned char)*name;

		putchar(fl.hide_nonprint && !isprint(c) ? '?' : c);
	}
}

void
print_header(const char *path)
{
	print_name(path);
	puts(":");
}

/* Ký tự mà -F thêm vào sau tên. */
static const char *
classify_char(mode_t mode)
{
	if (S_ISDIR(mode))
		return "/";
	if (S_ISLNK(mode))
		return "@";
	if (S_ISSOCK(mode))
		return "=";
	if (S_ISFIFO(mode))
		return "|";
#ifdef S_ISWHT
	if (S_ISWHT(mode))
		return "%";
#endif
	if (mode & (S_IXUSR | S_IXGRP | S_IXOTH))
		return "*";
	return "";
}

/* ------------------------------------------------------------------ */
/* từng trường riêng lẻ, mỗi trường được định dạng vào một buffer nhỏ  */
/* ------------------------------------------------------------------ */

static void
field_inode(const struct stat *st, char *buf)
{
	snprintf(buf, FIELD_LEN, "%llu", (ull)st->st_ino);
}

/* Số block: -h cho dạng "4.0K", còn lại tính theo đơn vị 512 byte, 1024 byte
 * (-k) hoặc $BLOCKSIZE, làm tròn lên.  'blocks512' đếm số block 512 byte. */
static void
format_blocks(ull blocks512, char *buf)
{
	ull unit;

	if (fl.size == SIZE_HUMAN) {
		humanize(blocks512 * 512, buf, FIELD_LEN);
		return;
	}
	unit = block_unit();
	snprintf(buf, FIELD_LEN, "%llu", (blocks512 * 512 + unit - 1) / unit);
}

static void
field_blocks(const struct stat *st, char *buf)
{
	format_blocks((ull)st->st_blocks, buf);
}

static void
field_nlink(const struct stat *st, char *buf)
{
	snprintf(buf, FIELD_LEN, "%lu", (unsigned long)st->st_nlink);
}

static void
field_owner(const struct stat *st, char *buf)
{
	struct passwd *pw = fl.numeric ? NULL : getpwuid(st->st_uid);

	if (pw != NULL)
		snprintf(buf, FIELD_LEN, "%s", pw->pw_name);
	else
		snprintf(buf, FIELD_LEN, "%lu", (unsigned long)st->st_uid);
}

static void
field_group(const struct stat *st, char *buf)
{
	struct group *gr = fl.numeric ? NULL : getgrgid(st->st_gid);

	if (gr != NULL)
		snprintf(buf, FIELD_LEN, "%s", gr->gr_name);
	else
		snprintf(buf, FIELD_LEN, "%lu", (unsigned long)st->st_gid);
}

/* Kích thước tính bằng byte; với file thiết bị thì là "major, minor". */
static void
field_size(const struct stat *st, char *buf)
{
	if (S_ISCHR(st->st_mode) || S_ISBLK(st->st_mode))
		snprintf(buf, FIELD_LEN, "%lu, %lu",
		    (unsigned long)major(st->st_rdev),
		    (unsigned long)minor(st->st_rdev));
	else if (fl.size == SIZE_HUMAN)
		humanize((ull)st->st_size, buf, FIELD_LEN);
	else
		snprintf(buf, FIELD_LEN, "%llu", (ull)st->st_size);
}

static void
field_date(const struct stat *st, char *buf)
{
	struct timespec ts = file_time(st);
	time_t t = ts.tv_sec, now = time(NULL);
	struct tm *tm = localtime(&t);		/* tuân theo $TZ */
	const char *format = "%b %e %H:%M";

#if OLD_FILES_SHOW_YEAR
	if (t < now - SIX_MONTHS || t > now)
		format = "%b %e  %Y";
#endif
	if (tm == NULL || strftime(buf, FIELD_LEN, format, tm) == 0)
		snprintf(buf, FIELD_LEN, "?");
}

static void
widen(size_t *width, const char *s)
{
	size_t len = strlen(s);

	if (len > *width)
		*width = len;
}

/* ------------------------------------------------------------------ */
/* in danh sách                                                      */
/* ------------------------------------------------------------------ */

/* Lượt 1: đo độ rộng các cột và cộng tổng số block. */
static ull
measure(const struct fileinfo *list, size_t n, struct widths *w)
{
	char buf[FIELD_LEN];
	ull total = 0;
	size_t i;

	memset(w, 0, sizeof(*w));
	for (i = 0; i < n; i++) {
		const struct stat *st = &list[i].st;

		total += (ull)st->st_blocks;
		if (fl.inode) {
			field_inode(st, buf);
			widen(&w->inode, buf);
		}
		if (fl.blocks) {
			field_blocks(st, buf);
			widen(&w->blocks, buf);
		}
		if (fl.longfmt) {
			field_nlink(st, buf);
			widen(&w->nlink, buf);
			field_owner(st, buf);
			widen(&w->owner, buf);
			field_group(st, buf);
			widen(&w->group, buf);
			field_size(st, buf);
			widen(&w->size, buf);
		}
	}
	return total;
}

/* In " -> đích" cho symbolic link. */
static void
print_link_target(const struct fileinfo *f)
{
	char target[PATH_MAX + 1];
	ssize_t len = readlink(f->path, target, PATH_MAX);

	if (len < 0)
		return;
	target[len] = '\0';
	fputs(" -> ", stdout);
	print_name(target);
}

/* Lượt 2: in một dòng. */
static void
print_one(const struct fileinfo *f, const struct widths *w)
{
	const struct stat *st = &f->st;
	char buf[FIELD_LEN];

	if (fl.inode) {
		field_inode(st, buf);
		printf("%*s ", (int)w->inode, buf);
	}
	if (fl.blocks) {
		field_blocks(st, buf);
		printf("%*s ", (int)w->blocks, buf);
	}
	if (fl.longfmt) {
		char mode[11], nlink[FIELD_LEN], owner[FIELD_LEN];
		char group[FIELD_LEN], size[FIELD_LEN];

		mode_string(st->st_mode, mode);
		field_nlink(st, nlink);
		field_owner(st, owner);
		field_group(st, group);
		field_size(st, size);
		field_date(st, buf);
		printf("%s  %*s %-*s  %-*s  %*s %s ", mode,
		    (int)w->nlink, nlink, (int)w->owner, owner,
		    (int)w->group, group, (int)w->size, size, buf);
	}

	print_name(f->name);
	if (fl.classify)
		fputs(classify_char(st->st_mode), stdout);
	if (fl.longfmt && S_ISLNK(st->st_mode))
		print_link_target(f);
	putchar('\n');
}

void
print_files(const struct fileinfo *list, size_t n, int in_dir)
{
	struct widths w;
	char buf[FIELD_LEN];
	ull total;
	size_t i;

	if (n == 0)
		return;

	total = measure(list, n, &w);

	/* "total" luôn in với -l; với -s thì chỉ in khi stdout là terminal. */
	if (in_dir && (fl.longfmt || (fl.blocks && isatty(STDOUT_FILENO)))) {
		format_blocks(total, buf);
		printf("total %s\n", buf);
	}

	for (i = 0; i < n; i++)
		print_one(&list[i], &w);
}
