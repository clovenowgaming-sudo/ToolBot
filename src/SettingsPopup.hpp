#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>

using namespace geode::prelude;

class ToolBotPopup : public geode::Popup {
protected:
    geode::TextInput* m_tpsInput = nullptr;
    CCMenuItemSpriteExtra* m_pfBtn = nullptr;
    CCMenuItemSpriteExtra* m_bypassBtn = nullptr;
    CCMenuItemSpriteExtra* m_blockBtn = nullptr;

    bool init();

    void onTpsPlus(CCObject*);
    void onTpsMinus(CCObject*);
    void onTogglePracticeFix(CCObject*);
    void onToggleBypass(CCObject*);
    void onToggleBlockInput(CCObject*);
    void onSave(CCObject*);
    void onImport(CCObject*);
    void onExport(CCObject*);

    void refreshToggles();

public:
    static ToolBotPopup* create();
};
