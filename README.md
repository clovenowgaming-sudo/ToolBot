# ToolBot

Mod bot/macro untuk **Geometry Dash 2.2081** (Geode SDK **5.11.2**). Record input kamu, replay sebagai macro, atur TPS, dan simpan/bagi macro dalam format **GDR2** (GDReplayFormat) yang kompatibel dengan bot lain.

## Fitur

- **Buletan 1 (pink, titik merah)** — tombol **Record**. Tekan sekali buat "arm", lalu masuk level: recording mulai otomatis dari awal attempt.
- **Buletan 2 (hijau, segitiga play)** — tombol **Replay macro**. Butuh macro yang sudah direcord/di-import. Masuk level: macro langsung dimainkan.
- **Buletan 3 (abu-abu, garis tiga)** — buka **ToolBot Settings**:
  - **Input TPS** — ketik manual TPS untuk record/play (default `240`).
  - **Tombol plus (+) / minus (−)** tepat di bawah input — naik/turun TPS 10 per klik.
  - **Practice Fix** (default ON) — penjelasan di bawah.
  - **TPS Bypass** (default ON) — selama record/play, game dijalankan dengan dt = `1/TPS` supaya stepping deterministik.
  - **Block Input Play** (default ON) — input asli kamu diabaikan saat macro diputar (anti desync).
  - **Save** — simpan macro ke folder save mod (`.../save/toolbot.toolbot/macros/`).
  - **Import** — load file `.gdr2` dari mana saja (TPS otomatis disamakan dengan fps macro).
  - **Export** — simpan macro ke lokasi pilihanmu, siap dibagikan.
- Ketiga buletan muncul di **halaman level utama** (LevelPage, seperti screenshot Stereo Madness) dan di **LevelInfoLayer** (halaman level online).
- Indikator `REC` / `PLAY` kecil di pojok kiri atas saat in-game.

## Practice Fix

Kalau kamu record di **practice mode** terus mati:

1. Saat kamu mati, death **tidak dicatat** ke macro.
2. Saat respawn dari checkpoint, semua input yang terekam **setelah checkpoint itu dibuang** (frame dihitung dari `m_gameState.m_levelTime` yang otomatis di-restore game ke waktu checkpoint).
3. Tombol yang masih kamu tahan saat respawn otomatis direkam ulang di frame checkpoint supaya stream input tetap konsisten.

Hasilnya: pas macro di-replay, bot **tidak ikut mati** di tempat kamu mati waktu record. Practice fix juga berlaku saat playback — kalau bot mati (misal level-nya random), replay lanjut dari checkpoint dengan frame yang benar.

## Soal akurasi (jujur)

Tidak ada bot yang bisa menjamin angka pasti seperti "99.8%" — akurasi tergantung kondisi. Yang mod ini lakukan supaya sedekat mungkin 100%:

- Frame dihitung dari **`m_gameState.m_levelTime`** (clock internal game), bukan timer luar — sama persis antara record, checkpoint, dan replay.
- **TPS Bypass** membuat tiap update memproses dt = `1/TPS` → stepping deterministik.
- Format GDR2 menyimpan TPS macro; saat import, TPS otomatis disamakan.

Tips akurasi maksimal:
- **Record dan play di TPS yang sama** (otomatis kalau pakai import).
- Default **240** = tick rate native GD 2.2 → lossless. TPS di atas 240 butuh patch physics sungguhan (di luar scope mod ini); di atas 240, TPS di sini hanya quantisasi frame + FPS bypass.
- Di HP 60Hz, input sentuh cuma disampling 60x/detik — untuk hasil terbaik record di TPS 60/120, atau record di PC.
- Level dengan trigger random tetap bisa beda hasilnya.

## Cara pakai singkat

1. Buka halaman level → tekan **buletan gear (3)** → set TPS kalau perlu.
2. Tekan **buletan 1** (record ON) → masuk level → main seperti biasa → selesaikan level / keluar.
3. Rekaman otomatis jadi macro. Buka gear → **Save** (atau **Export** untuk dibagikan).
4. Tekan **buletan 2** (replay ON) → masuk level → macro dimainkan otomatis.

## Build lokal (opsional)

Butuh [Geode CLI](https://docs.geode-sdk.org/getting-started/sdk-installation):

```sh
geode sdk install        # sekali saja
geode sdk update         # pastikan SDK 5.11.2
geode build              # hasil: toolbot.toolbot.geode
```

Build otomatis di GitHub sudah disiapkan lewat `.github/workflows/build.yml` — setiap push akan build untuk Windows, macOS, iOS, Android32, dan Android64, lalu menghasilkan satu file `ToolBot.geode` di artifacts.

## Bikin repo GitHub & upload (pakai git)

```sh
cd ToolBot
git init
git add .
git commit -m "ToolBot v1.0.0"

# bikin repo kosong dulu di https://github.com/new (nama bebas, misal ToolBot)
git remote add origin https://github.com/USERNAME/ToolBot.git
git branch -M main
git push -u origin main
```

## Cara yang lebih gampang: pakai GitHub CLI (`gh`)

Install `gh` dari https://cli.github.com, lalu:

```sh
gh auth login                       # login sekali saja (pilih GitHub.com -> HTTPS -> browser)

cd ToolBot
git init
git add .
git commit -m "ToolBot v1.0.0"

gh repo create ToolBot --public --source=. --push
# selesai! repo langsung ke-create dan ke-push
```

Setelah push, buka tab **Actions** di repo → tunggu workflow "Build Geode Mod" selesai → download artifact **ToolBot.geode**. Bisa juga trigger manual lewat **Actions → Build Geode Mod → Run workflow**.

### Rilis (supaya orang bisa download .geode tanpa build)

```sh
git tag v1.0.0
git push --tags
# build dulu secara lokal (geode build) atau ambil dari artifact Actions, lalu:
gh release create v1.0.0 path/ke/toolbot.toolbot.geode --title "ToolBot v1.0.0" --notes "Rilis pertama"
```

## Struktur proyek

```
ToolBot/
├── mod.json                    # metadata mod (Geode 5.11.2, GD 2.2081)
├── CMakeLists.txt              # build config (C++23)
├── .github/workflows/build.yml # CI: build semua platform
├── libraries/GDReplayFormat/   # format GDR2 (header-only)
└── src/
    ├── main.cpp                # hooks: record/play, tombol buletan, status
    ├── Bot.hpp / Bot.cpp       # state bot, practice fix, save/import/export
    └── SettingsPopup.hpp/.cpp  # popup TPS + plus/minus + save/import/export
```

## Kredit

- [GDReplayFormat](https://github.com/maxnut/GDReplayFormat) — format `.gdr2`.
- [ReplayBot](https://github.com/adafcaefc/ReplayBot-rewrite) — referensi konsep recorder/practice fix.
- [Geode](https://geode-sdk.org) — mod loader & SDK.
