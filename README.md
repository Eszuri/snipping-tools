# 📸 Native Windows Snipping Tool (Ultra-Fast & Native)

Aplikasi screenshot dan anotasi native Windows yang super cepat, ringan, dan mandiri, ditulis dalam **C++20 murni dengan Win32 & GDI+ API**.

---

## ✨ Fitur & Alur Kerja Aplikasi

### 1. Halaman Awal Sederhana (Home Page)
Saat membuka aplikasi, jendela utama langsung muncul dengan antarmuka yang bersih dan modern:
* **Tombol "➕ New Snip"**: Memulai pengambilan tangkapan layar baru.
* **Menu "⏱ Delay / Timeout"**: Pengaturan jeda waktu hitung mundur sebelum layar dibekukan (`Off / 0s`, `3 detik`, `5 detik`, `10 detik`) — sangat berguna untuk menangkap menu dropdown atau tooltip.
* **Menu "📐 Mode"**: Pilihan mode penangkapan (`Rectangle`, `Window`, `FullScreen`).

### 2. Live Preview Saat Menyeret Kursor (Drag Snip)
* Begitu tombol "New Snip" ditekan, jendela utama otomatis menyembunyikan diri.
* Selama menekan tombol mouse dan mengarahkan luas snip, area yang dipilih langsung menampilkan **Live Preview** yang terang dan jernih.
* Area di luar kotak seleksi otomatis digelapkan.
* Dilengkapi dengan badge ukuran dimensi real-time (`W × H px`), garis panduan (*crosshairs*), serta **Loupe Magnifier 4x** dengan info kode warna HEX & RGB.

### 3. Hasil Snip Langsung Terbuka di Aplikasi (Preview & Editor)
* Begitu kursor mouse dilepas (*release*), hasil potongan layar **langsung otomatis terbuka di jendela aplikasi**.
* **Canvas Preview & Annotation Studio**:
  * **Pen (`✎`)**: Menggambar bebas dengan goresan halus (*anti-aliased*).
  * **Highlighter (`🖍`)**: Spidol stabilo transparan.
  * **Bentuk Vektor (`▭`, `○`, `➜`)**: Kotak, lingkaran, dan panah penunjuk.
  * **Step Counter (`①`, `②`, `③`)**: Penomoran bulat otomatis bertambah.
  * **Blur / Redact (`▦`)**: Sensor informasi sensitif (password, data pribadi).
  * **Teks (`T`)**: Menambahkan catatan tulisan.
  * **Palet Warna & Ketebalan**: Pilihan 8 warna kontras.
  * **Undo (`Ctrl+Z`) / Redo (`Ctrl+Y`) / Clear (`🗑`)**: Riwayat pengeditan tanpa batas.
* **Aksi Cepat**:
  * **Copy (`Ctrl+C`)**: Menyalin gambar hasil anotasi langsung ke Clipboard.
  * **Save As (`Ctrl+S`)**: Menyimpan ke format PNG (lossless tanpa kompresi).
  * **Pin to Screen (`F2`)**: Membuat jendela potongan melayang (*always-on-top*).

---

## ⌨️ Daftar Shortcut

| Shortcut | Aksi |
| :--- | :--- |
| **`Ctrl + N`** / **`PrtScn`** | Mulai ambil Snip baru |
| **`Ctrl + C`** | Salin gambar ke Clipboard |
| **`Ctrl + S`** | Simpan gambar ke file PNG |
| **`Ctrl + Z`** | Undo anotasi terakhir |
| **`Ctrl + Y`** | Redo anotasi |
| **`F2`** | Pin gambar melayang di atas layar |
| **`Esc`** | Batalkan overlay seleksi |

---

## 🛠️ Kompilasi / Build

Cukup jalankan script batch yang tersedia:
```cmd
build.bat
```
File executable mandiri akan dihasilkan di [`bin/NativeSnippingTool.exe`](file:///d:/Codingan/C++/native-snipping-tool/bin/NativeSnippingTool.exe) (~707 KB, tanpa dependensi runtime eksternal).
