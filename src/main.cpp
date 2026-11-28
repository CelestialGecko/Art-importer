#include "importer/MainMenu.h"
#include "importer/CreateArt.h"
#include <functional>
#include <Geode/modify/CCLayer.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <alphalaneous.editortab_api/API.hpp>

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
public:

    // the shit that does stuff
    struct Fields {
        CreateArt artCreator;
        MainMenu* menu = nullptr;
        std::string bCol = Mod::get()->getSettingValue<std::string>("Button-Colour");
    };

    // triggered when button is clicked
    void onPixelArtImport(CCObject*) {
        GameObject* ob = CCArrayExt<GameObject*>(this->getSelectedObjects())[0];
        // check if exactly one object is selected
        if (this->getSelectedObjects()->count() == 1) {
            // sets values needed for importing
			m_fields->artCreator.setSelectedObject(ob);
            // creates a menu and gets a pointer to a method to close it outside of the menu
            m_fields->menu = MainMenu::create(this->getSelectedObjects(), &m_fields->artCreator);
            m_fields->artCreator.setCloseMenu(std::bind(&MainMenu::removeFromParent, m_fields->menu));
            CCScene::get()->addChild(m_fields->menu);

        }
        else {
            // show an error message if not exactly one object is selected
            FLAlertLayer::create("Error", "You need to select <cr>one</c> object!", "OK")->show();
        }
    }

    // if the player has this dependency then my other codes messes up everyone else buttons
    bool init(LevelEditorLayer* layer) {
        if (!EditorUI::init(layer)) return false;

        if (Loader::get()->isModLoaded("alphalaneous.editortab_api")) {
            CCMenuItemSpriteExtra* btn = alpha::editor_tabs::createEditButton(CCSprite::createWithSpriteFrameName("pixelArtToolBtn.png"_spr), getButtonColour(), [this](CCObject*) {
                this->onPixelArtImport(nullptr);
                });
            btn->setID("pixel-art-import-button");
            //btn->setTag(67);
            alpha::editor_tabs::getMode(alpha::editor_tabs::Edit)->getTab("edit")->getNode()->addItem(btn);
        }

        return true;
    }

    // creates the button that is used to open the pixel art importer
    void createMoveMenu() {
        
        EditorUI::createMoveMenu();

        // this stops it from breaking other mods however I dont want my mod fully dependent on this
        if(!Loader::get()->isModLoaded("alphalaneous.editortab_api")) {


            // credit to alphalaneous for this format as I was too busy to think of my own
            cocos2d::CCSprite* sp = CCSprite::createWithSpriteFrameName("pixelArtToolBtn.png"_spr);
            CCSize size = sp->getContentSize();
            float fit = std::min(std::min(32.f / size.width, 32.f / size.height), 1.f);

            ButtonSprite* btnS = ButtonSprite::create(sp, 40, 0, 40.f, fit, true, fmt::format("GJ_button_0{}.png", getButtonColour()).c_str(), false, 40.f);
            CCMenuItemSpriteExtra* btn = CCMenuItemExt::createSpriteExtra(btnS, [this](CCObject*) {
                this->onPixelArtImport(nullptr);
                });

            btn->setID("pixel-art-import-button");
            //btn->setTag(67);

            if (!m_editButtonBar) {
                CCNode* menu = m_dpadMoveNode->getChildByID("edit-buttons");
                auto warp = menu->getChildByID("warp-button");
                auto snap = menu->getChildByID("rotate-snap-button");
                btn->setPosition({ snap->getPositionX(), warp->getPositionY() });
				btn->setScale(snap->getScale());
				btn->m_baseScale = snap->getScale();
                menu->addChild(btn);
                menu->updateLayout();

            }
            else {
                // add a new button to the editor's UI without a custom image for now
                m_editButtonBar->m_buttonArray->addObject(btn);
                // fetch rows and columns settings from the game manager
                int rows = GameManager::sharedState()->getIntGameVariable("0049");
                int cols = GameManager::sharedState()->getIntGameVariable("0050");
                // reload it
                m_editButtonBar->reloadItems(rows, cols);
            }
		}
    }



    int getButtonColour() {
        if (m_fields->bCol == "pink") {
            return 3;
        }
        else if (m_fields->bCol == "blue") {
            return 2;
        }
        else if (m_fields->bCol == "green") {
            return 1;
        }
        else {
            return 5;
        }
    }
};