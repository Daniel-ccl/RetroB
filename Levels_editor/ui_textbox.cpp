#include "ui_textbox.h"
#include <cmath>

void TextBoxClick(TextBox& tb, Vector2 mousePos) {
    tb.activo = CheckCollisionPointRec(mousePos, tb.rect);
}

void TextBoxTeclado(TextBox& tb) {
    if (!tb.activo) return;

    int tecla = GetCharPressed();
    while (tecla > 0) {
        if (tecla >= 32 && tecla <= 125 && (int)tb.text.size() < tb.maxLen) {
            tb.text += (char)tecla;
        }
        tecla = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !tb.text.empty()) {
        tb.text.pop_back();
    }
}

void TextBoxDibujar(const TextBox& tb, const char* label, Color colorBorde) {
    DrawText(label, (int)tb.rect.x, (int)tb.rect.y - 18, 12, LIGHTGRAY);
    DrawRectangleRec(tb.rect, (Color){20, 20, 30, 255});

    Color borde = tb.activo ? WHITE : colorBorde;
    DrawRectangleLinesEx(tb.rect, tb.activo ? 2.0f : 1.0f, borde);
    DrawText(tb.text.c_str(), (int)tb.rect.x + 6, (int)tb.rect.y + 7, 16, WHITE);

    if (tb.activo) {
        int textW = MeasureText(tb.text.c_str(), 16);
        float cursorX = tb.rect.x + 6 + textW + 2;
        if (fmodf((float)GetTime(), 1.0f) < 0.5f) {
            DrawLine((int)cursorX, (int)(tb.rect.y + 6),
                     (int)cursorX, (int)(tb.rect.y + tb.rect.height - 6), WHITE);
        }
    }
}
