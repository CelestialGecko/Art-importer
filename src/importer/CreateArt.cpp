#define STB_IMAGE_IMPLEMENTATION
#include "CreateArt.h"

void CreateArt::importArt() {
    async::spawn(file::pick(file::PickMode::OpenFile, ALLOWED_TYPES2),
        [this](Result<std::optional<std::filesystem::path>> result) {
            if (result.isOk()) {
                auto opt = result.unwrap();
                if (opt) {
                    std::string const pathStr = utils::string::pathToString(opt.value());
                    this->placeArt(pathStr);
                }
                else {
                }
            }
		}
    );
}

// places the art, will choose a specific method base on settings
void CreateArt::placeArt(std::string const& p) {
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

void CreateArt::createObjects(std::string const& objString) {
    auto editorUI = EditorUI::get();
    auto objects = editorUI->pasteObjects(objString.c_str(), false, false);
    
    /*
    1. First undo deselects the imported image and replaces and reselects the placeholder object
    2. Second undo deletes the imported image
        This is so if someone wants to use the same object to import multiple images in the same or similar
        places, they can do that by pressing undo.
    */
    editorUI->deselectAll();
	editorUI->selectObject(obj, false);
	editorUI->createUndoSelectObject(true);
	editorUI->deselectAll();
	editorUI->selectObjects(objects, false);

    auto baseLayer = GJBaseGameLayer::get();
    baseLayer->groupStickyObjects(objects); // links the art together

    auto editorLayer = LevelEditorLayer::get();
    editorLayer->removeObject(obj, false); // Removes the placeholder object
}

// just a normal import with none of this woke optimisation stuff
void CreateArt::simpleImport(std::string const& p) {
    int height;
    int channels;
    int width;
    unsigned char* data = nullptr;
    float const startX = obj->getPositionX();
    float const startY = obj->getPositionY();
    std::ostringstream objInLevel;
    std::string objString;

    try {
        // gets image data
        data = stbi_load(p.c_str(), &width, &height, &channels, 0);

        // checks the size or if the size limit is on
        if ((width * height > sizeLimit) && !limitSize) {
            FLAlertLayer::create("Error", "Image cannot be bigger than " + std::to_string(sizeLimit) + ".\nChange this in the settings menu.", "OK")->show();
            stbi_image_free(data);
            data = nullptr;
            return;
        }

        // if data doesnt exist or doesnt work
        if (!data) {
            FLAlertLayer::create("Error", "Failed to load image.", "OK")->show();
            stbi_image_free(data);
            data = nullptr;
            return;
        }

        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                // gets the index for the current pixel being looked at
                int const pixelIndex = (y * width + x) * channels;
                // gets alpha
                uint8_t const alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
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
        createObjects(objString);
        FLAlertLayer::create("Success!", "Art was imported", "OK")->show();
        stbi_image_free(data);
        data = nullptr;
        // this is too scary for me not to check
        closeMenu();
    }
    catch (std::exception const& e) {
        stbi_image_free(data);
        data = nullptr;
        FLAlertLayer::create("Error", e.what(), "OK")->show();
    }
}

// basic optimisation using 4 different pixel objects from the pixel art tab
void CreateArt::basicOptimiseImport(const std::string& p) {
    int height;
    int channels;
    int width;
    unsigned char* data = nullptr;
    float const startX = obj->getPositionX();
    float const startY = obj->getPositionY();
    std::ostringstream objInLevel;
    std::string objString;

    try {
        // gets image data
        data = stbi_load(p.c_str(), &width, &height, &channels, 0);

        // checks the size or if the size limit is on
        if ((width * height > sizeLimit) && !limitSize) {
            FLAlertLayer::create("Error", "Image cannot be bigger than " + std::to_string(sizeLimit) + ".\nChange this in the settings menu.", "OK")->show();
            stbi_image_free(data);
            data = nullptr;
            return;
        }

        // if data doesnt exist or doesnt work
        if (!data) {
            FLAlertLayer::create("Error", "Failed to load image.", "OK")->show();
            stbi_image_free(data);
            data = nullptr;
            return;
        }
        // used to determine if a pixel should be placed
        std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));

        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
				if (placed[y][x]) continue;
                // gets the index for the current pixel being looked at
                int const pixelIndex = (y * width + x) * channels;
                // gets alpha
                uint8_t const alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
                // doesnt place anything if empty
                if (alpha == 0)continue;

                // decides which pixel fits the best
				int const pixelType = bestFit(placed, data, x, y, channels, width, height);

                // gets the colour in GD format
                std::string objColour;
                formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);

                // places the object depending on which one works the best
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
        createObjects(objString);
        FLAlertLayer::create("Success!", "Art was imported", "OK")->show();
        stbi_image_free(data);
        data = nullptr;
        // this is too scary for me not to check
        closeMenu();
    }
    catch (std::exception const& e) {
        stbi_image_free(data);
        data = nullptr;
        FLAlertLayer::create("Error", e.what(), "OK")->show();
    }
}

// uses scaling to optimise the art
void CreateArt::scaleOptimiseImport(std::string const& p) {
    int height;
    int channels;
    int width;
    unsigned char* data = nullptr;
    float const startX = obj->getPositionX();
    float const startY = obj->getPositionY();
    std::ostringstream objInLevel;
    std::string objString;
    try {
        // gets image data
        data = stbi_load(p.c_str(), &width, &height, &channels, 0);

        // checks the size or if the size limit is on
        if ((width * height > sizeLimit) && !limitSize) {
            FLAlertLayer::create("Error", "Image cannot be bigger than " + std::to_string(sizeLimit) + ".\nChange this in the settings menu.", "OK")->show();
            stbi_image_free(data);
            data = nullptr;
            return;
        }

        // if data doesnt exist or doesnt work
        if (!data) {
            FLAlertLayer::create("Error", "Failed to load image.", "OK")->show();
            stbi_image_free(data);
            data = nullptr;
            return;
        }
        // used to determine if a pixel should be placed
        std::vector<std::vector<bool>> placed(height, std::vector<bool>(width, false));
        for (int y = height - 1; y >= 0; --y) {
            for (int x = 0; x < width; ++x) {
                if (placed[y][x]) continue;
                // gets the index for the current pixel being looked at
                int const pixelIndex = (y * width + x) * channels;
                // gets alpha
                uint8_t const alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
                // doesnt place anything if empty
                if (alpha == 0)continue;

                // gets the 2 offsets used to determine the size and pixel pos
				int scaleX = scalePixX(placed, data, x, y, channels, width, height);
				int scaleY = scalePixY(placed, data, x, y, channels, width, height, scaleX);

                // gets the colour in GD format
                std::string objColour;
                formatHSV(data[pixelIndex], data[pixelIndex + 1], data[pixelIndex + 2], objColour);

                float const xSize = scale * scaleX;
                float const ySize = scale * scaleY;

                // places the pixel that was chosen
                if (useOldPixel) {
                    objInLevel << "1," << oldLargePixelObjID << ",2," << startX + x * scale + xSize / 2 << ",3," <<
                        startY - y * scale + ySize / 2 << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",128," << (objSize / 30) * scaleX
                        << ",129," << (objSize / 30) * scaleY << ";";
                }
                else {

                    objInLevel << "1," << largePixelObjID << ",2," << startX + x * scale + xSize / 2 << ",3," <<
                        startY - y * scale + ySize / 2 << ",21," << colourChannel << ",41,1,43," <<
                        objColour << ",25," << zOrder << ",128," << xSize
                        << ",129," << ySize << ";";
                }
            }
        }

        // removes the last ;
        objString = objInLevel.str();
        objString.pop_back();
        // adds the new objects to the level and then prompts the user
        createObjects(objString);
        FLAlertLayer::create("Success!", "Art was imported", "OK")->show();
        stbi_image_free(data);
        data = nullptr;
        // this is too scary for me not to check
        closeMenu();
    }
    catch (std::exception const& e) {
        stbi_image_free(data);
        data = nullptr;
        FLAlertLayer::create("Error", e.what(), "OK")->show();
    }
}

// scales on the x
int CreateArt::scalePixX(std::vector<std::vector<bool>>& p, unsigned char const* data, int px, int py, int ch, int wid, int hi) {
    int const refPix = (py * wid + px) * ch;
	bool fits = true;
	int off = 1;
    // will loop until it cant place an extra pixel or reaches the width
    do {
        // checks if on the width
        if (px + off >= wid) {
			fits = false;
        }
        else {
            // compares and then offsets if pixel is close enough
            if (comparePixels(data, refPix, (py * wid + px + off) * ch)) {
                p[py][px + off] = true;
                ++off;
            }
            else {
                fits = false;
            }
        }
    } while (fits);
    //sets the original as placed
	p[py][px] = true;
	return off;
}

// scales on the y
int CreateArt::scalePixY(std::vector<std::vector<bool>>& p, unsigned char const* data, int px, int py, int ch, int wid, int hi, int xScale) {
    int const refPix = (py * wid + px) * ch;
	bool fits = true;
	int off = 1;
    // will loop until out of range or is unable to place another layer
	do {
		if (py - off <= 0) {
			fits = false;
		}
        else {
            // loops through the pixels for a given row
            // if 1 pixel doesnt match then this layer cannot be filled
            for (int i = 0; i < xScale && fits; ++i) {
                if (!comparePixels(data, refPix, ((py - off) * wid + px + i) * ch)) {
                    fits = false;
                }
            }
			// if it fits then we mark all the spaces as placed
            // we also move the offset down to check the next row
            if (fits) {
                for (int i = 0; i < xScale; ++i) {
                    p[py - off][px + i] = true;
                }
                ++off;
            }
        }
	} while (fits);
    return off;
}

// finds the best pixel object to reduce object count
int CreateArt::bestFit(std::vector<std::vector<bool>>& p, const unsigned char* data, int px, int py, int ch, int wid, int hi) {
    int const referencePixel = (py * wid + px) * ch;
    // storing the variations for the pixels in a array
    std::vector<int> const pixelSizes = { 1, 2, 3, 6 };
    std::vector<int> const pixelObjIDs = { pixelObjID, medPixelObjID, bigPixelObjID, largePixelObjID };

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
            int const sizeP = (i == 0) ? 1 : pixelSizes[i - 1];
            int const returnID = (i == 0) ? pixelObjID : pixelObjIDs[i - 1];

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
bool CreateArt::comparePixels(const unsigned char*& data, int p1, int p2) const {
    return (std::abs(data[p1] - data[p2]) <= tolerance)
        && (std::abs(data[p1 + 1] - data[p2 + 1]) <= tolerance)
        && (std::abs(data[p1 + 2] - data[p2 + 2]) <= tolerance);
}

// formats the hasv values in a way that GD can understand
void CreateArt::formatHSV(float r, float g, float b, std::string& objColour) const {
	RGBtoHSV(r, g, b);
    g = (g * 100) / 100;
    b = (b * 100) / 100;

	// formats the HSV values into a string
	objColour = std::to_string(r) + "a" + std::to_string(g)
        + "a" + std::to_string(b) + "a" + "1a1";
}

// faster version that I made for university
void CreateArt::RGBtoHSV(float& r, float& g, float& b) const {
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
    // for some reason hue 0 is weird
    if (hue == 0.0f) hue++;
    r = hue;
    g = sat;
    b = vibe;
}

// updates settings while in editor
void CreateArt::updateSettings() {
	limitSize = Mod::get()->getSettingValue<bool>("Disable-limit");
	sizeLimit = Mod::get()->getSettingValue<int>("Size-limit");
	colourChannel = Mod::get()->getSettingValue<int>("Colour-channel");
	useOldPixel = Mod::get()->getSettingValue<bool>("Use-OlderObjects");
	tolerance = Mod::get()->getSettingValue<int>("Tolerance");
    basic = Mod::get()->getSettingValue<std::string>("Optimise-Type");
}