#include "Bot.hpp"

#include <Geode/ui/Notification.hpp>
#include <Geode/utils/file.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <ctime>
#include <cstring>

Bot& Bot::get() {
    static Bot instance;
    return instance;
}

void Bot::loadSettings() {
    auto mod = Mod::get();
    tps = mod->getSavedValue<double>("tps", 240.0);
    practiceFix = mod->getSavedValue<bool>("practice-fix", true);
    tpsBypass = mod->getSavedValue<bool>("tps-bypass", true);
    blockInput = mod->getSavedValue<bool>("block-input", true);
}

void Bot::saveSettings() {
    auto mod = Mod::get();
    mod->setSavedValue("tps", tps);
    mod->setSavedValue("practice-fix", practiceFix);
    mod->setSavedValue("tps-bypass", tpsBypass);
    mod->setSavedValue("block-input", blockInput);
}

uint64_t Bot::frameNow(GJBaseGameLayer* gbl) const {
    double frame = gbl->m_gameState.m_levelTime * tps;
    if (frame <= 0.0) return 0;
    return static_cast<uint64_t>(std::llround(frame));
}

void Bot::applyTpsBypass() {
    if (!tpsBypass) return;
    auto director = cocos2d::CCDirector::sharedDirector();
    if (!m_intervalSaved) {
        m_savedInterval = director->getAnimationInterval();
        m_intervalSaved = true;
    }
    director->setAnimationInterval(1.0 / tps);
}

void Bot::restoreInterval() {
    if (m_intervalSaved) {
        cocos2d::CCDirector::sharedDirector()->setAnimationInterval(m_savedInterval);
        m_intervalSaved = false;
    }
}

void Bot::startRecording(PlayLayer* pl) {
    macro = gdr::Replay<>("ToolBot", 1);
    macro.framerate = tps;
    macro.gameVersion = 22081;

    if (auto gm = GameManager::sharedState()) {
        macro.author = gm->m_playerName.c_str();
    }
    if (pl->m_level) {
        int id = pl->m_level->m_levelID;
        macro.levelInfo.id = id > 0 ? static_cast<uint32_t>(id) : 0;
        macro.levelInfo.name = pl->m_level->m_levelName.c_str();
        macro.platformer = pl->m_level->isPlatformer();
    }

    std::memset(holding, 0, sizeof(holding));
    hasMacro = false;
    playIndex = 0;
    mode = BotMode::Recording;

    applyTpsBypass();
    updateStatusLabel(pl);
    Notification::create("ToolBot: RECORDING dimulai", NotificationIcon::Info)->show();
}

void Bot::startPlaying(PlayLayer* pl) {
    if (!hasMacro || macro.inputs.empty()) {
        Notification::create("ToolBot: belum ada macro! Record atau import dulu.", NotificationIcon::Error)->show();
        return;
    }
    macro.sortInputs();
    playIndex = 0;
    mode = BotMode::Playing;

    applyTpsBypass();
    updateStatusLabel(pl);
    Notification::create("ToolBot: PLAYING macro", NotificationIcon::Success)->show();
}

void Bot::stopBot(PlayLayer* pl) {
    if (mode == BotMode::Recording && !macro.inputs.empty()) {
        // level non-platformer cuma punya tombol jump di format GDR2,
        // buang input left/right biar tidak terkonversi jadi jump
        if (!macro.platformer) {
            auto& inputs = macro.inputs;
            inputs.erase(
                std::remove_if(inputs.begin(), inputs.end(),
                    [](gdr::Input<> const& i) { return i.button != 1; }),
                inputs.end()
            );
        }
        if (!macro.inputs.empty()) {
            macro.duration = static_cast<float>(macro.inputs.back().frame) / static_cast<float>(tps);
            hasMacro = true;
        }
    }
    mode = BotMode::None;
    restoreInterval();
    updateStatusLabel(pl);
}

void Bot::onLevelStart(PlayLayer* pl) {
    updateStatusLabel(pl);
    if (armedRecord) {
        armedRecord = false;
        startRecording(pl);
    } else if (armedPlay) {
        armedPlay = false;
        startPlaying(pl);
    }
    updateStatusLabel(pl);
}

void Bot::onResetLevel(PlayLayer* pl) {
    if (mode == BotMode::None) return;

    // Setelah resetLevel, game me-restore m_gameState dari checkpoint
    // (atau 0 kalau reset penuh), jadi frame di sini SELALU frame checkpoint.
    uint64_t frame = frameNow(pl);

    if (mode == BotMode::Recording) {
        auto& inputs = macro.inputs;

        // PRACTICE FIX: buang semua input yang terekam SETELAH checkpoint
        // tempat player respawn. Death di practice mode tidak ikut terekam,
        // jadi pas replay bot tidak mati di tempat kamu mati waktu record.
        // (Berlaku juga untuk reset penuh: frame = 0 -> recording mulai ulang.)
        inputs.erase(
            std::remove_if(inputs.begin(), inputs.end(),
                [frame](gdr::Input<> const& i) { return i.frame > frame; }),
            inputs.end()
        );
        macro.deaths.erase(
            std::remove_if(macro.deaths.begin(), macro.deaths.end(),
                [frame](uint64_t d) { return d > frame; }),
            macro.deaths.end()
        );

        // re-emit tombol yang masih ditahan supaya stream input tetap konsisten
        for (int p = 0; p < 2; p++) {
            for (int b = 1; b <= 3; b++) {
                if (holding[p][b]) {
                    inputs.emplace_back(frame, static_cast<uint8_t>(b), p == 1, true);
                }
            }
        }
    } else if (mode == BotMode::Playing) {
        // lanjutkan playback dari frame checkpoint
        auto& inputs = macro.inputs;
        playIndex = static_cast<size_t>(std::lower_bound(
            inputs.begin(), inputs.end(), frame,
            [](gdr::Input<> const& i, uint64_t f) { return i.frame < f; }
        ) - inputs.begin());

        // pastikan state tombol sesuai kondisi di frame checkpoint:
        // kalau di macro tombol sedang ditahan, tekan ulang supaya hold tidak putus
        for (int p = 0; p < 2; p++) {
            for (int b = 1; b <= 3; b++) {
                bool held = false;
                for (size_t i = playIndex; i-- > 0;) {
                    auto const& inp = inputs[i];
                    if ((inp.player2 ? 1 : 0) == p && static_cast<int>(inp.button) == b) {
                        held = inp.down;
                        break;
                    }
                }
                if (held) {
                    dispatching = true;
                    pl->handleButton(true, b, p == 0);
                    dispatching = false;
                }
            }
        }
    }

    updateStatusLabel(pl);
}

void Bot::onDestroyPlayer(PlayLayer* pl, PlayerObject* player) {
    if (mode != BotMode::Recording) return;
    if (player != pl->m_player1) return;

    // PRACTICE FIX: death saat practice mode tidak dicatat ke macro
    if (pl->m_isPracticeMode && practiceFix) return;

    macro.deaths.push_back(frameNow(pl));
}

void Bot::onLevelComplete(PlayLayer* pl) {
    if (mode == BotMode::Recording) {
        stopBot(pl);
        if (hasMacro) {
            Notification::create("ToolBot: rekaman selesai! Save lewat tombol gear.", NotificationIcon::Success)->show();
        }
    } else if (mode == BotMode::Playing) {
        stopBot(pl);
        Notification::create("ToolBot: macro selesai", NotificationIcon::Success)->show();
    }
}

void Bot::onQuitLevel(PlayLayer* pl) {
    if (mode != BotMode::None) {
        stopBot(pl);
    }
}

void Bot::updateStatusLabel(PlayLayer* pl) {
    if (!pl) return;

    auto label = static_cast<cocos2d::CCLabelBMFont*>(pl->getChildByID("toolbot-status"_spr));
    if (!label) {
        label = cocos2d::CCLabelBMFont::create("", "chatFont.fnt");
        label->setID("toolbot-status"_spr);
        label->setAnchorPoint({ 0.f, 1.f });
        label->setPosition({ 5.f, cocos2d::CCDirector::sharedDirector()->getWinSize().height - 5.f });
        label->setScale(0.9f);
        pl->addChild(label, 999);
    }

    switch (mode) {
        case BotMode::None:
            label->setString("");
            break;
        case BotMode::Recording:
            label->setString("REC");
            label->setColor({ 255, 60, 60 });
            break;
        case BotMode::Playing:
            label->setString("PLAY");
            label->setColor({ 60, 255, 60 });
            break;
    }
}

static std::string sanitizeFileName(std::string name) {
    for (auto& c : name) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (!(std::isalnum(uc) || c == '-' || c == '_' || c == '.')) c = '_';
    }
    if (name.empty()) name = "macro";
    return name;
}

geode::Result<> Bot::saveMacroToDisk() {
    if (!hasMacro) return geode::Err("Belum ada macro! Record atau import dulu.");

    auto dir = Mod::get()->getSaveDir() / "macros";
    GEODE_UNWRAP(geode::utils::file::createDirectoryAll(dir));

    auto base = macro.levelInfo.name.empty() ? "macro" : macro.levelInfo.name;
    auto name = fmt::format("{}-{}.gdr2", sanitizeFileName(base), static_cast<long long>(std::time(nullptr)));

    auto res = macro.exportData(dir / name);
    if (res.isErr()) return geode::Err(res.unwrapErr());

    log::info("ToolBot: macro disimpan ke {}", (dir / name).string());
    return geode::Ok();
}

geode::Result<> Bot::exportMacroTo(std::filesystem::path const& path) {
    if (!hasMacro) return geode::Err("Belum ada macro! Record atau import dulu.");

    auto p = path;
    if (p.extension() != ".gdr2") p += ".gdr2";

    auto res = macro.exportData(p);
    if (res.isErr()) return geode::Err(res.unwrapErr());

    log::info("ToolBot: macro diexport ke {}", p.string());
    return geode::Ok();
}

geode::Result<> Bot::importMacroFrom(std::filesystem::path const& path) {
    auto res = gdr::Replay<>::importData(path);
    if (res.isErr()) return geode::Err(res.unwrapErr());

    macro = res.unwrap();
    macro.sortInputs();

    if (macro.framerate > 0.0) {
        tps = macro.framerate;
        saveSettings();
    }

    hasMacro = !macro.inputs.empty();
    if (!hasMacro) return geode::Err("Macro kosong (tidak ada input).");

    log::info("ToolBot: macro diimport dari {} ({} inputs, {} fps)",
        path.string(), macro.inputs.size(), macro.framerate);
    return geode::Ok();
}
