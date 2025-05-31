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
    // large old pixel
	static constexpr int oldLargePixelObjID = 211;
    // z order layering
    static constexpr int zOrder = 1;
    // size of the objects
    static constexpr float objSize = 5.0f;
    // scale used for moving between pixels
    static constexpr float scale = 5;

    // selected object
    GameObject* obj = nullptr;

    // closing the menu after complete
    std::function<void()> closeMenu = nullptr;

    // settings
    bool limitSize = Mod::get()->getSettingValue<bool>("Disable-limit");
    int sizeLimit = Mod::get()->getSettingValue<int>("Size-limit");
    int colourChannel = Mod::get()->getSettingValue<int>("Colour-channel");
	bool useOldPixel = Mod::get()->getSettingValue<bool>("Use-OlderObjects");
	int tolerance = Mod::get()->getSettingValue<int>("Tolerance");
	std::string basic = Mod::get()->getSettingValue<std::string>("Optimise-Type");

    // places the art with the path
    void placeArt(const std::string& p);

    // helper methods - methods of importing the art
    void simpleImport(const std::string& p);
	void basicOptimiseImport(const std::string& p);
	void scaleOptimiseImport(const std::string& p);

    // other helpers
    void formatHSV(float red, float green, float blue, std::string& objColour) const;
    void RGBtoHSV(float& r, float& g, float& b) const;
    int bestFit(std::vector<std::vector<bool>>& p, unsigned char const* data, int x, int y, int ch, int wid, int hi);
    bool comparePixels(unsigned char const*& data, int p1, int p2) const;
    int scalePixX(std::vector<std::vector<bool>>& p, unsigned char const* data, int x, int y, int ch, int wid, int hi);
	int scalePixY(std::vector<std::vector<bool>>& p, unsigned char const* data, int x, int y, int ch, int wid, int hi, int xScale);

public:
    CreateArt() {}
    ~CreateArt() {}

    // importing the art
    void importArt();

    // changes the settings before the art is imported
    void updateSettings();

    // setters
    void setSelectedObject(GameObject* o) { obj = o; }
    void setCloseMenu(std::function<void()> c) { closeMenu = c; }
};