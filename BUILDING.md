# Panduan Build & Deploy – Vyrja Classroom

Dokumen ini menjelaskan cara mengubah tampilan, lalu membuat aplikasi jadi (installer Windows `.exe`,
paket Linux `.deb`, atau build manual) dari source code ini.

> **Ringkasnya:** aplikasi ini (turunan Veyon) hanya mendukung **Windows** dan **Linux**.
> **macOS (.dmg) tidak didukung** – lihat bagian macOS di bawah.

---------------------------------------------------------------------------

## 1. Cara paling mudah: build di cloud (GitHub Actions) → dapat `.exe` dan `.deb`

Tidak perlu install apa pun di PC Anda.

1. Buat repository baru di GitHub, lalu upload seluruh isi folder source ini
   (atau `git init`, `git add .`, `git commit`, `git push`).
2. Buka tab **Actions** → workflow **Vyrja build** → **Run workflow**.
3. Tunggu ±20–40 menit. Setelah hijau, buka run tersebut → bagian **Artifacts**:
   * `vyrja-classroom-windows-x86_64` → berisi `vyrja-classroom-<versi>-win64-setup.exe` (installer Windows 64-bit)
   * `vyrja-classroom-linux-ubuntu.24.04` / `...debian.12` → berisi paket `.deb`
4. Untuk Windows 32-bit tambahkan `i686` pada `matrix.arch` di `.github/workflows/vyrja-build.yml`.

Catatan penting
* Source dari GitHub-release Veyon **tidak membawa submodule**. Folder `3rdparty/` pada paket ini sudah berisi
  `libvncserver`, `x11vnc`, `ultravnc`, `libfakekey`, `qthttpserver`. Yang **belum** ada hanya `kldap`
  (dukungan LDAP/Active Directory). Jika tidak butuh LDAP, tambahkan `-DSKIP_PLUGINS=ldap` pada `CMAKE_FLAGS`
  di workflow. Jika butuh LDAP:
  `git clone https://invent.kde.org/pim/kldap 3rdparty/kldap && git -C 3rdparty/kldap checkout 2002b2c4ec81d05eb0fc5b2106ef112d404f5cea`
* Image Docker Windows (`registry.gitlab.com/veyon/ci-mingw-w64:main`) disediakan proyek Veyon. Jika image itu
  suatu saat dipindah/diganti, lihat `.gitlab-ci.yml` di repository resmi Veyon untuk nama terbarunya.
  Workflow ini belum pernah dijalankan di akun GitHub Anda – kalau ada error, kirim log-nya, mudah diperbaiki.

---------------------------------------------------------------------------

## 2. Build Windows `.exe` secara lokal (Docker, di Windows/Mac/Linux)

Pakai Docker Desktop (atau WSL2 + Docker):

```bash
docker run --rm -v "$PWD":/src -w /src \
  -e CMAKE_FLAGS="" \
  registry.gitlab.com/veyon/ci-mingw-w64:main \
  .ci/windows/build.sh x86_64 create-windows-installer
```
Hasil: `vyrja-classroom-<versi>-win64-setup.exe` di folder source.
(Tambahkan `-DSKIP_PLUGINS=ldap` pada `CMAKE_FLAGS` bila `kldap` tidak ada.)

Pemasangan di Windows: jalankan installer sebagai Administrator.
* **Komputer guru**: pasang → jalankan **Vyrja Setup** → klik **Pengaturan Cepat** → pilih *Komputer Guru*.
* **Komputer siswa**: pasang → jalankan Vyrja Setup → *Komputer Siswa* → pilih file kunci dari guru.
* Jika Veyon asli sudah terpasang, **uninstall dulu** (service-nya bernama sama).

---------------------------------------------------------------------------

## 3. Build Linux

### 3a. Paket `.deb` lewat Docker (sama seperti cloud build)
```bash
docker run --rm -v "$PWD":/src -w /src veyon/ci.linux.ubuntu.24.04 \
  .ci/linux.ubuntu.24.04/script.sh /src /tmp
```
Hasil: `veyon*.deb` di folder source → `sudo apt install ./veyon_*.deb`.

### 3b. Build manual di Ubuntu/Debian
```bash
sudo apt install cmake ninja-build pkg-config g++ qt6-base-dev qt6-base-dev-tools qt6-tools-dev \
  qt6-tools-dev-tools qt6-l10n-tools libqt6core5compat6-dev libqca-qt6-dev libqca-qt6-plugins \
  libssl-dev libjpeg-dev libpng-dev liblzo2-dev zlib1g-dev libpam0g-dev libproc2-dev \
  libxtst-dev libxrandr-dev libxdamage-dev libxfixes-dev libxcomposite-dev libxinerama-dev \
  libxcursor-dev libxrender-dev libfakekey-dev libldap2-dev libsasl2-dev libgl-dev libvncserver-dev
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release -DWITH_TRANSLATIONS=ON -DWITH_WEBAPI=OFF \
      -DWITH_TESTS=OFF -DSKIP_PLUGINS="ldap" \
      -DCMAKE_CXX_FLAGS="-Wno-error=maybe-uninitialized" -DCMAKE_C_FLAGS="-Wno-error=maybe-uninitialized"
ninja -C build
sudo cmake --install build --prefix /usr/local        # atau: ninja -C build package  (membuat .deb)
```
Jalankan: `veyon-master` (Vyrja Classroom), `veyon-configurator` (Vyrja Setup).
Nama file program sengaja tetap `veyon-*` supaya tetap kompatibel dengan komputer siswa yang memakai Veyon asli.

Siswa Linux: layanan `veyon-service` dijalankan lewat systemd (wizard Pengaturan Cepat mengaturnya).

---------------------------------------------------------------------------

## 4. macOS (`.dmg`) – kenapa belum bisa

Veyon tidak memiliki *platform plugin* untuk macOS (yang mengatur service, input, sesi pengguna, firewall).
Artinya app bisa dikompilasi sebagian, tetapi **tidak bisa dikontrol / mengontrol** secara andal di macOS, dan
membuat `.dmg` palsu yang tidak berfungsi tidak akan membantu Anda. Pilihan realistis:

1. Pakai Windows/Linux untuk komputer guru dan siswa (disarankan).
2. Menulis plugin platform macOS sendiri (pekerjaan besar: capture layar via ScreenCaptureKit, input via
   CGEvent, izin Accessibility/Screen Recording, LaunchDaemon) – bisa dikerjakan sebagai proyek terpisah.

---------------------------------------------------------------------------

## 5. Mengubah tampilan / merek sendiri

| Yang ingin diubah | File |
|---|---|
| Nama produk, warna merek | `core/src/Branding.h` |
| Warna, bentuk tombol, header, kartu, tab (terang+gelap) | `core/resources/vyrja-theme.qss` + `core/src/VyrjaTheme.cpp` |
| **Semua ikon** (fitur, status, halaman, toast) | `branding/icons/generate_icons.py` → jalankan `python3 branding/icons/generate_icons.py` |
| Logo, splash, ikon aplikasi, gambar installer | folder `branding/` (lalu salin ke `core/resources/icon*.png`, `master/resources/splash.png`, `nsis/*.bmp`, `*.ico`) |
| Tata letak jendela Master (header, ribbon, kartu) | `master/src/MainWindow.cpp`, `master/src/ComputerItemDelegate.cpp` |
| Tata letak Setup (sidebar, judul halaman) | `configurator/src/MainWindow.cpp` |
| Wizard Pengaturan Cepat | `configurator/src/SetupWizard.cpp` |
| Aksi Kelas Cepat (scene, timer, absensi) | `master/src/ClassScenes.cpp` |
| Teks installer Windows | `nsis/veyon.nsi.in` |
| Terjemahan Indonesia | `translations/veyon_id.ts` |

Setelah mengubah, build ulang (bagian 1–3). Pengaturan bahasa: Setup → Umum → Bahasa, atau
`veyon-cli config set UI/Language "Bahasa Indonesia (id_ID)"`.

---------------------------------------------------------------------------

## 6. Soal hak cipta & lisensi (ringkas, bukan nasihat hukum)

* Veyon berlisensi **GPL-2.0-or-later**. Anda **boleh** mengubah tampilan, menamai ulang, dan mendistribusikannya,
  selama: tetap GPL, menyertakan file `COPYING`, mempertahankan catatan hak cipta pembuat asli (sudah ada di dialog
  *Tentang*), dan menyediakan source code bagi penerima binary.
* Tata letak/tampilan sendiri tidak masalah. Yang sensitif adalah **nama dan logo "Veyon"** (merek dagang) dan
  **aset gambar pihak lain** – semuanya sudah diganti: nama → Vyrja Classroom, logo → logo Vyrja Studio,
  seluruh ikon digambar ulang (orisinal, dibuat oleh `generate_icons.py`).
* Anda tidak dapat menjadikannya closed-source atau mengklaim sebagai satu-satunya pencipta kode.
* Teks pada dialog/README masih menyebut "based on Veyon" – itu kewajiban kredit yang sebaiknya dipertahankan.
