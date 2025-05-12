#ifndef COLOR_H
#define COLOR_H

struct Color {
  char r;
  char g;
  char b;
  Color(char r, char g, char b) {
    this->r = r;
    this->g = g;
    this->b = b;
  }
};

// Colors
Color colorOff        = Color(  0,   0,   0);
Color colorRed        = Color(255,   0,   0);
Color colorGreen      = Color(  0, 255,   0);
Color colorBlue       = Color(  0,   0, 255);
Color colorPurple     = Color(100,   0, 100);
Color colorCyan       = Color(  0, 100, 100);
Color colorDimOrange  = Color(255,  35,   0);
Color colorWhite      = Color(255, 255, 255);

#endif
