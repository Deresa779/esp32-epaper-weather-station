#include "output_image.h"
#include "icon.h"
#include "Arduino.h"

// Array that stores the mapping of iconcode to corresponding image data
IconImageData iconImageMap[] = {
    {"01d", nullptr, gImage_01dr}, 
    {"01n", gImage_01nb, nullptr}, 
    {"02d", gImage_02db, gImage_02dr},
    {"02n", gImage_02nb, nullptr},
    {"03d", gImage_03b, nullptr},
    {"03n", gImage_03b, nullptr},
    {"04d", gImage_04b, nullptr},
    {"04n", gImage_04b, nullptr},
    {"09d", gImage_09b, nullptr},
    {"09n", gImage_09b, nullptr},
    {"10d", gImage_10db, gImage_10dr},
    {"10n", gImage_10nb, nullptr},
    {"11d", gImage_11b, gImage_11r},
    {"11n", gImage_11b, gImage_11r},
    {"13d", gImage_13b, nullptr},
    {"13n", gImage_13b, nullptr},
    {"50d", gImage_50b_or_r, nullptr},
    {"50n", gImage_50b_or_r, nullptr}
};

// Function to find image data for the given iconcode using linear search
imagedataType find_icon_data(const String& iconcode) {
    imagedataType icondata;
    Serial.println(iconcode);
    int i = 0;
    const int numIcons = sizeof(iconImageMap) / sizeof(iconImageMap[0]);
    while (i < numIcons) {
        if (strcmp(iconImageMap[i].code, iconcode.c_str()) == 0) {
            // Return matched black and red image data
            icondata.image_b = iconImageMap[i].blackImage;
            icondata.image_r = iconImageMap[i].redImage;
            // Set the color type based on available images
            if (icondata.image_b != nullptr && icondata.image_r != nullptr) {
                icondata.colortype = 3;  // Both black and red images
            } else if (icondata.image_r != nullptr) {
                icondata.colortype = 2;  // Only red image
            } else {
                icondata.colortype = 1;  // Only black image
            }
            return icondata;
            break;
        }
        i++;
    }

    // If no match is found, return default uninitialized icondata
    icondata.image_b = nullptr;
    icondata.image_r = nullptr;
    icondata.colortype = 0;
    return icondata;
}

// Main function to map iconcode to icon data
imagedataType iconcode_to_icon(String& iconcode, int& Id) {
    imagedataType icondata = find_icon_data(iconcode);
    Serial.print("id = ");
    Serial.println(Id);
    if (icondata.colortype==1) {
        // In special cases, the icons for smog, sandstorms, etc. in the output information of the web page API are output in black, but here I want them to be displayed in red
        if (Id > 710 && Id < 732 || Id > 750 && Id < 782) {
            icondata.image_r = gImage_50b_or_r;  
            icondata.image_b = nullptr;
            icondata.colortype = 2;
        }    
    } 
    return icondata;
}
