#include "MainMenu.h"

// sets up all the elements in the UI
bool MainMenu::setup(cocos2d::CCArray* startObj, CreateArt* artImposter) {

    artPointer = artImposter;

    this->setTitle(" Choose a file to import ");

    // imports the art
    CCMenuItemSpriteExtra* btn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Choose a file"),
        this,
        menu_selector(MainMenu::importArt)
    );
    btn->setPosition(MainMenu::m_size.width / 2, MainMenu::m_size.height / 2 + 7.5f);
    this->m_buttonMenu->addChild(btn);

    // opens the settings menu
    btn = CCMenuItemSpriteExtra::create(
        ButtonSprite::create("Settings"),
        this,
        menu_selector(MainMenu::openSettings)
    );
    btn->setPosition(MainMenu::m_size.width / 2, MainMenu::m_size.height / 2 - 32.5f);
    this->m_buttonMenu->addChild(btn);

    // info menu
    btn = CCMenuItemSpriteExtra::create(
        CCSprite::createWithSpriteFrameName("GJ_infoIcon_001.png"),
        this,
        menu_selector(MainMenu::openInfo)
    );
    btn->setPosition(MainMenu::m_size.width, MainMenu::m_size.height);
    this->m_buttonMenu->addChild(btn);

    //btn = CCMenuItemSpriteExtra::create(
    //    ButtonSprite::create("Restart"),
    //    this,
    //    menu_selector(MainMenu::restartGame)
    //);
    //btn->setPosition(MainMenu::m_size.width / 2, MainMenu::m_size.height / 2 - 100.0f);
    //this->m_buttonMenu->addChild(btn);

    return true;
}

// create the menu
MainMenu* MainMenu::create(CCArray* startObj, CreateArt* artImposter) {
    MainMenu* ret = new MainMenu();
    if (ret && ret->initAnchored(240.0f, 130.0f, startObj, artImposter)) {
        ret->autorelease();
    }
    else {
        delete ret;
        ret = nullptr;
    }
    return ret;
}

// info menu that also contain another discord plug
// discord grind never stops
void MainMenu::openInfo(CCObject* sender) {
    geode::createQuickPopup(
        "Info",
        "Imports images into the editor using pixels.\nI would recommend using a <cg>PNG</c> file.",
        "OK", "Get help",
        [](auto, bool btn2) {
            if (btn2) {
                web::openLinkInBrowser("https://discord.gg/nS5HFrbJ6y");
            }
        }
    );
}

// updates settings and then imports the art
void MainMenu::importArt(CCObject* sender) {
    artPointer->updateSettings();
    artPointer->importArt();
}