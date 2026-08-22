#ifndef UI_TEXTBOX_H
#define UI_TEXTBOX_H

#include "raylib.h"
#include <string>

struct TextBox {
    Rectangle   rect;
    std::string text;
    bool        activo;
    int         maxLen;
};

void TextBoxClick(TextBox& tb, Vector2 mousePos);

void TextBoxTeclado(TextBox& tb);

void TextBoxDibujar(const TextBox& tb, const char* label, Color colorBorde);

#endif
