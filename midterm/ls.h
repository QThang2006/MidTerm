/*
 * ls.h - các kiểu dữ liệu và biến toàn cục dùng chung cho mọi file nguồn.
 */
#ifndef LS_H
#define LS_H

#include <sys/types.h>
#include <sys/stat.h>

#define PROGNAME "ls"
#define OPTSTRING "AacdFfhiklnqRrSstuw"

/* 1: hiện năm thay cho HH:MM với file cũ hơn 6 tháng (giống ls truyền thống).
 * 0: luôn hiện HH:MM, đúng từng chữ như man page. */
#define OLD_FILES_SHOW_YEAR 1

enum sort_by   { BY_NAME, BY_TIME, BY_SIZE };
enum time_kind { TIME_M, TIME_C, TIME_A };
enum size_kind { SIZE_BLOCKSIZE, SIZE_KILO, SIZE_HUMAN };

/*
 * Tất cả các cờ trên dòng lệnh.  Các cờ xung đột nhau (-c/-u, -S/-t, -h/-k,
 * -q/-w, -a/-A, -l/-n) dùng chung một biến, nên cờ đứng cuối tự động thắng.
 */
struct flags {
	int all;		/* -a: hiện cả . và .. và file ẩn */
	int almost;		/* -A: hiện file ẩn nhưng không hiện . và .. */
	int dir_as_file;	/* -d: thư mục được liệt kê như file thường */
	int classify;		/* -F: thêm ký hiệu / * @ = | sau tên */
	int unsorted;		/* -f: không sắp xếp */
	int inode;		/* -i: in số inode */
	int blocks;		/* -s: in số block */
	int longfmt;		/* -l hoặc -n: dạng liệt kê dài */
	int numeric;		/* -n: hiện UID/GID dạng số */
	int recursive;		/* -R: liệt kê đệ quy */
	int reverse;		/* -r: đảo ngược thứ tự sắp xếp */
	int hide_nonprint;	/* -q (1): ký tự không in được thành '?'; -w (0): giữ nguyên */
	enum sort_by   sort;	/* -S, -t */
	enum time_kind time;	/* -c, -u */
	enum size_kind size;	/* -h, -k */
};

/* Một file cần liệt kê: đường dẫn, tên hiển thị và metadata (struct stat).
 * 'name' trỏ vào bên trong 'path'; chỉ 'path' được cấp phát trên heap. */
struct fileinfo {
	char *path;
	const char *name;
	struct stat st;
};

extern struct flags fl;
extern int exit_status;

/* In "ls: <what>: <strerror(err)>" ra stderr và đặt exit status = 1. */
void warn_errno(const char *what, int err);

#endif /* LS_H */
