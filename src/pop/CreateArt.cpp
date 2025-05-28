#define STB_IMAGE_IMPLEMENTATION
#include "CreateArt.h"

CreateArt::~CreateArt() {

}

void CreateArt::importArt() {
    // picks a file then will listen for when the event is done
    utils::file::pick(file::PickMode::OpenFile, ALLOWED_TYPES2).listen(
        [this](Result<std::filesystem::path>* result) {
            // no file selected
            if (result->isOk()) {
                // gets the path as a string
                std::string pathStr = result->unwrap().string();
                placeArt(pathStr);
            }
        }
    );
}

// places the art, will choose a specific method base on settings
void CreateArt::placeArt(const std::string &p) {
    // a simple optimisation that makes use of the different sized pixels
    if (basic == "Basic Optimisation" && !useOldPixel) {
		basicOptimiseImport(p);
    }
    // scales the objects on the x and y to fit the area
    else if (basic == "Scale Optimisation") {
		scaleOptimiseImport(p);
    }
    // just places the pixels without any fancy optimisation
    else {
        simpleImport(p);
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
                // adds object to the string
                if (useOldPixel) {
                    objInLevel << "1," << oldPixelObjID << ",2," << startX + x * (scale + 2.5f) << ",3," <<
                        startY - y * (scale + 2.5f) << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",32," << objSize / 5 << ";";
                }
                else {
                    objInLevel << "1," << pixelObjID << ",2," << startX + x * scale << ",3," <<
                        startY - y * scale << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",32," << objSize << ";";
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
        // this is too scary for me not to check
        closeMenu();
    }
    catch (const std::exception& e) {
        if (data) {
            stbi_image_free(data);
			data = nullptr;
        }
        FLAlertLayer::create("Error", e.what(), "OK")->show();
    }
}

void CreateArt::basicOptimiseImport(const std::string& p) {
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
        // used to determine if a pixel should be placed
        std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
				if (placed[y][x]) continue;
                // gets the index for the current pixel being looked at
                int pixelIndex = (y * width + x) * channels;
                // gets alpha
                uint8_t alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
                // doesnt place anything if empty
                if (alpha == 0)continue;

                // decides which pixel fits the best
				int pixelType = bestFit(placed, data, x, y, channels, width, height);

                // gets the colour in GD format
                std::string objColour;
                formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);

                // places the objects
                switch (pixelType) {
                case 3097:
                    objInLevel << "1," << pixelType << ",2," << startX + x * scale << ",3," <<
                        startY - y * scale << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",32," << objSize << ";";;
                    break;
                case 3094:
                    objInLevel << "1," << pixelType << ",2," << (startX + x * scale) + 2.5f << ",3," <<
                        (startY - y * scale) + 2.5f << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",32," << objSize * 2.0f << ";";;
					break;
				case 3093:
                    objInLevel << "1," << pixelType << ",2," << (startX + x * scale) + 5.0f << ",3," <<
                        (startY - y * scale) + 5.0f << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",32," << objSize * 3.0f << ";";;
                    break;
				case 3092:
                    objInLevel << "1," << pixelType << ",2," << (startX + x * scale) + 12.5f << ",3," <<
                        (startY - y * scale) + 12.5f << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",32," << objSize * 6.0f << ";";;
					break;
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
        // this is too scary for me not to check
        closeMenu();
    }
    catch (const std::exception& e) {
        if (data) {
            stbi_image_free(data);
            data = nullptr;
        }
        FLAlertLayer::create("Error", e.what(), "OK")->show();
    }
}

// uses scaling to optimise the art
void CreateArt::scaleOptimiseImport(const std::string& p) {
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
        // used to determine if a pixel should be placed
        std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                if (placed[y][x]) continue;
                // gets the index for the current pixel being looked at
                int pixelIndex = (y * width + x) * channels;
                // gets alpha
                uint8_t alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
                // doesnt place anything if empty
                if (alpha == 0)continue;

				int scaleX = scalePixX(placed, data, x, y, channels, width, height);

                // gets the colour in GD format
                std::string objColour;
                formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);

                if (useOldPixel) {
                    objInLevel << "1," << oldPixelObjID << ",2," << startX + x * (scale + 2.5f) << ",3," <<
                        startY - y * (scale + 2.5f) << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",128," << objSize / 5
                        << ",129," << objSize / 5 << ";";
                }
                else {
					float xSize = scale * scaleX;
                    FLAlertLayer::create("Success!", std::to_string(scaleX), "OK")->show();

                    objInLevel << "1," << largePixelObjID << ",2," << startX + x * scale + xSize / 2 << ",3," <<
                        startY - y * scale << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",128," << xSize
                        << ",129," << objSize << ";";
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
        // this is too scary for me not to check
        closeMenu();
    }
    catch (const std::exception& e) {
        if (data) {
            stbi_image_free(data);
            data = nullptr;
        }
        FLAlertLayer::create("Error", e.what(), "OK")->show();
    }
}

// scales on the x
int CreateArt::scalePixX(std::vector<std::vector<bool>>& p, unsigned char*& data, int px, int py, int ch, int wid, int hi) {
    const int refPix = (py * wid + px) * ch;
	bool fits = true;
	int off = 1;
    do {
        if (px + off >= wid) {
			fits = false;
        }
        else {
            if (comparePixels(data, refPix, (py * wid + px + off) * ch)) {
                p[py][px + off] = true;
                ++off;
            }
            else {
                fits = false;
            }
        }
    } while (fits);
	p[py][px] = true;
	return off;
}

// scales on the y
int CreateArt::scalePixY(std::vector<std::vector<bool>>& p, unsigned char*& data, int px, int py, int ch, int wid, int hi, int xScale) {
    const int refPix = (py * wid + px) * ch;

}

// finds the best pixel object to reduce object count
int CreateArt::bestFit(std::vector<std::vector<bool>>& p, unsigned char*& data, int px, int py, int ch, int wid, int hi) {
    const int referencePixel = (py * wid + px) * ch;
    // storing the variations for the pixels in a array
    const std::vector<int> pixelSizes = { 1, 2, 3, 6 };
    const std::vector<int> pixelObjIDs = { pixelObjID, medPixelObjID, bigPixelObjID, largePixelObjID };

	// loops throught the pixel sizes
    for (int i = 0; i < pixelSizes.size(); ++i) {
        bool fits = true;

        // Check if the pixel block fits within image boundaries
        if (px + pixelSizes[i] > wid || py - pixelSizes[i] + 1 < 0) {
            fits = false;
        }

		// checks if the pixels in the the block are close to the reference
        if (fits) {
            for (int y = py; y >= py - pixelSizes[i] + 1 && fits; --y) {
                for (int x = px; x < px + pixelSizes[i] && fits; ++x) {
                    if (!comparePixels(data, referencePixel, (y * wid + x) * ch)) {
                        fits = false;
                    }
                }
            }
        }

        // if the larger pixel doesnt fit then it will place the next best one
        if (!fits) {
            // gets the pixel that will be placed, if this is the first loop then we just place the small one
            int sizeP = (i == 0) ? 1 : pixelSizes[i - 1];
            int returnID = (i == 0) ? pixelObjID : pixelObjIDs[i - 1];

            // marks the pixels as placed
            for (int y = 0; y < sizeP; ++y) {
                for (int x = 0; x < sizeP; ++x) {
                    p[py - y][px + x] = true;
                }
            }
            return returnID;
        }
    }

    // this is for the largest pixel, marks them as placed
    for (int y = 0; y < pixelSizes.back(); ++y) {
        for (int x = 0; x < pixelSizes.back(); ++x) {
            p[py - y][px + x] = true;
        }
    }
    return pixelObjIDs.back();
}

// compares 2 pixels
bool CreateArt::comparePixels(unsigned char*& data, int p1, int p2) {
    return (std::abs(data[p1] - data[p2]) <= tolerance)
        && (std::abs(data[p1 + 1] - data[p2 + 1]) <= tolerance)
        && (std::abs(data[p1 + 2] - data[p2 + 2]) <= tolerance);
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
    r /= 255.0f;
    g /= 255.0f;
    b /= 255.0f;
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

void CreateArt::updateSettings() {
	limitSize = Mod::get()->getSettingValue<bool>("Disable-limit");
	sizeLimit = Mod::get()->getSettingValue<int>("Size-limit");
	colourChannel = Mod::get()->getSettingValue<int>("Colour-channel");
	useOldPixel = Mod::get()->getSettingValue<bool>("Use-OlderObjects");
	tolerance = Mod::get()->getSettingValue<int>("Tolerance");
    basic = Mod::get()->getSettingValue<std::string>("Optimise-Type");
}