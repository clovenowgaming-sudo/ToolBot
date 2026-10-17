#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelPage.hpp>
#include <Geode/modify/LevelInfoLayer.hpp>
#include <Geode/ui/BasedButtonSprite.hpp>
#include <Geode/ui/Notification.hpp>
#include <Geode/loader/GameEvent.hpp>

#include "Bot.hpp"
#include "SettingsPopup.hpp"

using namespace geode::prelude;

// ============================================================
// Icon buatan sendiri (digambar via CCDrawNode, jadi tidak
// tergantung sprite frame bawaan game)
// ============================================================

static CCDrawNode* makeRecIcon() {
    auto d = CCDrawNode::create();
    d->drawDot({ 0.f, 0.f }, 10.f, ccc4f(1.f, 0.2f, 0.2f, 1.f));
    return d;
}

static CCDrawNode* makePlayIcon() {
    auto d = CCDrawNode::create();
    CCPoint pts[3] = { { -6.f, -9.f }, { -6.f, 9.f }, { 9.f, 0.f } };
    d->drawPolygon(pts, 3, ccc4f(1.f, 1.f, 1.f, 1.f), 0.f, ccc4f(0.f, 0.f, 0.f, 0.f));
    return d;
}

static CCDrawNode* makeMenuIcon() {
    auto d = CCDrawNode::create();
    for (int i = -1; i <= 1; i++) {
        d->drawSegment({ -8.f, i * 6.f }, { 8.f, i * 6.f }, 2.2f, ccc4f(1.f, 1.f, 1.f, 1.f));
    }
    return d;
}

// ============================================================
// Target callback bersama untuk semua tombol ToolBot
// ============================================================

class ToolBotUI : public CCObject {
public:
    static ToolBotUI* get() {
        static auto* instance = new ToolBotUI();
        return instance;
    }

    void onRecord(CCObject* sender) {
        auto& bot = Bot::get();
        bot.armedRecord = !bot.armedRecord;
        if (bot.armedRecord) bot.armedPlay = false;
        updateStateLabel(sender);
        Notification::create(
            bot.armedRecord
                ? "ToolBot: Record ON - masuk level untuk mulai merekam"
                : "ToolBot: Record OFF",
            bot.armedRecord ? NotificationIcon::Success : NotificationIcon::Info
        )->show();
    }

    void onPlay(CCObject* sender) {
        auto& bot = Bot::get();
        if (!bot.hasMacro) {
            Notification::create(
                "ToolBot: belum ada macro! Record atau import dulu lewat tombol gear.",
                NotificationIcon::Error
            )->show();
            return;
        }
        bot.armedPlay = !bot.armedPlay;
        if (bot.armedPlay) bot.armedRecord = false;
        updateStateLabel(sender);
        Notification::create(
            bot.armedPlay
                ? "ToolBot: Replay ON - masuk level untuk memutar macro"
                : "ToolBot: Replay OFF",
            bot.armedPlay ? NotificationIcon::Success : NotificationIcon::Info
        )->show();
    }

    void onSettings(CCObject*) {
        ToolBotPopup::create()->show();
    }

private:
    static void updateStateLabel(CCObject* sender) {
        auto node = static_cast<CCNode*>(sender);
        auto menu = node ? node->getParent() : nullptr;
        if (!menu) return;
        auto label = static_cast<CCLabelBMFont*>(menu->getChildByID("toolbot-state"_spr));
        if (!label) return;

        auto& bot = Bot::get();
        if (bot.armedRecord) {
            label->setString("REC ON");
            label->setColor({ 255, 80, 80 });
        } else if (bot.armedPlay) {
            label->setString("PLAY ON");
            label->setColor({ 80, 255, 80 });
        } else {
            label->setString("");
        }
    }
};

// ============================================================
// Menu 3 buletan: (1) Record, (2) Replay, (3) Settings
// ============================================================

static void addToolBotMenu(CCLayer* layer, CCPoint pos) {
    if (layer->getChildByID("toolbot-menu"_spr)) return;

    auto menu = CCMenu::create();
    menu->setID("toolbot-menu"_spr);
    menu->setContentSize({ 0.f, 0.f });
    menu->setPosition(pos);

    auto target = ToolBotUI::get();

    // Buletan 1: RECORD
    auto recBtn = CCMenuItemSpriteExtra::create(
        CircleButtonSprite::create(makeRecIcon(), CircleBaseColor::Pink, CircleBaseSize::Medium),
        target, menu_selector(ToolBotUI::onRecord)
    );
    recBtn->setID("toolbot-record-btn"_spr);
    recBtn->setPosition({ 0.f, 0.f });
    menu->addChild(recBtn);

    // Buletan 2: REPLAY macro
    auto playBtn = CCMenuItemSpriteExtra::create(
        CircleButtonSprite::create(makePlayIcon(), CircleBaseColor::Green, CircleBaseSize::Medium),
        target, menu_selector(ToolBotUI::onPlay)
    );
    playBtn->setID("toolbot-play-btn"_spr);
    playBtn->setPosition({ 0.f, -46.f });
    menu->addChild(playBtn);

    // Buletan 3: Settings (TPS, practice fix, save/import/export)
    auto settingsBtn = CCMenuItemSpriteExtra::create(
        CircleButtonSprite::create(makeMenuIcon(), CircleBaseColor::Gray, CircleBaseSize::Small),
        target, menu_selector(ToolBotUI::onSettings)
    );
    settingsBtn->setID("toolbot-settings-btn"_spr);
    settingsBtn->setPosition({ 0.f, -86.f });
    menu->addChild(settingsBtn);

    auto stateLabel = CCLabelBMFont::create("", "bigFont.fnt");
    stateLabel->setID("toolbot-state"_spr);
    stateLabel->setScale(0.35f);
    stateLabel->setPosition({ 0.f, -108.f });
    menu->addChild(stateLabel);

    layer->addChild(menu, 100);
}

// ============================================================
// Hooks gameplay: record & playback
// ============================================================

class $modify(ToolBotGBL, GJBaseGameLayer) {
    void update(float dt) {
        auto& bot = Bot::get();
        auto pl = typeinfo_cast<PlayLayer*>(this);

        if (!pl || bot.mode == BotMode::None) {
            GJBaseGameLayer::update(dt);
            return;
        }

        if (bot.mode == BotMode::Playing) {
            uint64_t frame = bot.frameNow(this);
            auto& inputs = bot.macro.inputs;
            while (bot.playIndex < inputs.size() && inputs[bot.playIndex].frame <= frame) {
                auto const& inp = inputs[bot.playIndex];
                bot.dispatching = true;
                this->handleButton(inp.down, static_cast<int>(inp.button), !inp.player2);
                bot.dispatching = false;
                bot.playIndex++;
            }
            if (bot.playIndex >= inputs.size()) {
                bot.stopBot(pl);
                Notification::create("ToolBot: macro selesai diputar", NotificationIcon::Success)->show();
            }
        }

        // TPS bypass: setiap update memakai dt = 1/TPS supaya stepping deterministik
        if (bot.tpsBypass) {
            GJBaseGameLayer::update(1.0f / static_cast<float>(bot.tps));
        } else {
            GJBaseGameLayer::update(dt);
        }
    }

    void handleButton(bool down, int button, bool isPlayer1) {
        auto& bot = Bot::get();
        auto pl = typeinfo_cast<PlayLayer*>(this);

        if (pl && bot.mode == BotMode::Recording) {
            bool player2 = !isPlayer1;
            bot.macro.inputs.emplace_back(
                bot.frameNow(this),
                static_cast<uint8_t>(button),
                player2,
                down
            );
            if (button >= 1 && button <= 3) {
                bot.holding[player2 ? 1 : 0][button] = down;
            }
        } else if (
            pl && bot.mode == BotMode::Playing &&
            bot.blockInput && !bot.dispatching
        ) {
            // blokir input asli user saat macro sedang diputar
            return;
        }

        GJBaseGameLayer::handleButton(down, button, isPlayer1);
    }
};

class $modify(ToolBotPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        Bot::get().onLevelStart(this);
        return true;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        Bot::get().onResetLevel(this);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        PlayLayer::destroyPlayer(player, object);
        Bot::get().onDestroyPlayer(this, player);
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        Bot::get().onLevelComplete(this);
    }

    void onQuit() {
        Bot::get().onQuitLevel(this);
        PlayLayer::onQuit();
    }
};

// ============================================================
// Hooks halaman level: tempat 3 buletan ToolBot
// ============================================================

class $modify(ToolBotLevelPage, LevelPage) {
    bool init(GJGameLevel* level) {
        if (!LevelPage::init(level)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        addToolBotMenu(this, { 34.f, winSize.height - 148.f });
        return true;
    }
};

class $modify(ToolBotLevelInfoLayer, LevelInfoLayer) {
    bool init(GJGameLevel* level, bool challenge) {
        if (!LevelInfoLayer::init(level, challenge)) return false;

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        addToolBotMenu(this, { winSize.width - 34.f, winSize.height - 130.f });
        return true;
    }
};

// ============================================================
// Load settings pas game nyala
// ============================================================

$on_game(Loaded) {
    Bot::get().loadSettings();
}
