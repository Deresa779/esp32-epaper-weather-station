#ifndef OUTPUT_H_
#define OUTPUT_H_

#include <Arduino.h>
#include "icon.h"

typedef struct {
    const unsigned char* image_b;  // Black image data
    const unsigned char* image_r;  // Red image data
    int colortype;                 // 1: black, 2: red, 3: both
} imagedataType;

struct IconImageData {
    const char* code;
    const unsigned char* blackImage;
    const unsigned char* redImage;
};

imagedataType find_icon_data(const String& iconcode);
imagedataType iconcode_to_icon(String& iconcode, int& Id);

#endif /* OUTPUT_H_ */