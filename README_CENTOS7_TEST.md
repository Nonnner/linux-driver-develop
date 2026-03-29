# README_CENTOS7_TEST

Huong dan test end-to-end cho nguoi dung CentOS 7 lan dau mo terminal.

Muc tieu: setup nhanh, build dung, test day du chat + crypto driver + USB keyboard monitor.

## 0) Dieu kien dau vao

- Ban da co may/VM `CentOS Linux 7 (Core)`.
- Co quyen `sudo`.
- Co ket noi internet de cai package.
- Neu dung VMware va test ban phim USB: da bat USB passthrough.

## 1) Pre-flight check (bat buoc)

Chay tung lenh:

```bash
cat /etc/os-release
uname -r
whoami
sudo -v
```

Ky vong:
- `VERSION="7 (Core)"`
- Kernel co dang `3.10.x` (hoac kernel tuong thich CentOS 7).
- `sudo -v` khong bao loi.

Neu `sudo -v` loi: can cap quyen sudo truoc khi tiep tuc.

## 2) Cai moi truong build tren CentOS 7

### 2.1 Cai bo cong cu bien dich

```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y epel-release
sudo yum install -y git make gcc elfutils-libelf-devel kmod util-linux usbutils bc
```

### 2.2 Cai kernel-devel va kernel-headers dung version

```bash
uname -r
sudo yum install -y kernel-devel-$(uname -r) kernel-headers-$(uname -r)
```

### 2.3 Xac minh version khop 100%

```bash
uname -r
rpm -q kernel-devel kernel-headers
ls -ld /lib/modules/$(uname -r)/build
```

Neu khong khop version `kernel-devel` voi `uname -r`, dung lai va sua ngay:

```bash
sudo yum clean all
sudo yum reinstall -y kernel-devel-$(uname -r) kernel-headers-$(uname -r)
```

## 3) Lay source code

Neu da push len Git:

```bash
cd ~
git clone <REPO_URL>
cd linux-driver-develop
```

Neu thu muc co ten khac, thay dung ten khi `cd`.

Kiem tra nhanh:

```bash
ls -1
```

Can thay cac file chinh: `Makefile`, `Makefile.driver`, `chat_server.c`, `chat_client.c`, thu muc `driver/`.

## 4) Build toan bo project

### 4.1 Build user-space chat app

```bash
make clean
make
ls -lh chat_server chat_client
```

Ky vong:
- Co file `chat_server`, `chat_client`.
- Khong co loi compile.

### 4.2 Build kernel modules

```bash
make driver-clean
make driver-build
ls -lh *.ko
```

Ky vong:
- Co `crypto_driver.ko`
- Co `usb_kbd_monitor.ko`

Neu loi build module:
1. Kiem tra lai Muc 2.3.
2. Chay lai `make driver-clean && make driver-build`.
3. Neu van loi, xem log day du:

```bash
make -f Makefile.driver
```

## 5) Test module USB keyboard monitor (phase driver)

Mo 2 terminal:

- Terminal A: load/unload module
- Terminal B: theo doi kernel log

### 5.1 Terminal A - load module keyboard

```bash
make driver-load-kbd
lsmod | grep usb_kbd_monitor
```

Ky vong co dong `usb_kbd_monitor` trong `lsmod`.

### 5.2 Terminal B - watch log

```bash
dmesg -w | grep usb_kbd_monitor
```

Ky vong khi module khoi dong:
- `usb_kbd_monitor: initializing ...`
- `usb_kbd_monitor: input handler registered ...`

### 5.3 Cam USB keyboard va nhan phim

Ky vong trong Terminal B:
- `connected to device ...`
- `keycode=... state=PRESSED`
- `keycode=... state=RELEASED`

Neu khong thay event:
1. Kiem tra USB passthrough trong VMware.
2. Kiem tra keyboard da vao guest:

```bash
lsusb
cat /proc/bus/input/devices | less
```

## 6) Test crypto driver + chat flow (coexistence)

Mo 4 terminal de test ro rang:

- Terminal 1: chat server
- Terminal 2: chat client A
- Terminal 3: chat client B
- Terminal 4: dmesg watcher (module keyboard)

### 6.1 Load crypto driver

```bash
make driver-load
lsmod | grep crypto_driver
ls -l /dev/crypto_dev
```

Ky vong:
- Thay `crypto_driver` trong `lsmod`.
- Co `/dev/crypto_dev`.

Neu bi permission denied khi chat_server mo device:

```bash
sudo chmod 666 /dev/crypto_dev
```

### 6.2 Chay chat server (Terminal 1)

```bash
./chat_server
```

Ky vong:
- Server listen cong `8888`.
- Khoi tao crypto driver thanh cong.

### 6.3 Chay chat client A (Terminal 2)

```bash
./chat_client
```

Dang nhap user mau:
- Username: `alice`
- Password: `password123`

Gui message thu:
- `hello from alice`

### 6.4 Chay chat client B (Terminal 3)

```bash
./chat_client
```

Dang nhap user mau:
- Username: `bob`
- Password: `password456`

Test broadcast:
- Client A gui `hello all`
- Client B nhan duoc message.

Test private message:
- Client A gui: `/msg bob hi bob private`
- Client B nhan duoc private message.

### 6.5 Xac nhan keyboard module van log song song (Terminal 4)

Trong luc chat dang hoat dong, nhan phim tren USB keyboard.
Ky vong van thay `EV_KEY` log deu trong `dmesg`.

=> Neu chat + keyboard log deu on dinh: pass coexistence.

## 7) Test Web UI (tuy chon)

Luu y: CentOS 7 co the khong co Python 3.12, nhung van co the test web voi python3 co san neu dependency phu hop.

### 7.1 Cai python3 neu thieu

```bash
python3 --version || sudo yum install -y python3 python3-pip
```

### 7.2 Chay web backend

```bash
make backend
make run-web
```

Mo browser trong VM (hoac host neu co port forwarding):
- `http://localhost:5000`

## 8) Cleanup sau test

### 8.1 Dung process chat/web

Nhan `Ctrl+C` tai terminal dang chay `chat_server` va `make run-web`.

Neu con process treo cang:

```bash
pkill -f chat_server || true
pkill -f chat_backend.py || true
```

### 8.2 Unload modules

```bash
make driver-unload-kbd
make driver-unload
```

Kiem tra da sach:

```bash
lsmod | grep -E "usb_kbd_monitor|crypto_driver" || echo "Modules unloaded"
```

## 9) Troubleshooting nhanh

### Loi 1: `insmod ... Exec format error`
Nguyen nhan thuong gap: kernel-devel khong khop kernel dang chay.

Fix:
```bash
uname -r
rpm -q kernel-devel
sudo yum reinstall -y kernel-devel-$(uname -r) kernel-headers-$(uname -r)
make driver-clean && make driver-build
```

### Loi 2: `Failed to open /dev/crypto_dev` hoac permission denied

Fix:
```bash
ls -l /dev/crypto_dev
sudo chmod 666 /dev/crypto_dev
```

### Loi 3: `Address already in use` (port 8888)

Fix:
```bash
lsof -ti :8888 | xargs -r kill -9
./chat_server
```

### Loi 4: khong thay log keyboard

Fix:
1. Dam bao module da load: `lsmod | grep usb_kbd_monitor`
2. Dam bao USB keyboard da attach vao guest CentOS (VMware).
3. Re-load module:
```bash
make driver-reload-kbd
```

### Loi 5: web backend loi venv/pip

Fix:
```bash
rm -rf .venv
make backend
make run-web
```

## 10) Checklist nghiem thu (copy cho bao cao)

Danh dau `PASS` sau khi xong:

- [ ] OS la CentOS 7, kernel xac dinh ro rang (`uname -r`)
- [ ] Cai du package build va kernel-devel match
- [ ] Build thanh cong `chat_server`, `chat_client`
- [ ] Build thanh cong `crypto_driver.ko`, `usb_kbd_monitor.ko`
- [ ] Load `usb_kbd_monitor` thanh cong, thay log EV_KEY
- [ ] Load `crypto_driver` thanh cong, co `/dev/crypto_dev`
- [ ] Chat server/client login va gui nhan message thanh cong
- [ ] Test private message thanh cong
- [ ] Chat va keyboard monitor chay song song khong xung dot
- [ ] Unload modules sach, khong oops/crash

## 11) Lenh chay nhanh (one-shot)

Neu ban da setup xong package va repo, day la chuoi lenh toi gian:

```bash
make clean && make
make driver-clean && make driver-build
make driver-load-kbd
make driver-load
./chat_server
```

Mo them terminal de chay:

```bash
./chat_client
```

Va terminal giam sat:

```bash
dmesg -w | grep usb_kbd_monitor
```

---

Tai lieu nay uu tien tinh thuc chien: copy/paste duoc ngay cho nguoi moi bat dau tren CentOS 7.
