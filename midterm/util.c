/*
 * util.c - các hàm phụ trợ: cấp phát an toàn, chuỗi quyền, định dạng kích thước.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ls.h"
#include "util.h"

#define MIN_BLOCK 512ULL
#define MAX_BLOCK (1024ULL * 1024 * 1024)

void *
xrealloc(void *p, size_t n)
{
	void *q = realloc(p, n);

	if (q == NULL) {
		fprintf(stderr, "%s: out of memory\n", PROGNAME);
		exit(1);
	}
	return q;
}

char *
xstrdup(const char *s)
{
	return strcpy(xrealloc(NULL, strlen(s) + 1), s);
}

/* Ký tự cho ô "thực thi" của một bộ quyền.  Nếu có bit đặc biệt
 * (setuid/setgid/sticky) thì x thành s/t và - thành S/T. */
static char
exec_char(mode_t mode, mode_t xbit, mode_t special, char on, char off)
{
	if (mode & special)
		return (mode & xbit) ? on : off;
	return (mode & xbit) ? 'x' : '-';
}

void
mode_string(mode_t mode, char buf[11])
{
	switch (mode & S_IFMT) {
	case S_IFREG:  buf[0] = '-'; break;
	case S_IFDIR:  buf[0] = 'd'; break;
	case S_IFLNK:  buf[0] = 'l'; break;
	case S_IFBLK:  buf[0] = 'b'; break;
	case S_IFCHR:  buf[0] = 'c'; break;
	case S_IFSOCK: buf[0] = 's'; break;
	case S_IFIFO:  buf[0] = 'p'; break;
#ifdef S_IFWHT
	case S_IFWHT:  buf[0] = 'w'; break;
#endif
	default:       buf[0] = '?'; break;
	}
	buf[1] = (mode & S_IRUSR) ? 'r' : '-';
	buf[2] = (mode & S_IWUSR) ? 'w' : '-';
	buf[3] = exec_char(mode, S_IXUSR, S_ISUID, 's', 'S');
	buf[4] = (mode & S_IRGRP) ? 'r' : '-';
	buf[5] = (mode & S_IWGRP) ? 'w' : '-';
	buf[6] = exec_char(mode, S_IXGRP, S_ISGID, 's', 'S');
	buf[7] = (mode & S_IROTH) ? 'r' : '-';
	buf[8] = (mode & S_IWOTH) ? 'w' : '-';
	buf[9] = exec_char(mode, S_IXOTH, S_ISVTX, 't', 'T');
	buf[10] = '\0';
}

void
humanize(unsigned long long bytes, char *buf, size_t len)
{
	static const char suffix[] = "BKMGTPE";
	double v = (double)bytes;
	int i = 0;

	/* Chia cho 1024 cho đến khi số còn tối đa ba chữ số. */
	while (v >= 999.5 && i < 6) {
		v /= 1024.0;
		i++;
	}
	if (i == 0)
		snprintf(buf, len, "%lluB", bytes);
	else if (v < 9.95)
		snprintf(buf, len, "%.1f%c", v, suffix[i]);	/* 1.5K */
	else
		snprintf(buf, len, "%.0f%c", v, suffix[i]);	/* 23M */
}

/* Phân tích $BLOCKSIZE: một số, có thể kèm hậu tố k, m, g (hoặc b). */
static unsigned long long
blocksize_from_env(void)
{
	const char *env = getenv("BLOCKSIZE");
	char *end;
	unsigned long long n;

	if (env == NULL)
		return MIN_BLOCK;
	n = strtoull(env, &end, 10);
	if (end == env)
		n = 1;
	switch (*end) {
	case 'k': case 'K': n *= 1024; end++; break;
	case 'm': case 'M': n *= 1024 * 1024; end++; break;
	case 'g': case 'G': n *= 1024ULL * 1024 * 1024; end++; break;
	case 'b': case 'B': n *= 512; end++; break;
	default: break;
	}
	if (*end != '\0')
		return MIN_BLOCK;		/* giá trị rác: dùng mặc định */
	if (n < MIN_BLOCK)
		n = MIN_BLOCK;
	if (n > MAX_BLOCK)
		n = MAX_BLOCK;
	return n;
}

unsigned long long
block_unit(void)
{
	if (fl.size == SIZE_KILO)
		return 1024;
	return blocksize_from_env();
}
