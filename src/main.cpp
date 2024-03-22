#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/Bindings.hpp>
#include <cocos2d.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <Geode/modify/GameObject.hpp>
#include <Geode/binding/GameObject.hpp>
#include <algorithm>
#include <vector>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace geode::prelude;

class $modify(MyEditorUI, EditorUI) {


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
						// opens file explorer
						geode::utils::file::pickFile(geode::utils::file::PickMode::OpenFile, 
						{"", }, [this](ghc::filesystem::path const& path){
							// checks if a file is selected
							if (!path.empty()){
								// makes the path a string
								std::string imagePath = path.string();
								CreateArt(path.string());
							}
							else{
								FLAlertLayer::create("Error", "This file did not work!", "OK")->show();
							}
						});
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

	void CreateArt(std::string const& path){
		// the id of the object, bacl colour channel, z order
		int objID = 3097;
		int colourChannel = 1010;
		int zOrder = 1;
		int width;
		int height;
		int channels;
		// gets the start pos and set size/scale
		float startX = m_selectedObject->m_realXPosition;
		float startY = m_selectedObject->m_realYPosition;
		float objSize = 5;
		float scale = 2.5f;
		// will contain the formated level string
		std::string objString;
		// loads the image from a path
    	unsigned char *data = stbi_load(path.c_str(), &width, &height, &channels, 0);

		    try {
				// gets image data
        		auto imageData = loadImage(path);
				// gets the pixel count of the image
				int pixelCount = width * height;
				// checks if the image is to big
				if(pixelCount <= 40000){
					std::stringstream objInLevel;
					// loops through each pixel
					for (int i = 0; i < pixelCount; ++i){
						// calculate pos
						int pixelIndex = i * channels;
						uint8_t alpha = (channels == 4) ? data[pixelIndex + 3] : 255;
						// checks if there is a pixel (mainly for png)
						if(alpha != 0){

							uint8_t red = data[pixelIndex];
							uint8_t green = data[pixelIndex + 1];
							uint8_t blue = data[pixelIndex + 2];
							
							// converts colours
							std::string objColour = formatHsvToString(red, green, blue);
							// the object id
							objInLevel <<"1,3097,";
							// x location
							objInLevel << "2," << startX + (i % width) * scale << ",";
							// y location
							objInLevel << "3," << startY - (i / width) * scale << ",";
							// sets the colour channel to black
							objInLevel <<"21," << colourChannel;
							// the scale 5 is 1X scale
							objInLevel <<",32," << objSize;
							// for hsv
							objInLevel <<",41,1";
							objInLevel <<",43," << objColour;
							// z order
							objInLevel << ",25," << zOrder << ";";
						}
						// when the end of the row is reached
						if ((i + 1) % width == 0) {
							// reset x
							startX = m_selectedObject->m_realXPosition;
							// move down
							startY -= scale;
						} else {
							// move right
							startX += scale;
						}
					}
					objString = objInLevel.str();
					// removes the last character
					objString.pop_back();
					// updates the editor
					auto editorlayrn = LevelEditorLayer::get();
					editorlayrn->createObjectsFromString(objString.c_str(), true, true);
					// it worked
					FLAlertLayer::create("Success!", "<cg>Art was imported</c>", "OK")->show();
				}else{
					// did not work ):
					FLAlertLayer::create("Error!", "<cr>Image is too big</c>", "OK")->show();
				}

			} catch (std::exception const& e) {
				// it failed ):
				FLAlertLayer::create("Error", "<cr>File not valid</c>", "OK")->show();
			}
			stbi_image_free(data);
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
	}

	// makes hsv a string
	std::string formatHsvToString(int red, int green, int blue) {
		float h;
		float s;
		float v;
		rgbToHsv(red, green, blue, h, s, v);;
		// returns the formated data
		return std::to_string(h) + "a" + std::to_string(s)
		 + "a" + std::to_string(v) + "a" + "1a1";
	}
};

