#include "SettingsPopup.hpp"
#include "Bot.hpp"

#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/utils/async.hpp>
#include <Geode/utils/file.hpp>
#include <fmt/format.h>

#include <algorithm>
#include <cstdlib>
#include <optional>

static std::optional<double> parseTps(std::string const& str) {
    if (str.empty()) return std::nullopt;
    char* end = nullptr;
    double value = std::strtod(str.c_str(), &end);
    if (end != str.c_str() + str.size()) return std::nullopt;
    if (value < 1.0 || value > 5000.0) return std::nullopt;
    return value;
}

static CCMenuItemSpriteExtra* makeTextBtn(char const* caption, CCObject* target, SEL_MenuHandler selector) {
    auto spr = ButtonSprite::create(caption);
    spr->setScale(0.72f);
    return CCMenuItemSpriteExtra::create(spr, target, selector);
}

static void setToggleCaption(CCMenuItemSpriteExtra* btn, char const* name, bool on) {
    auto caption = fmt::format("{}: {}", name, on ? "ON" : "OFF");
    auto spr = ButtonSprite::create(caption.c_str());
    spr->setScale(0.72f);
    btn->setNormalImage(spr);
}

bool ToolBotPopup::init() {
    if (!Popup::init(330.f, 255.f)) return false;

    this->setTitle("ToolBot Settings");

    auto& bot = Bot::get();
    float cx = m_size.width / 2.f;

    // --- TPS ---
    auto tpsLabel = CCLabelBMFont::create("TPS (Record/Play):", "goldFont.fnt");
    tpsLabel->setScale(0.55f);
    tpsLabel->setPosition({ cx, m_size.height - 52.f });
    m_mainLayer->addChild(tpsLabel);

    m_tpsInput = geode::TextInput::create(130.f, "240", "bigFont.fnt");
    m_tpsInput->setCommonFilter(CommonFilter::Float);
    m_tpsInput->setMaxCharCount(6);
    m_tpsInput->setScale(0.9f);
    m_tpsInput->setPosition({ cx, m_size.height - 82.f });
    m_tpsInput->setString(fmt::format("{}", bot.tps).c_str());
    m_tpsInput->setCallback([](std::string const& str) {
        if (auto value = parseTps(str)) {
            Bot::get().tps = *value;
            Bot::get().saveSettings();
        }
    });
    m_mainLayer->addChild(m_tpsInput);

    // tombol minus & plus TEPAT DI BAWAH input TPS
    auto minusSpr = CircleButtonSprite::create(
        CCLabelBMFont::create("-", "bigFont.fnt"),
        CircleBaseColor::Pink, CircleBaseSize::Tiny
    );
    auto minusBtn = CCMenuItemSpriteExtra::create(minusSpr, this, menu_selector(ToolBotPopup::onTpsMinus));
    minusBtn->setPosition({ cx - 40.f, m_size.height - 118.f });
    m_buttonMenu->addChild(minusBtn);

    auto plusSpr = CircleButtonSprite::create(
        CCLabelBMFont::create("+", "bigFont.fnt"),
        CircleBaseColor::Green, CircleBaseSize::Tiny
    );
    auto plusBtn = CCMenuItemSpriteExtra::create(plusSpr, this, menu_selector(ToolBotPopup::onTpsPlus));
    plusBtn->setPosition({ cx + 40.f, m_size.height - 118.f });
    m_buttonMenu->addChild(plusBtn);

    // --- toggles ---
    m_pfBtn = makeTextBtn("Practice Fix: ON", this, menu_selector(ToolBotPopup::onTogglePracticeFix));
    m_pfBtn->setPosition({ cx, m_size.height - 150.f });
    m_buttonMenu->addChild(m_pfBtn);

    m_bypassBtn = makeTextBtn("TPS Bypass: ON", this, menu_selector(ToolBotPopup::onToggleBypass));
    m_bypassBtn->setPosition({ cx, m_size.height - 176.f });
    m_buttonMenu->addChild(m_bypassBtn);

    m_blockBtn = makeTextBtn("Block Input Play: ON", this, menu_selector(ToolBotPopup::onToggleBlockInput));
    m_blockBtn->setPosition({ cx, m_size.height - 202.f });
    m_buttonMenu->addChild(m_blockBtn);

    refreshToggles();

    // --- save / import / export ---
    auto saveBtn = makeTextBtn("Save", this, menu_selector(ToolBotPopup::onSave));
    saveBtn->setPosition({ cx - 105.f, 32.f });
    m_buttonMenu->addChild(saveBtn);

    auto importBtn = makeTextBtn("Import", this, menu_selector(ToolBotPopup::onImport));
    importBtn->setPosition({ cx, 32.f });
    m_buttonMenu->addChild(importBtn);

    auto exportBtn = makeTextBtn("Export", this, menu_selector(ToolBotPopup::onExport));
    exportBtn->setPosition({ cx + 105.f, 32.f });
    m_buttonMenu->addChild(exportBtn);

    return true;
}

void ToolBotPopup::refreshToggles() {
    auto& bot = Bot::get();
    setToggleCaption(m_pfBtn, "Practice Fix", bot.practiceFix);
    setToggleCaption(m_bypassBtn, "TPS Bypass", bot.tpsBypass);
    setToggleCaption(m_blockBtn, "Block Input Play", bot.blockInput);
}

void ToolBotPopup::onTpsPlus(CCObject*) {
    auto& bot = Bot::get();
    bot.tps = std::min(bot.tps + 10.0, 5000.0);
    bot.saveSettings();
    if (m_tpsInput) m_tpsInput->setString(fmt::format("{}", bot.tps).c_str());
}

void ToolBotPopup::onTpsMinus(CCObject*) {
    auto& bot = Bot::get();
    bot.tps = std::max(bot.tps - 10.0, 1.0);
    bot.saveSettings();
    if (m_tpsInput) m_tpsInput->setString(fmt::format("{}", bot.tps).c_str());
}

void ToolBotPopup::onTogglePracticeFix(CCObject*) {
    auto& bot = Bot::get();
    bot.practiceFix = !bot.practiceFix;
    bot.saveSettings();
    refreshToggles();
}

void ToolBotPopup::onToggleBypass(CCObject*) {
    auto& bot = Bot::get();
    bot.tpsBypass = !bot.tpsBypass;
    bot.saveSettings();
    refreshToggles();
}

void ToolBotPopup::onToggleBlockInput(CCObject*) {
    auto& bot = Bot::get();
    bot.blockInput = !bot.blockInput;
    bot.saveSettings();
    refreshToggles();
}

void ToolBotPopup::onSave(CCObject*) {
    auto res = Bot::get().saveMacroToDisk();
    if (res.isOk()) {
        Notification::create("Macro tersimpan di folder save ToolBot/macros", NotificationIcon::Success)->show();
    } else {
        Notification::create(res.unwrapErr().c_str(), NotificationIcon::Error)->show();
    }
}

void ToolBotPopup::onImport(CCObject*) {
    geode::utils::file::FilePickOptions options;
    options.filters.push_back({ "GDR2 Replay", { "gdr2" } });

    geode::async::spawn(
        geode::utils::file::pick(geode::utils::file::PickMode::OpenFile, options),
        [](geode::utils::file::PickResult result) {
            if (result.isErr()) {
                Notification::create(
                    fmt::format("Gagal membuka file: {}", result.unwrapErr()).c_str(),
                    NotificationIcon::Error
                )->show();
                return;
            }
            auto picked = result.unwrap();
            if (!picked) return; // user batal pilih file

            auto res = Bot::get().importMacroFrom(*picked);
            if (res.isOk()) {
                Notification::create(
                    fmt::format("Macro di-import! TPS diset ke {}", Bot::get().tps).c_str(),
                    NotificationIcon::Success
                )->show();
            } else {
                Notification::create(
                    fmt::format("Import gagal: {}", res.unwrapErr()).c_str(),
                    NotificationIcon::Error
                )->show();
            }
        }
    );
}

void ToolBotPopup::onExport(CCObject*) {
    auto& bot = Bot::get();
    if (!bot.hasMacro) {
        Notification::create("Belum ada macro! Record atau import dulu.", NotificationIcon::Error)->show();
        return;
    }

    geode::utils::file::FilePickOptions options;
    options.filters.push_back({ "GDR2 Replay", { "gdr2" } });
    options.defaultPath = Mod::get()->getSaveDir() / "macros" / "macro.gdr2";

    geode::async::spawn(
        geode::utils::file::pick(geode::utils::file::PickMode::SaveFile, options),
        [](geode::utils::file::PickResult result) {
            if (result.isErr()) {
                Notification::create(
                    fmt::format("Gagal membuka file: {}", result.unwrapErr()).c_str(),
                    NotificationIcon::Error
                )->show();
                return;
            }
            auto picked = result.unwrap();
            if (!picked) return;

            auto res = Bot::get().exportMacroTo(*picked);
            if (res.isOk()) {
                Notification::create("Macro berhasil di-export!", NotificationIcon::Success)->show();
            } else {
                Notification::create(
                    fmt::format("Export gagal: {}", res.unwrapErr()).c_str(),
                    NotificationIcon::Error
                )->show();
            }
        }
    );
}

ToolBotPopup* ToolBotPopup::create() {
    auto ret = new ToolBotPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}
