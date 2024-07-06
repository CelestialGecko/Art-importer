#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/Bindings.hpp>
#include <Geode/utils/cocos.hpp>
#include <Geode/modify/CCLayer.hpp>
#include <cocos2d.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <Geode/modify/GameObject.hpp>
#include <Geode/binding/GameObject.hpp>
#include <Geode/utils/file.hpp>
#include <algorithm>
#include <vector>
#include <cmath>
#include <utility>
#include <stdexcept>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace geode::prelude;

    static auto ALLOWED_TYPES = file::FilePickOptions {
        std::nullopt,
        {
            {
                "Image Files",
                { "*.png", "*.jpg" }
            }
        }
    };

class $modify(MyEditorUI, EditorUI) {

    struct Fields {
        GameObject* m_object = nullptr;
        std::vector<std::vector<bool>> containsPixelObject;
    };
private:
    // object IDs for various object sizes
    static constexpr int pixelObjID = 3097;
    static constexpr int medPixelObjID = 3094;
    static constexpr int bigPixelObjID = 3093;
    static constexpr int largePixelObjID = 3092;

    // object ID of old pixel object
    static constexpr int oldPixelObjID = 917;

    // black color channel
    static constexpr int colourChannel = 1010;

    // z order layering
    static constexpr int zOrder = 1;

    // size of the objects
    static constexpr float objSize = 5.0f;

    // scale used for moving between pixels
    static constexpr float scale = 5;

    struct OWOShape {
        std::vector<std::pair<int, int>> offsets;
        int id;
    };

    // initialize the objects, pls dont question the name
    static const std::vector<OWOShape> OWOshapes;

public:
    struct Pixel {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
    };

    struct HSV {
        double hue;
        double saturation;
        double value;
    };

    // loads the image
    std::vector<std::vector<Pixel>> loadImage(std::string const& imagePath) {
        int width;
        int height;
        int channels;
        // load it
        unsigned char* img = stbi_load(imagePath.c_str(), &width, &height, &channels, 0);
        // store image data in a vector
        std::vector<std::vector<Pixel>> imageData(height, std::vector<Pixel>(width));
        // loops through image
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                unsigned char* pixelOffset = img + (x + y * width) * channels;
                imageData[y][x] = {pixelOffset[0], pixelOffset[1], pixelOffset[2]};
            }
        }
        stbi_image_free(img);
        // returns the image data
        return imageData;
    }
    
    // triggered when button is clicked
    void onPixelArtImport(CCObject*) {
        // check if exactly one object is selected
        if(this->getSelectedObjects()->count() == 1) {
            // asks for a png
            std::string message = 
            "Choose a file to import, I would advise using a <cg>png</c>!";
            // opens a cool menu
            geode::createQuickPopup(
                "Import Art",
                message,
                "Close", "Open file explorer",
                [this](auto, bool btn2) {
                    if (btn2) {
                        // weird new file picking system
                        utils::file::pick(file::PickMode::OpenFile, ALLOWED_TYPES).listen(
                            [this](Result<std::filesystem::path>* result) {
                                if (!result->isOk()) {
                                    // handles error
                                    FLAlertLayer::create("Error", "Failed to choose file", "OK")->show();
                                    return;
                                }
                                auto path = result->unwrap();
                                // makes path string
                                std::string pathStr = path.string(); 
                                CreateArt(pathStr);
                            },
                            [](auto const&) {} // I needed this for some reason
                        );
                    }
                }
            );
        } 
        else {
            // show an error message if not exactly one object is selected
            FLAlertLayer::create("Error", 
            "You need to select <cr>one</c> object!", "OK")->show();
        }
    }

    void CreateArt(const std::string& path) {
        int height;
        int channels;
        int width;
        unsigned char *data = nullptr;
        // gets the start position for where the art gets made
        auto* object = CCArrayExt<GameObject*>(this->getSelectedObjects())[0];
        m_fields->m_object = object;
        float startX = object->getPositionX();
        float startY = object->getPositionY();
        // holds the added object in string format
        std::ostringstream objInLevel;
        std::string objString;
        try {
            // gets image data
            data = stbi_load(path.c_str(), &width, &height, &channels, 0);
            // checks if the image data was fetched
            if (!data) {
                throw std::runtime_error("Failed to load image.");
            }
            // gets the settings 
            auto sizeLimitValue = Mod::get()->getSettingValue<bool>("Disable-limit");
            auto useScaling = Mod::get()->getSettingValue<bool>("Enable-Scale");
            auto useBasicOpt = Mod::get()->getSettingValue<bool>("Enable-Basic-optimise");
            auto useOldObject = Mod::get()->getSettingValue<bool>("Use-OlderObjects");
            // sets the pixel object id
            int currentObjID = pixelObjID;
            if(useOldObject){
                currentObjID = oldPixelObjID;
            }
            // checks the size or if the size limit is on
            if ((width * height > 40000) && !sizeLimitValue) {
                throw std::runtime_error(
                "Image cannot be bigger than 200 by 200. Change this in the settings for this mod.");
            }
            // contains the pixels that have been placed
            std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));
            // loops through image data
            for (int y = height - 1; y >= 0; --y) {
                for (int x = 0; x < width;) {
                    // gets the index for the current pixel being looked at
                    int pixelIndex = (y * width + x) * channels;
                    // gets alpha amd checks if there is a pixel
                    uint8_t alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
                    if (placed[y][x] || alpha == 0) {
                        x++;
                        continue;
                    }
                    int xStretch = 0;
                    int yStretch = 0;
                    // checks if the old pixel is not being used or basic op is on
                    if(useBasicOpt && !useOldObject){
                        // sets a target colour for comparing
                        uint8_t targetColour[3] = {data[pixelIndex], data[pixelIndex + 1], 
                                data[pixelIndex + 2]};
                        // loops through the different shapes (these represent GD objects)
                        for (const auto& shape : OWOshapes){
                            bool canPlace = true;
                            // goes through the possible locations these shapes could fit
                            for (const auto& offset : shape.offsets){
                                int newX = x + offset.first;
                                int newY = y - offset.second;
                                // makes sure its not out of bounds
                                if (newY < 0 || newY >= height || newX < 0 || newX >= width) {
                                    canPlace = false;
                                    break;
                                }
                                // makes a new pixel index for the pixel being compared
                                int newPixelIndex = (newY * width + newX) * channels;
                                uint8_t newAlpha = (channels == 4) ? data[newPixelIndex + 3] : 255;
                                // checks if the new pixel is the same colour as the original pixel
                                if (data[newPixelIndex] != targetColour[0] ||
                                data[newPixelIndex + 1] != targetColour[1] ||
                                data[newPixelIndex + 2] != targetColour[2] ||
                                newAlpha == 0) {
                                    canPlace = false;
                                    break;
                                }
                            }
                            // checks if scale opt is checked or the shape is 0
                            if (shape.id == 0 && canPlace && useScaling) {
                                // new pixel indexes for the pixels offset y and x direction
                                int tempPixelIndexX = (y * width + x + 1) * channels;
                                int tempPixelIndexY = ((y - 1) * width + x) * channels;
                                // prioritises x over y
                                if(data[tempPixelIndexX] == targetColour[0] 
                                && data[tempPixelIndexX + 1] == targetColour[1] 
                                && data[tempPixelIndexX + 2] == targetColour[2] 
                                && !placed[y][x + 1]){
                                    for (int currentX = x; currentX < width; ++currentX) {
                                        int currentPixelIndex = (y * width + currentX) * channels;
                                        uint8_t newAlpha = (channels == 4) ? data[currentPixelIndex + 3] : 255;
                                        if (data[currentPixelIndex] == targetColour[0] &&
                                            data[currentPixelIndex + 1] == targetColour[1] &&
                                            data[currentPixelIndex + 2] == targetColour[2] &&
                                            !placed[y][currentX] && newAlpha != 0) {
                                            xStretch++;
                                            placed[y][currentX] = true;
                                        } else {
                                            break;
                                        }
                                    }
                                }else if(data[tempPixelIndexY] == targetColour[0] 
                                && data[tempPixelIndexY + 1] == targetColour[1] 
                                && data[tempPixelIndexY + 2] == targetColour[2] 
                                && !placed[y - 1][x]){
                                    for (int currentY = y; currentY >= 0; --currentY) {
                                        int currentPixelIndex = (currentY * width + x) * channels;
                                        uint8_t newAlpha = (channels == 4) ? 
                                                data[currentPixelIndex + 3] : 255;
                                        if (data[currentPixelIndex] == targetColour[0] &&
                                            data[currentPixelIndex + 1] == targetColour[1] &&
                                            data[currentPixelIndex + 2] == targetColour[2] &&
                                            !placed[currentY][x] && newAlpha != 0) {
                                            yStretch++;
                                            placed[currentY][x] = true;
                                        } else {
                                            break;
                                        }
                                    }
                                }
                                // does the same checks but for the second shape
                            }else if(shape.id == 1 && canPlace && useScaling){
                                // this section doesnt work so just ignore it
                                bool stretchX = true;
                                for (int i = 0; i <= 1; ++i) {
                                    int pixelIndexX = ((y - i) * width + x + 3) * channels;
                                    if (!(data[pixelIndexX] == targetColour[0] &&
                                        data[pixelIndexX + 1] == targetColour[1] &&
                                        data[pixelIndexX + 2] == targetColour[2]) ||
                                        placed[y - i][x + 3]) {
                                        stretchX = false;
                                        break;
                                    }
                                    else{
                                        placed[y - i][x + 3] = true;
                                        xStretch++;
                                    }
                                }
                                if(stretchX){
                                    for (int offsetX = 4; offsetX < width - x; ++offsetX) {
                                        bool rowMatch = true;
                                        for (int i = 0; i <= 1; ++i) {
                                            int pixelIndexX = ((y - i) * width + x + offsetX) * channels;
                                            if (!(data[pixelIndexX] == targetColour[0] &&
                                            data[pixelIndexX + 1] == targetColour[1] &&
                                            data[pixelIndexX + 2] == targetColour[2]) ||
                                            placed[y - i][x + offsetX]) {
                                                rowMatch = false;
                                                break;
                                            }
                                            else{
                                                placed[y - i][x + offsetX] = true;
                                            }
                                        }
                                        if(rowMatch == false){
                                            break;
                                        }
                                        else{
                                            xStretch++;
                                        }
                                    }
                                }
                            }
                            // checks if an object can be placed
                            if (canPlace) {
                                // formats the colours so they are GD format
                                std::string objColour = formatHsvToString(data[pixelIndex], 
                                            data[pixelIndex + 1], data[pixelIndex + 2]);
                                // for the scaling based opt it sets the offsets as true if they exist
                                for (const auto& offset : shape.offsets) {
                                    placed[y - offset.second][x + offset.first] = true;
                                }
                                // gets the current object id depending the shape
                                currentObjID = shape.id == 1 ? medPixelObjID : (shape.id == 2 ? 
                                bigPixelObjID : (shape.id == 3 ? largePixelObjID : pixelObjID));
                                // figures out the scale
                                float currentScale = shape.id == 1 ? 2.5f : (shape.id == 2 ? 5.0f : 
                                (shape.id == 3 ? 12.5f : 0));
                                // calculates the size
                                int currentSize = objSize * (shape.id + 1);
                                // shape 3 is much bigger than the others so I set it here
                                if(shape.id == 3){
                                    currentSize = 30;
                                }
                                // for the scaling I calculated the x and y scale
                                float currentXSize = currentSize * xStretch;
                                float currentYSize = currentSize * yStretch;
                                // checks if stretching has occured on x
                                if(xStretch > 0){
                                    objInLevel << "1," << currentObjID << ",2," << ((startX + x * scale + currentScale) + 
                                    (xStretch * 0.5 * scale - 2.5f)) << ",3," <<
                                    (startY - y * scale + currentScale) << ",21," << colourChannel <<
                                    ",41,1,43," << objColour << ",25," << zOrder << ",128," << 
                                    currentXSize  << ",129," << currentSize << ";";
                                }
                                // checks if stretching has occured on y
                                else if(yStretch > 0){
                                    objInLevel << "1," << currentObjID << ",2," << 
                                    (startX + x * scale + currentScale) << ",3," << ((startY - y * scale + currentScale)
                                     + (yStretch * 0.5 * scale - 2.5f)) << ",21," << colourChannel << ",41,1,43," 
                                     << objColour << ",25," << zOrder << ",128," << currentSize  
                                     << ",129," << currentYSize << ";";
                                }
                                // No stretching
                                else{
                                    objInLevel << "1," << currentObjID << ",2," << (startX + x * scale + currentScale) 
                                    << ",3," << (startY - y * scale + currentScale) << ",21," << colourChannel <<
                                    ",41,1,43," << objColour << ",25," << zOrder << ",32," << currentSize << ";";
                                }
                                break;
                            }
                        }
                    }
                    // if no opt is used
                    else{
                        std::string objColour = formatHsvToString(data[pixelIndex], 
                                    data[pixelIndex + 1], data[pixelIndex + 2]);
                        placed[y][x] = true;
                        if(useOldObject){
                            objInLevel << "1," << currentObjID << ",2," << startX + x * (scale + 2.5f) << ",3," <<
                            startY - y * (scale + 2.5f) << ",21," << colourChannel << ",41,1,43," <<
                            objColour << ",25," << zOrder << ",32," << objSize / 5 << ";";
                        }
                        else{
                            objInLevel << "1," << currentObjID << ",2," << (startX + x * scale) 
                                    << ",3," << (startY - y * scale) << ",21," << colourChannel <<
                                    ",41,1,43," << objColour << ",25," << zOrder << ",32," << objSize << ";";
                        }
                    }
                    // skips checking pixels when x stretch is used
                    x += std::max(1, xStretch);
                }
            }
            // removes the last ;
            objString = objInLevel.str();
            objString.pop_back();
            // adds the new objects to the level and then prompts the user
            auto editorLayer = LevelEditorLayer::get();
            editorLayer->createObjectsFromString(objString.c_str(), true, true);
            FLAlertLayer::create("Success!", "Art was imported", "OK")->show();
            if (data) {
                stbi_image_free(data);
                data = nullptr;
            }
            // if the image did not work
        } catch (const std::exception& e) {
            if (data) {
                stbi_image_free(data);
            }
            // used for testing
            std::string errorMessage = e.what();
            FLAlertLayer::create("Error", errorMessage, "OK")->show();
        }
    }
    
    // creates the button that is used to open the pixel art importer
    void createMoveMenu() {
        EditorUI::createMoveMenu();
        // formats the button
        auto* btn = this->getSpriteButton("pixelArtToolBtn.png"_spr, 
        menu_selector(MyEditorUI::onPixelArtImport), nullptr, 0.9f);
        // add a new button to the editor's UI without a custom image for now
        m_editButtonBar->m_buttonArray->addObject(btn);
        // fetch rows and columns settings from the game manager
        auto rows = GameManager::sharedState()->getIntGameVariable("0049"),
        cols = GameManager::sharedState()->getIntGameVariable("0050");
        // reload it
        m_editButtonBar->reloadItems(rows, cols);
    }

    // converts the rgb values to hsv
    void rgbToHsv(int red, int green, int blue,
     float &h, float &s, float &v) {
        // normalize the rgb values
        float r = red / 255.0f;
        float g = green / 255.0f;
        float b = blue / 255.0f;
        // find the maximum and minimum
        float maxColour = std::max(r, std::max(g, b));
        float minColour = std::min(r, std::min(g, b));
        // max colour value
        v = maxColour;
        // difference between the max and min
        float delta = maxColour - minColour;
        // if the difference is less than a small threshold, the colour is grey
        if (delta < 0.00001f) {
            s = 0;
            h = 0;
            return;
        }
        // the difference between the max and min 
        // colour values divided by the max colour
        if (maxColour > 0.0) {
            s = (delta / maxColour);
        } else {
            // max colour is 0, the colour is black, so saturation is 0, hue is undefined
            s = 0.0;
            h = std::numeric_limits<float>::quiet_NaN();
            return;
        }
        // calculate hue
        if (r >= maxColour)
            h = (g - b) / delta;
        else if (g >= maxColour)
            h = 2.0f + (b - r) / delta;
        else
            h = 4.0f + (r - g) / delta;
        // convert hue to degrees
        h *= 60.0f;
        if (h < 0.0f)
            // stop it being negative
            h += 360.0f;
        // converts colours to the gd format of -180 to 180
        h = fmodf(h + 180.0f, 360.0f) - 180.0f;
    }

    // makes hsv a string
    std::string formatHsvToString(int red, int green, int blue) {
        float h;
        float s;
        float v;
        rgbToHsv(red, green, blue, h, s, v);
        if(h == 0){
            h+=1;
        }
        // returns the formated data
        return std::to_string(h) + "a" + std::to_string(s)
         + "a" + std::to_string(v) + "a" + "1a1";
    }
};

const std::vector<MyEditorUI::OWOShape> MyEditorUI::OWOshapes = {
    // LargePixelObjID - checks this first as it's a big boy
    {
        {
            {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0},
            {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1},
            {0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2}, {5, 2},
            {0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3}, {5, 3},
            {0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4}, {5, 4},
            {0, 5}, {1, 5}, {2, 5}, {3, 5}, {4, 5}, {5, 5},
        },
        3
    },

    // bigPixelObjID
    {
        {
            {0, 0}, {1, 0}, {2, 0},
            {0, 1}, {1, 1}, {2, 1},
            {0, 2}, {1, 2}, {2, 2}
        },
        2
    },

    // medPixelObjID
    {
        {
            {0, 0}, {1, 0},
            {0, 1}, {1, 1}
        },
        1
    },

    // PixelObjID
    {
        {
            {0, 0}
        },
        0
    }
};
