#define STB_IMAGE_IMPLEMENTATION
#include "CreateArt.h"

CreateArt::~CreateArt() {
}

void CreateArt::importArt(cocos2d::CCObject* sender) {
    // picks a file then will listen for when the event is done
    utils::file::pick(file::PickMode::OpenFile, ALLOWED_TYPES2).listen(
        [this](Result<std::filesystem::path>* result) {
            // no file selected
            if (!result->isOk()) {
                FLAlertLayer::create("Error", "Failed to choose file", "OK")->show();
                return;
            }
            // gets the path as a string
            std::string pathStr = result->unwrap().string();
            placeArt(pathStr);
        }
    );
}

// places the art, will choose a specific method base on settings
void CreateArt::placeArt(const std::string &p) {
    FLAlertLayer::create("owo", "It worked", "OK")->show();
    // a simple optimisation that makes use of the different sized pixels
    if (useBasicOptimization && !useOldPixel) {
		simpleImport(p);
    }
    // scales the objects on the x and y to fit the area
    else if (useScale) {

    }
    // just places the pixels without any fancy optimisation
    else {

    }
}

void CreateArt::simpleImport(const std::string& p) {
    int height;
    int channels;
    int width;
    unsigned char* data = nullptr;
    float startX = obj->getPositionX();
    float startY = obj->getPositionY();
    std::ostringstream objInLevel;
    std::string objString;

    try {
        // gets image data
        data = stbi_load(p.c_str(), &width, &height, &channels, 0);

        // checks the size or if the size limit is on
        if ((width * height > sizeLimit) && !limitSize) {
            std::string message = "Image cannot be bigger than " + std::to_string(sizeLimit) + ".\nChange this in the settings menu.";
            throw std::runtime_error(message);
        }

        // if data doesnt exist or doesnt work
        if (!data) {
            throw std::runtime_error("Failed to load image.");
        }
        // gets the current id being used
        int currentObjID = pixelObjID;
        if (useOldPixel) {
            currentObjID = oldPixelObjID;
        }
        // used to determine if a pixel should be placed
        //std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));

        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                // gets the index for the current pixel being looked at
                int pixelIndex = (y * width + x) * channels;
                // gets alpha
                uint8_t alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
                // doesnt place anything if empty
                if (alpha == 0)continue;
                std::string objColour;
                formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);
                //placed[y][x] = true;
                if (useOldPixel) {
                    objInLevel << "1," << currentObjID << ",2," << startX + x * (scale + 2.5f) << ",3," <<
                        startY - y * (scale + 2.5f) << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",32," << objSize / 5 << ";";
                }
                else {
                    objInLevel << "1," << currentObjID << ",2," << (startX + x * scale)
                        << ",3," << (startY - y * scale) << ",21," << colourChannel <<
                        ",41,1,43," << objColour << ",25," << zOrder << ",32," << objSize << ";";
                }
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
    }
    catch (const std::exception& e) {
        if (data) {
            stbi_image_free(data);
			data = nullptr;
        }
        FLAlertLayer::create("Error", e.what(), "OK")->show();
    }
}


void CreateArt::formatHSV(float r, float g, float b, std::string& objColour) {
	RGBtoHSV(r, g, b);
    if (b == 0.0f) {
        b += 1.0f;
    }
    g = (g * 100) / 100;
    b = (b * 100) / 100;

	// formats the HSV values into a string
	objColour = std::to_string(r) + "a" + std::to_string(g)
        + "a" + std::to_string(b) + "a" + "1a1";
}

// faster version that I made for university
void CreateArt::RGBtoHSV(float& r, float& g, float& b) {
    // need them to be between 1 and 0
    r *= nom;
    g *= nom;
    b *= nom;
    // gets the largest, smallest and the difference
    float max = (std::max)(r, (std::max)(g, b));
    float min = (std::min)(r, (std::min)(g, b));
    float del = max - min;
    float hue = 0.0f, sat = 0.0f, vibe = max;
    // calculates the rest of the values (sat and hue)
    // when delta is 0 the the brightness is 0 so black
    if (del > 0.0f) {
        sat = del / max;
        // checks which colour is the highest
        // between -60 and 60 
        if (max == r) hue = 60.0f * ((g - b) / del);
        // between 60 and 180 
        else if (max == g) hue = 60.0f * (((b - r) / del) + 2.0f);
        // 180 to 300
        else hue = 60.0f * (((r - g) / del) + 4.0f);
    }
    // this is for the red so that the negative value converts to being between 300 and 360
    if (hue < 0.0f) hue += 360.0f;
    hue = ((static_cast<int>(hue) + 180) % 360) - 180;
    r = hue;
    g = sat;
    b = vibe;
}