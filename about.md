# ToolBot

Macro bot untuk Geometry Dash 2.2081. Rekam input kamu, replay sebagai macro, atur TPS, dan simpan/bagi macro dalam format GDR2 yang kompatibel dengan bot lain.

## Fitur

- **Buletan 1 (merah muda, titik merah)**: arm/disarm **Record** — masuk ke level untuk mulai merekam.
- **Buletan 2 (hijau, segitiga)**: arm/disarm **Replay** — masuk ke level untuk memutar macro yang tersimpan/diimport.
- **Buletan 3 (abu-abu, garis tiga)**: buka **ToolBot Settings**:
  - Input **TPS** + tombol **plus/minus** di bawahnya untuk mengatur tick rate record/play.
  - Toggle **Practice Fix**.
  - Toggle **TPS Bypass**.
  - Tombol **Save / Import / Export** macro (format `.gdr2`).
- **Practice Fix**: kalau kamu record sambil practice mode lalu mati, input setelah checkpoint terakhir dibuang dan death tidak dicatat — jadi pas replay, bot tidak ikut mati di tempat kamu mati waktu record.
- Status kecil di pojok kiri atas saat in-game (`REC` / `PLAY`).

Lihat [README.md](README.md) untuk panduan lengkap (build, repo GitHub, cara pakai).
