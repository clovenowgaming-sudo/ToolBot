#pragma once

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>

using namespace geode::prelude;

enum class BotMode {
    None,
    Recording,
    Playing,
};

class Bot {
public:
    static Bot& get();

    BotMode mode = BotMode::None;

    // "armed" = dinyalakan dari tombol bulat di halaman level,
    // aksi mulai begitu PlayLayer terbuka
    bool armedRecord = false;
    bool armedPlay = false;

    // macro yang ada di memori (hasil record / import)
    gdr::Replay<> macro;
    bool hasMacro = false;
    size_t playIndex = 0;

    // state tombol yang sedang ditahan: [player2 ? 1 : 0][button 1..3]
    bool holding[2][4] = {};

    // settings (disimpan via saved values)
    double tps = 240.0;
    bool practiceFix = true;
    bool tpsBypass = true;
    bool blockInput = true;

    // internal: true saat playback sedang mengirim input (biar tidak dianggap input user)
    bool dispatching = false;

    void loadSettings();
    void saveSettings();

    // frame sekarang berdasarkan waktu level yang sama dipakai game
    // (m_gameState.m_levelTime) -> konsisten antara record, checkpoint, dan replay
    uint64_t frameNow(GJBaseGameLayer* gbl) const;

    void startRecording(PlayLayer* pl);
    void startPlaying(PlayLayer* pl);
    void stopBot(PlayLayer* pl);

    void onLevelStart(PlayLayer* pl);
    void onResetLevel(PlayLayer* pl);
    void onDestroyPlayer(PlayLayer* pl, PlayerObject* player);
    void onLevelComplete(PlayLayer* pl);
    void onQuitLevel(PlayLayer* pl);

    void applyTpsBypass();
    void restoreInterval();

    void updateStatusLabel(PlayLayer* pl);

    geode::Result<> saveMacroToDisk();
    geode::Result<> exportMacroTo(std::filesystem::path const& path);
    geode::Result<> importMacroFrom(std::filesystem::path const& path);

private:
    Bot() : macro("ToolBot", 1) {}

    double m_savedInterval = 0.0;
    bool m_intervalSaved = false;
};
