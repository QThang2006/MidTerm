/*
 * ls.c - bản cài đặt lại ls(1): xử lý option và duyệt thư mục.
 *
 * Luồng chạy:
 *   main() -> parse_options() -> list_operands()
 *     list_operands(): stat từng operand; in các file không phải thư mục
 *                      thành một danh sách, rồi gọi list_dir() cho từng thư mục
 *     list_dir():      đọc thư mục, sắp xếp, in, và đệ quy nếu có -R
 */
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "cmp.h"
#include "ls.h"
#include "print.h"
#include "util.h"

struct flags fl;
int exit_status = 0;

/* Các danh sách thư mục được ngăn cách bằng một dòng trống. */
static int blank_line_pending = 0;

static void
usage(void)
{
	fprintf(stderr, "usage: %s [-%s] [file ...]\n", PROGNAME, OPTSTRING);
	exit(1);
}

void
warn_errno(const char *what, int err)
{
	fflush(stdout);		/* giữ đúng thứ tự giữa stdout và stderr */
	fprintf(stderr, "%s: %s: %s\n", PROGNAME, what, strerror(err));
	exit_status = 1;
}

static void
parse_options(int argc, char **argv)
{
	int ch;

	/* Ký tự không in được: hiện '?' khi ra terminal, còn lại giữ nguyên. */
	fl.hide_nonprint = isatty(STDOUT_FILENO);

	while ((ch = getopt(argc, argv, OPTSTRING)) != -1) {
		switch (ch) {
		case 'A': fl.almost = 1; fl.all = 0; break;
		case 'a': fl.all = 1; fl.almost = 0; break;
		case 'c': fl.time = TIME_C; break;
		case 'd': fl.dir_as_file = 1; break;
		case 'F': fl.classify = 1; break;
		case 'f': fl.unsorted = 1; break;
		case 'h': fl.size = SIZE_HUMAN; break;
		case 'i': fl.inode = 1; break;
		case 'k': fl.size = SIZE_KILO; break;
		case 'l': fl.longfmt = 1; fl.numeric = 0; break;
		case 'n': fl.longfmt = 1; fl.numeric = 1; break;
		case 'q': fl.hide_nonprint = 1; break;
		case 'R': fl.recursive = 1; break;
		case 'r': fl.reverse = 1; break;
		case 'S': fl.sort = BY_SIZE; break;
		case 's': fl.blocks = 1; break;
		case 't': fl.sort = BY_TIME; break;
		case 'u': fl.time = TIME_A; break;
		case 'w': fl.hide_nonprint = 0; break;
		default:  usage();
		}
	}

	/* -A luôn được bật với super-user (nhưng -a nêu rõ vẫn thắng). */
	if (geteuid() == 0 && !fl.all)
		fl.almost = 1;
}

static int
is_dot_or_dotdot(const char *name)
{
	return strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

/* Entry trong thư mục có tên này có được liệt kê không? */
static int
is_shown(const char *name)
{
	if (name[0] != '.' || fl.all)
		return 1;
	return fl.almost && !is_dot_or_dotdot(name);
}

/* "dir" + "name" = "dir/name"; *offset là vị trí bắt đầu của "name". */
static char *
join_path(const char *dir, const char *name, size_t *offset)
{
	size_t dirlen = strlen(dir);
	int need_slash = dirlen > 0 && dir[dirlen - 1] != '/';
	size_t size = dirlen + need_slash + strlen(name) + 1;
	char *path = xrealloc(NULL, size);

	snprintf(path, size, "%s%s%s", dir, need_slash ? "/" : "", name);
	*offset = dirlen + need_slash;
	return path;
}

/* Đọc mọi entry của 'path' vào một mảng mới cấp phát; trả NULL nếu lỗi. */
static struct fileinfo *
read_dir(const char *path, size_t *count)
{
	DIR *dp = opendir(path);
	struct dirent *de;
	struct fileinfo *list = NULL;
	size_t n = 0, cap = 0;

	*count = 0;
	if (dp == NULL) {
		warn_errno(path, errno);
		return NULL;
	}

	errno = 0;
	while ((de = readdir(dp)) != NULL) {
		struct fileinfo *f;
		size_t offset;

		if (!is_shown(de->d_name))
			continue;
		if (n == cap) {
			cap = cap ? 2 * cap : 64;
			list = xrealloc(list, cap * sizeof(*list));
		}
		f = &list[n];
		f->path = join_path(path, de->d_name, &offset);
		f->name = f->path + offset;
		if (lstat(f->path, &f->st) != 0) {
			warn_errno(f->path, errno);
			free(f->path);
			continue;
		}
		n++;
	}
	if (errno != 0)
		warn_errno(path, errno);
	closedir(dp);

	*count = n;
	return list;
}

static void
free_files(struct fileinfo *list, size_t n)
{
	size_t i;

	for (i = 0; i < n; i++)
		free(list[i].path);
	free(list);
}

static void
list_dir(const char *path, int with_header)
{
	struct fileinfo *list;
	size_t n, i;

	if (blank_line_pending)
		putchar('\n');
	blank_line_pending = 1;
	if (with_header)
		print_header(path);

	if ((list = read_dir(path, &n)) == NULL)
		return;

	sort_files(list, n);
	print_files(list, n, 1);

	/* -R: đi vào các thư mục con.  Chúng được lstat nên symbolic link tới
	 * thư mục không bị đi theo và không thể gây vòng lặp vô hạn. */
	if (fl.recursive) {
		for (i = 0; i < n; i++) {
			if (S_ISDIR(list[i].st.st_mode) &&
			    !is_dot_or_dotdot(list[i].name))
				list_dir(list[i].path, 1);
		}
	}
	free_files(list, n);
}

/*
 * Operand: những cái không phải thư mục (hoặc tất cả, nếu có -d) được in
 * trước thành một danh sách, sau đó liệt kê từng thư mục.  Hai nhóm được
 * sắp xếp riêng.
 */
static void
list_operands(int argc, char **argv)
{
	struct fileinfo *files, *dirs;
	size_t nfiles = 0, ndirs = 0, i;
	int a;
	/* Như ls truyền thống: symbolic link ghi trên dòng lệnh sẽ được đi theo,
	 * trừ khi có -d, -l (-n) hoặc -F. */
	int follow = !(fl.dir_as_file || fl.longfmt || fl.classify);
	int with_header = fl.recursive || argc > 1;

	files = xrealloc(NULL, argc * sizeof(*files));
	dirs = xrealloc(NULL, argc * sizeof(*dirs));

	for (a = 0; a < argc; a++) {
		struct fileinfo f;
		struct stat target;

		f.path = xstrdup(argv[a]);
		f.name = f.path;
		if (lstat(f.path, &f.st) != 0) {
			warn_errno(argv[a], errno);
			free(f.path);
			continue;
		}
		if (follow && S_ISLNK(f.st.st_mode) && stat(f.path, &target) == 0)
			f.st = target;

		if (S_ISDIR(f.st.st_mode) && !fl.dir_as_file)
			dirs[ndirs++] = f;
		else
			files[nfiles++] = f;
	}

	sort_files(files, nfiles);
	print_files(files, nfiles, 0);
	if (nfiles > 0)
		blank_line_pending = 1;

	sort_files(dirs, ndirs);
	for (i = 0; i < ndirs; i++)
		list_dir(dirs[i].path, with_header);

	free_files(files, nfiles);
	free_files(dirs, ndirs);
}

int
main(int argc, char **argv)
{
	char dot[] = ".";
	char *default_operand[] = { dot, NULL };

	parse_options(argc, argv);
	argc -= optind;
	argv += optind;
	if (argc == 0) {
		argc = 1;
		argv = default_operand;
	}

	list_operands(argc, argv);

	if (fflush(stdout) != 0 || ferror(stdout)) {
		fprintf(stderr, "%s: write error: %s\n", PROGNAME,
		    strerror(errno));
		exit_status = 1;
	}
	return exit_status;
}
