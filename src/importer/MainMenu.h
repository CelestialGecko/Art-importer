#pragma once
#include <Geode/Bindings.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/utils/web.hpp>
#include "CreateArt.h"

using namespace geode::prelude;

class MainMenu : public geode::Popup<CCArray*, CreateArt*> {
private:
    CreateArt* artPointer = nullptr;
	bool imported = false;
protected:
    bool setup(CCArray* startObj, CreateArt* artImposter) override;
    // runs the importer
    void importArt(CCObject* sender);
    // opens info
    void openInfo(CCObject* sender);
    // lets user change the settings of the mod
    void openSettings(CCObject* sender) { openSettingsPopup(Mod::get()); }
    // temp - restart the game
    //void restartGame(CCObject* sender) { game::restart(); }
public:
    static MainMenu* create(CCArray* startObj, CreateArt* artImposter);
};