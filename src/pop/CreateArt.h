#pragma once
#include <Geode/Geode.hpp>
#include <Geode/Bindings.hpp>
#include <Geode/utils/file.hpp>

#include "stb_image.h"

using namespace geode::prelude;

static auto ALLOWED_TYPES2 = file::FilePickOptions{
    std::nullopt,
    {
        {
            "Image Files",
            { "*.png", "*.jpg" }
        }
    }
};

class CreateArt final{
private:
    // bunch if stuff I need to know
    // object IDs for various object sizes
    static constexpr int pixelObjID = 3097;
    static constexpr int medPixelObjID = 3094;
    static constexpr int bigPixelObjID = 3093;
    static constexpr int largePixelObjID = 3092;
    // object ID of old pixel object
    static constexpr int oldPixelObjID = 917;
    // z order layering
    static constexpr int zOrder = 1;
    // size of the objects
    static constexpr float objSize = 5.0f;
    // scale used for moving between pixels
    static constexpr float scale = 5;

    // colours inside the pixel
    uint8_t blue = 0;
    uint8_t green = 0;
    uint8_t red = 0;

    // selected object
    GameObject* obj = nullptr;

    // closing the menu after complete
    std::function<void()> closeMenu = nullptr;

    // settings
    bool limitSize = Mod::get()->getSettingValue<bool>("Disable-limit");
    int sizeLimit = Mod::get()->getSettingValue<int>("Size-limit");
    int colourChannel = Mod::get()->getSettingValue<int>("Colour-channel");
	bool useOldPixel = Mod::get()->getSettingValue<bool>("Use-OlderObjects");
	bool useScale = Mod::get()->getSettingValue<bool>("Enable-Scale");
	bool useBasicOptimization = Mod::get()->getSettingValue<bool>("Enable-Basic-optimise");

    // helper methods - methods of importing the art
    void simpleImport(const std::string& p);


    // other helpers
    void formatHSV(float red, float green, float blue, std::string &objColour);
    void RGBtoHSV(float& r, float& g, float& b);
public:
    CreateArt() {}
	~CreateArt();

    void importArt();
	void placeArt(const std::string &p);

    // setters
    void setSelectedObject(GameObject* o) { obj = o; }
    void setCloseMenu(std::function<void()> c) { closeMenu = c; }
};