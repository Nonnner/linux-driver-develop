# linux-driver-develop
Tôi muốn xây dựng một hệ thống chat nhiều người dùng (User A, User B, User C, …) trên Linux, sử dụng lập trình socket TCP theo mô hình client–server.

Yêu cầu hệ thống như sau:

Mỗi người dùng phải đăng nhập bằng username và password trước khi tham gia chat.

Chat server chạy ở user space, chịu trách nhiệm:

Quản lý nhiều kết nối client đồng thời.

Xác thực người dùng.

Gửi và nhận tin nhắn giữa các người dùng.

Việc băm mật khẩu (MD5) và mã hóa/giải mã tin nhắn (AES) không được thực hiện ở user space, mà phải được triển khai trong kernel space thông qua một Linux character device driver.

Chat server giao tiếp với driver bằng các cơ chế ioctl/read/write.

Driver sử dụng Linux Kernel Crypto API để thực hiện thuật toán AES và MD5.

Driver chỉ cung cấp cơ chế mã hóa/băm (mechanism), không xử lý logic ứng dụng (policy).

Client không trực tiếp gọi driver, chỉ giao tiếp với server qua socket.

Hãy:

Đề xuất kiến trúc hệ thống.

Mô tả luồng xác thực và luồng gửi tin nhắn giữa các người dùng.

Cung cấp skeleton code cho:

Chat client

Chat server

Crypto character device driver

Giải thích ngắn gọn lý do thiết kế và lợi ích của việc mã hóa trong kernel.

🔹 PROMPT DÙNG CHO BÁO CÁO / ĐỒ ÁN

Viết phần mô tả hệ thống cho một đồ án xây dựng ứng dụng chat nhiều người dùng dựa trên socket TCP, trong đó xác thực người dùng và bảo mật tin nhắn được thực hiện thông qua Linux device driver. Driver triển khai thuật toán mã hóa AES và thuật toán băm MD5 trong kernel space bằng Linux Kernel Crypto API. Trình bày kiến trúc, chức năng từng thành phần, luồng hoạt động và ưu điểm của giải pháp.

🔹 PROMPT “ĂN ĐIỂM” (NẾU DÙNG AI GEN CODE)

Generate clean, well-commented C code compatible with Linux for a multi-user TCP chat system. Implement a user-space chat server and client, and a kernel-space character device driver providing AES encryption/decryption and MD5 hashing using the Linux Kernel Crypto API. The server must authenticate users and encrypt messages via the driver using ioctl. Follow proper kernel programming practices and separate mechanism (kernel) from policy (user space).
