#!/bin/sh
#
# test.sh - so sánh ./ls với ls(1) của hệ thống trên một cây thư mục mẫu.
#
# ls của hệ thống (REF) có thể là GNU ls, có dạng -l khác về khoảng trắng và
# -s/-f hơi khác, nên các output đó được chuẩn hoá trước khi so sánh
# (xem hàm 'norm').
#
set -u
LC_ALL=C; export LC_ALL

if [ "$(id -u)" = 0 ]; then
	echo "test.sh: hãy chạy bằng user thường (với root thì -A luôn bật," >&2
	echo "làm output khác với ls tham chiếu)." >&2
	exit 2
fi

MINE="$(pwd)/ls"
REF="${REF:-/bin/ls}"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/lstest.XXXXXX")"
PASS=0
FAIL=0
trap 'rm -rf "$TMP"' EXIT

# ---------------------------------------------------------- thư mục mẫu
F="$TMP/fx"
mkdir -p "$F/dir1/sub" "$F/dir2" "$F/empty"
cd "$F" || exit 1
echo hello        > a.txt
echo "hello world, longer" > B.txt
dd if=/dev/zero of=big bs=1024 count=300 2>/dev/null
dd if=/dev/zero of=mid bs=1024 count=3 2>/dev/null
: > zero
echo '#!/bin/sh' > script.sh; chmod 755 script.sh
echo x > .hidden
echo x > dir1/f1; echo xx > dir1/f2; echo x > dir1/sub/deep
echo x > dir1/.dot
ln -s a.txt link
ln -s dir1 dlink
ln -s nowhere dangling
mkfifo fifo
touch -d '2020-01-01 10:00' old
touch -d '2024-03-04 12:30' dir1/f1
touch -d '2023-05-06 07:08' a.txt
touch -d '2022-02-02 02:02' B.txt
touch -d '2021-01-01 01:01' big
touch -d '2025-01-01 01:01' mid
chmod 4755 dir2 2>/dev/null
printf 'x' > "$(printf 'we\001ird')"

norm() { tr -s ' ' | sed '/^total /d'; }

check() {	# check <mô tả> <chuẩn hoá 0|1> <tham số...>
	desc="$1"; n="$2"; shift 2
	if [ "$n" = 1 ]; then
		"$MINE" "$@" 2>&1 | norm > "$TMP/mine"
		"$REF" -1 "$@" 2>&1 | norm > "$TMP/ref"
	else
		"$MINE" "$@" 2>/dev/null > "$TMP/mine"
		"$REF" -1 "$@" 2>/dev/null > "$TMP/ref"
	fi
	if cmp -s "$TMP/mine" "$TMP/ref"; then
		PASS=$((PASS + 1))
	else
		FAIL=$((FAIL + 1))
		echo "LỖI: $desc: ls $*"
		diff "$TMP/ref" "$TMP/mine" | head -8
	fi
}

# ------------------------------------------------ liệt kê thường / file ẩn
check "mặc định"           0
check "-a"                 0 -a
check "-A"                 0 -A
check "-aA"                0 -aA
check "nhiều operand"           0 a.txt B.txt dir1 empty
check "operand không tồn tại"    0 nosuchfile a.txt
check "-d dir"             0 -d dir1 empty
check "-d dlink"           0 -d dlink
check "dlink (đi theo link)"   0 dlink
check "-F"                 0 -F
check "-F -a"              0 -Fa
check "-i"                 0 -i
check "-q"                 0 -q
# ------------------------------------------------------------- sắp xếp
check "-r"                 0 -r
check "-S"                 0 -S
check "-Sr"                0 -Sr
check "-t"                 0 -t
check "-tr"                0 -tr
check "-S thắng -t"         0 -tS
check "-t thắng -S"         0 -St
check "-t với operand"        0 -t a.txt B.txt big mid
check "-u"                 0 -tu
check "-c"                 0 -tc
# ------------------------------------------------------------- đệ quy
check "-R"                 0 -R
check "-Ra"                0 -Ra
check "-R dir1"            0 -R dir1
check "-Rd"                0 -Rd
check "nhiều thư mục"      0 dir1 dir2 empty
check "-R kèm -t"       0 -Rt
# --------------------------------------------------------- dạng dài (-l)
check "-l"                 1 -l
check "-la"                1 -la
check "-ln"                1 -ln
check "-li"                1 -li
check "-lS"                1 -lS
check "-lt"                1 -lt
check "-lc"                1 -lc
check "-lu"                1 -lu
check "-l dlink"           1 -l dlink
check "-l dlink/"          1 -l dlink/
check "-ld"                1 -ld dlink dir1
check "-lR"                1 -lR
check "-l dev"             1 -l /dev/null /dev/zero
check "-l bit đặc biệt"    1 -ld dir2 /tmp

# ------------------------------ những thứ ls hệ thống không so sánh được
t() {	# t <mô tả> <kết quả mong đợi> <lệnh...>
	desc="$1"; exp="$2"; shift 2
	got="$("$@" 2>&1)"
	if [ "$got" = "$exp" ]; then
		PASS=$((PASS + 1))
	else
		FAIL=$((FAIL + 1))
		echo "LỖI: $desc"; echo "  mong đợi: $exp"; echo "  thực tế:  $got"
	fi
}

t "-q thay ký tự điều khiển"  "we?ird"        sh -c "'$MINE' -q | grep ird"
t "-w giữ nguyên byte"         "we$(printf '\001')ird" sh -c "'$MINE' -w | grep ird"
t "-w thắng -q"            "we$(printf '\001')ird" sh -c "'$MINE' -qw | grep ird"
t "-q thắng -w"            "we?ird"        sh -c "'$MINE' -wq | grep ird"
t "-s -k của 'big'"            "300 big"       sh -c "'$MINE' -sk big"
t "-s block 512 byte"              "600 big"       sh -c "'$MINE' -s big"
t "BLOCKSIZE=1k"               "300 big"       sh -c "BLOCKSIZE=1k '$MINE' -s big"
t "-h thắng -k"            "300K big"      sh -c "'$MINE' -skh big"
t "-k thắng -h"            "300 big"       sh -c "'$MINE' -shk big"
t "-lh cỡ lớn"                  "300K"          sh -c "'$MINE' -lh big | awk '{print \$5}'"
t "-lh cỡ nhỏ"                  "3.0K"          sh -c "'$MINE' -lh mid | awk '{print \$5}'"
t "-lh cỡ 0"                   "0B"            sh -c "'$MINE' -lh zero | awk '{print \$5}'"
t "-l total (block 512)"      "$(POSIXLY_CORRECT=1 "$REF" -l | head -1)" sh -c "'$MINE' -l | head -1"
t "-s: total chỉ khi ra tty"       ""              sh -c "'$MINE' -s empty"
t "thư mục rỗng -l: không in gì" ""             "$MINE" -l empty
t "exit status khi thành công"             "0"             sh -c "'$MINE' >/dev/null; echo \$?"
t "exit status khi thiếu file"        "1"             sh -c "'$MINE' nosuch >/dev/null 2>&1; echo \$?"
t "exit status khi sai option"     "1"             sh -c "'$MINE' -Z >/dev/null 2>&1; echo \$?"
t "thông báo usage" "usage: ls [-AacdFfhiklnqRrSstuw] [file ...]" sh -c "'$MINE' -Z 2>&1 | tail -1"
t "thông báo lỗi" "ls: nosuch: No such file or directory" "$MINE" nosuch
t "hiện đích của symlink"       "link -> a.txt" sh -c "'$MINE' -l link | sed 's/.* link/link/'"
t "-lF với symlink"                "link@ -> a.txt" sh -c "'$MINE' -lF link | sed 's/.* link/link/'"

# thư mục không đọc được (bỏ qua khi chạy bằng root)
if [ "$(id -u)" != 0 ]; then
	mkdir "$F/locked"; chmod 000 "$F/locked"
	t "bị từ chối quyền" "ls: locked: Permission denied" "$MINE" locked
	t "exit status khi bị từ chối quyền" "1" sh -c "'$MINE' locked >/dev/null 2>&1; echo \$?"
	chmod 755 "$F/locked"
fi

# -f: thứ tự là thứ tự readdir trả về; chỉ so sánh tập hợp tên
# (GNU ls ngầm bật -a với -f, ls này thì không, nên dùng -fa ở đây).
"$MINE" -fa | sort > "$TMP/m"; "$REF" -1f | sort > "$TMP/r"
if cmp -s "$TMP/m" "$TMP/r"; then PASS=$((PASS + 1)); else FAIL=$((FAIL + 1)); echo "LỖI: -fa"; fi
t "-f không hiện file ẩn"  "0"  sh -c "'$MINE' -f | grep -c '^\\.'"

# -n và -l: cái đứng cuối thắng.
t "-n hiện ID dạng số" "$(id -u)" sh -c "'$MINE' -n a.txt | awk '{print \$3}'"
t "-nl hiện tên"      "$(id -un)" sh -c "'$MINE' -nl a.txt | awk '{print \$3}'"
t "-ln hiện ID số"        "$(id -u)" sh -c "'$MINE' -ln a.txt | awk '{print \$3}'"
t "-lF đánh dấu symlink"    "link@ -> a.txt" sh -c "'$MINE' -lF link | sed 's/.* link/link/'"
t "-A sau -a"          "0"   sh -c "'$MINE' -aA | grep -c '^\\.\\.\\?\$'"
t "-a sau -A"          "2"   sh -c "'$MINE' -Aa | grep -c '^\\.\\.\\?\$'"

echo "đạt: $PASS, lỗi: $FAIL"
[ "$FAIL" -eq 0 ]
