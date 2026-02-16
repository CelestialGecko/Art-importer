#include "importer/MainMenu.h"
#include "importer/CreateArt.h"
#include <functional>
#include <Geode/modify/CCLayer.hpp>
#include <Geode/modify/EditorUI.hpp>


using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {
public:

    // the shit that does stuff
    struct Fields {
        CreateArt artCreator;
        MainMenu* menu = nullptr;
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

    // creates the button that is used to open the pixel art importer
    void createMoveMenu() {
        EditorUI::createMoveMenu();
        // formats the button
        CCMenuItemSpriteExtra* btn = this->getSpriteButton("pixelArtToolBtn.png"_spr, menu_selector(MyEditorUI::onPixelArtImport), nullptr, 0.9f);
        // gives it a fun name
        btn->setID("importArtButton"_spr);
        // add a new button to the editor's UI without a custom image for now
        m_editButtonBar->m_buttonArray->addObject(btn);
        // fetch rows and columns settings from the game manager
        int rows = GameManager::sharedState()->getIntGameVariable("0049");
        int cols = GameManager::sharedState()->getIntGameVariable("0050");
        // reload it
        m_editButtonBar->reloadItems(rows, cols);
    }
};