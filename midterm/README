ls - bản cài đặt lại ls(1) cho bài giữa kỳ CS631
================================================

Cài đặt tập con của ls(1) NetBSD được mô tả trong man page đề bài cung cấp:

    ls [-AacdFfhiklnqRrSstuw] [file ...]

BIÊN DỊCH
---------
    $ make          # tạo ./ls (không có cảnh báo với -Wall -Wextra -Wpedantic)
    $ make test     # chạy test.sh (hãy chạy bằng user thường, không phải root)
    $ make clean

CÁC FILE
--------
    ls.c / ls.h        main(), xử lý option (getopt), xử lý operand, đọc thư
                       mục và duyệt cây thư mục (kể cả -R), báo lỗi
    cmp.c / cmp.h      các hàm so sánh để sắp xếp (tên, kích thước, thời gian),
                       chọn mốc thời gian theo -c/-u; mọi sắp xếp đều qua qsort(3)
    print.c / print.h  toàn bộ phần in: dạng thường, dạng dài -l/-n, -i, -s, -F,
                       -q/-w, dòng "total", căn thẳng cột
    util.c / util.h    hàm phụ trợ: xrealloc/xstrdup, bản tương đương của
                       strmode(3) và humanize_number(3), phân tích $BLOCKSIZE
    test.sh            bộ test (xem mục KIỂM THỬ)
    Makefile, checklist, LOG (kết quả git log)

THIẾT KẾ
--------
 * Luồng chạy:  main -> parse_options -> list_operands -> list_dir.
   list_operands() gọi lstat(2) cho từng operand, in các operand không phải
   thư mục thành một danh sách, rồi gọi list_dir() cho từng thư mục.
   list_dir() đọc toàn bộ thư mục vào một mảng `struct fileinfo` (đường dẫn,
   tên hiển thị, struct stat), sắp xếp, in, và đệ quy nếu có -R.  Thư mục
   được đóng (closedir) trước khi đệ quy để cây thư mục sâu không làm cạn
   file descriptor.
 * Mọi option nằm trong một `struct flags`.  Các option xung đột nhau dùng
   chung một thành viên, nên quy tắc "cái đứng cuối thắng" không cần xử lý
   riêng.
 * Sắp xếp: mỗi hàm so sánh là một thứ tự toàn phần (nếu bằng nhau thì so
   theo tên bằng strcmp(3), tức thứ tự byte chứ không theo locale); -r chỉ
   việc đổi dấu kết quả.
 * print_files() chạy hai lượt: lượt đầu định dạng từng trường chỉ để tìm giá
   trị rộng nhất của mỗi cột (và cộng tổng block cho dòng "total"), lượt sau
   in các dòng với độ rộng đó để các cột thẳng hàng.
 * -R không đi theo symbolic link tới thư mục (các entry được lstat), nên
   vòng lặp link không thể gây đệ quy vô hạn.
 * strmode(3) và humanize_number(3) không có sẵn ở mọi nơi (glibc không có),
   nên util.c tự cài đặt bản tương đương; chương trình build được trên Linux,
   NetBSD, FreeBSD và macOS.
 * Option xung đột: -a/-A, -c/-u, -S/-t, -h/-k, -q/-w và -l/-n (một -l đứng
   sau sẽ tắt chế độ số, một -n đứng sau sẽ bật lại).  -A luôn được bật với
   super-user (nhưng -a nêu rõ vẫn thắng).
 * Lỗi (operand không tồn tại, thư mục không đọc được, ...) được báo dạng
   "ls: <đường dẫn>: <strerror>" ra stderr, chương trình vẫn chạy tiếp, và
   exit status là 1.  Option không hợp lệ thì in dòng usage và thoát với
   status 1.
 * Các thông báo của chương trình (usage, thông báo lỗi) được giữ bằng tiếng
   Anh để giống hành vi của ls thật; chỉ phần chú thích và tài liệu là tiếng
   Việt.

CÁCH HIỂU MAN PAGE
------------------
 * -s: số block tính theo đơn vị 512 byte (hoặc $BLOCKSIZE, hoặc 1024 với
   -k), làm tròn lên.  Đúng như man page, dòng "total" của riêng -s chỉ được
   in khi stdout là terminal; với -l thì luôn in (cho thư mục có nội dung).
 * -h: ảnh hưởng tới -s và -l; kích thước có dạng 512B, 1.5K, 23M.  -k và -h
   ghi đè nhau, cái đứng cuối thắng.  $BLOCKSIZE chỉ được xét khi không có
   -h lẫn -k.
 * -f chỉ tắt sắp xếp (không ngầm bật -a vì man page không nói như vậy).
 * Symbolic link ghi trên dòng lệnh không bị đi theo với -d.  Như ls truyền
   thống, chúng cũng không bị đi theo với -l/-n hoặc -F (để `ls -l link` hiện
   chính cái link); các trường hợp còn lại thì đi theo.
 * Ngày trong -l: "Mon DD HH:MM", hoặc "Mon DD  YYYY" với file cũ hơn sáu
   tháng hoặc ở tương lai, giống ls truyền thống.  Đặt OLD_FILES_SHOW_YEAR
   thành 0 trong ls.h để luôn in HH:MM.  $TZ được tuân theo qua localtime(3).
 * -q/-w: mặc định là -q khi ra terminal, -w trong trường hợp khác.  Byte
   không in được được xác định bằng isprint(3), từng byte một.
 * File thiết bị ký tự/khối hiển thị "major, minor" ở trường kích thước.

KIỂM THỬ
--------
test.sh dựng một cây thư mục mẫu trong thư mục tạm, gồm: file thường với
nhiều kích thước và mtime khác nhau, file ẩn, file thực thi, FIFO, symlink
tới file, tới thư mục và symlink hỏng, thư mục lồng nhau, thư mục rỗng, thư
mục setuid và một file có ký tự điều khiển trong tên.  Sau đó script:

 1. so sánh output của chương trình với ls hệ thống (-1) cho: liệt kê thường,
    -a, -A, -d, -F, -i, -q, -r, -S, -t, -u, -c, các tổ hợp của chúng, -R, nhiều
    operand, operand không tồn tại, và -l/-ln/-li/-lS/-lt/-lc/-lu/-ld/-lR trên
    file, symlink và file trong /dev.  Output dạng dài được chuẩn hoá (gộp
    khoảng trắng liên tiếp, bỏ dòng "total") vì GNU ls dùng khoảng trắng khác
    và block 1K;
 2. kiểm tra các hành vi mà ls tham chiếu khác hoặc không diễn đạt được:
    -q/-w và thứ tự ưu tiên, -s với block 512 byte / -k / -h / $BLOCKSIZE,
    định dạng cỡ file của -h, dòng "total" so với ls chạy với POSIXLY_CORRECT,
    thứ tự ưu tiên -n/-l và -a/-A, -lF, đích của symlink, thông báo lỗi, dòng
    usage, exit status, thư mục rỗng, bị từ chối quyền;
 3. kiểm tra -f bằng cách so sánh tập hợp tên (thứ tự không xác định).

Ngoài test.sh, chương trình còn được biên dịch với -fsanitize=address,undefined
và chạy trên các cây thư mục lớn (/usr/include, /usr/bin, /etc với -lRaiFsh)
mà không có báo cáo lỗi nào, và biên dịch sạch với -Wall -Wextra -Wpedantic.

Hãy chạy test bằng user thường: với root thì -A luôn bật, làm output khác
GNU ls, và các test về quyền truy cập không chạy được.

GIỚI HẠN ĐÃ BIẾT
----------------
 * Không có output nhiều cột (man page quy định mỗi entry một dòng).
 * Không có ký hiệu ACL/thuộc tính mở rộng trong chuỗi quyền.
 * Tên file được xử lý như dãy byte; không xử lý multibyte/locale cho -q.
