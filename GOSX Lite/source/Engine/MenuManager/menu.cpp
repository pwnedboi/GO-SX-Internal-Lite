    //
    //  menu.cpp
    //  GOSX Pro
    //
    //  Created by Andre Kalisch on 02.03.17.
    //  Copyright © 2017 Andre Kalisch. All rights reserved.
    //

#include "menu.h"
#include "Engine/FeatureManager/Features/ClantagChanger.h"
#include "Engine/FeatureManager/Features/Chams.h"
#include "Engine/FeatureManager/Features/Aim.h"
#include "Engine/FeatureManager/Features/AntiAim.h"
#include "Engine/FeatureManager/Features/EnginePrediction.h"
#include "KeyStroke.h"

std::shared_ptr<CHackMenu> CHackMenu::instance = nullptr;

CHackMenu::CHackMenu() {
    dMgr = CDrawings::Instance();
}

int CHackMenu::GetCursorPosition() {
    return CursorPosition;
}

void CHackMenu::SetCursorPosition(int position) {
    CursorPosition = position;
}

int CHackMenu::GetItemCount() {
    return ItemIndex;
}

bool CHackMenu::IsMenuOpen() {
    return MenuOpen;
}

bool CHackMenu::IsWaitingForInput() {
    return keyInputWaiting;
}

void CHackMenu::OpenMenu(bool init) {
    Interfaces::Engine()->ExecuteClientCmd("cl_mouseenable 0");
    Interfaces::Surface()->UnlockCursor();
    MenuOpen = true;
}

void CHackMenu::CloseMenu() {
    Interfaces::Engine()->ExecuteClientCmd("cl_mouseenable 1");
    Interfaces::Surface()->LockCursor();
    MenuOpen = false;
}

void CHackMenu::HandleInput(int keynum) {
    if (keynum == ButtonCode_t::KEY_ENTER) {

    } else if (keynum == ButtonCode_t::KEY_SPACE) {
        m_szCurrentString[activeInput].append(" ");
        m_nCurrentPos[activeInput]++;
    } else if (keynum == ButtonCode_t::KEY_BACKSPACE) {
        std::size_t nLength = m_szCurrentString[activeInput].length();
        if (nLength > 0) {
            m_szCurrentString[activeInput].erase(nLength - 1);
            m_nCurrentPos[activeInput]--;
        }
    } else if (
               keynum >= ButtonCode_t::KEY_0 &&
               keynum <= ButtonCode_t::KEY_EQUAL &&
               m_szCurrentString[activeInput].length() <= 114
               ) {

        if (Functions::IsKeyPressed(kVK_Shift) || Functions::IsKeyPressed(kVK_RightShift)) {
            m_szCurrentString[activeInput].append(m_KeyStroke[keynum].m_szShiftDefinition);
            m_nCurrentPos[activeInput]++;
        } else if (Functions::IsKeyPressed(kVK_Option) || Functions::IsKeyPressed(kVK_Option)) {
            m_szCurrentString[activeInput].append(m_KeyStroke[keynum].m_szAltDefinition);
            m_nCurrentPos[activeInput]++;
        } else {
            m_szCurrentString[activeInput].append(m_KeyStroke[keynum].m_szDefinition);
            m_nCurrentPos[activeInput]++;
        }
    }
}

void CHackMenu::HandleKey(int keynum) {
    if (keynum != 0 && lastKey != keynum && keyInputWaiting) {
        lastKey = keynum;
        keyInputWaiting = false;
    }
}

void CHackMenu::SetMouseState(bool released) {
    if (released) {
        mouseReleased = true;
    }
}

void CHackMenu::SetMousePressed(bool value) {
    if (value) {
        mousePressed = true;
    }
}

void CHackMenu::DrawMenu() {
    ItemIndex = 1;
    InputIndex = 1;
    selectIndex = 1;
    keyInputIndex = 1;
    baseWidth = MENUGET_INT("Sizes", "menu_base_w");
    baseHeight = MENUGET_INT("Sizes", "menu_base_h");
    int sw, sh;
    Interfaces::Engine()->GetScreenSize(sw, sh);

    int halfScreenW = sw / 2;
    int halfScreenH = sh / 2;
    int halfBaseW = baseWidth / 2;
    int halfBaseH = baseHeight / 2;

    baseX = halfScreenW - halfBaseW;
    baseY = halfScreenH - halfBaseH;

    dMgr->DrawRect(baseX, baseY, baseWidth, baseHeight, MENUGET_COLOR("Colors", "background"));
    DrawHeader();
    tabCount = 1;

    CSimpleIniA::TNamesDepend sections;
    CSettingsManager::Instance("settings.ini")->GetAllSections(sections);
    sections.sort(CSimpleIniA::Entry::LoadOrder());
    for (auto section : sections) {
        if (!section.pComment || strcmp(section.pComment, "; ignore")) {
            AddTab(section.pItem);
            if (tabIndex[section.pItem] == activeTab) {
                CSimpleIniA::TNamesDepend keys;
                CSettingsManager::Instance("settings.ini")->GetAllKeys(section.pItem, keys);
                keys.sort(CSimpleIniA::Entry::LoadOrder());
                int column = 1;
                int counter = 1;
                for (auto key : keys) {
                    if (!key.pComment || strcmp(key.pComment, "; ignore")) {
                        FieldType_t fieldType = FieldType_t::FIELDTYPE_INPUT;
                        if (key.pComment && !strcmp(key.pComment, "; bool")) {
                            fieldType = FieldType_t::FIELDTYPE_BOOL;
                        } else if (key.pComment && !strcmp(key.pComment, "; float")) {
                            fieldType = FieldType_t::FIELDTYPE_FLOAT;
                        } else if (key.pComment && !strcmp(key.pComment, "; int")) {
                            fieldType = FieldType_t::FIELDTYPE_INT;
                        } else if (key.pComment && !strcmp(key.pComment, "; color")) {
                            fieldType = FieldType_t::FIELDTYPE_COLOR;
                        } else if (key.pComment && !strcmp(key.pComment, "; string")) {
                            fieldType = FieldType_t::FIELDTYPE_INPUT;
                        } else if (key.pComment && !strcmp(key.pComment, "; select")) {
                            fieldType = FieldType_t::FIELDTYPE_SELECT;
                        } else if (key.pComment && !strcmp(key.pComment, "; key")) {
                            fieldType = FieldType_t::FIELDTYPE_KEY;
                        }

                        AddMenuItem(section.pItem, key.pItem, column, counter, fieldType);
                        if (fieldType == FieldType_t::FIELDTYPE_COLOR) {
                            counter++;
                        }

                        counter++;
                        if (counter > MENUGET_INT("Misc", "max_items_per_row")) {
                            column++;
                            counter = 1;
                        }
                    }
                }
            }
        }
    }

    DrawActiveSelect();

    int mox, moy;
    Interfaces::Surface()->SurfaceGetCursorPos(mox, moy);
    DrawMouseCursor(mox, moy);
    mouseReleased = false;
}

void CHackMenu::DrawMouseCursor(int mx, int my) {
    Color MouseColor = MENUGET_COLOR("Colors", "mouse_pointer");
    dMgr->DrawLine(mx,      my,     mx,      my +  2, MouseColor);
    dMgr->DrawLine(mx +  1, my,     mx +  1, my +  4, MouseColor);
    dMgr->DrawLine(mx +  2, my + 1, mx +  2, my +  6, MouseColor);
    dMgr->DrawLine(mx +  3, my + 1, mx +  3, my +  8, MouseColor);
    dMgr->DrawLine(mx +  4, my + 2, mx +  4, my + 10, MouseColor);
    dMgr->DrawLine(mx +  5, my + 2, mx +  5, my + 12, MouseColor);
    dMgr->DrawLine(mx +  6, my + 3, mx +  6, my + 14, MouseColor);
    dMgr->DrawLine(mx +  7, my + 3, mx +  7, my + 16, MouseColor);
    dMgr->DrawLine(mx +  8, my + 4, mx +  8, my + 18, MouseColor);
    dMgr->DrawLine(mx +  9, my + 4, mx +  9, my + 20, MouseColor);
    dMgr->DrawLine(mx + 10, my + 5, mx + 10, my + 19, MouseColor);
    dMgr->DrawLine(mx + 11, my + 5, mx + 11, my + 18, MouseColor);
    dMgr->DrawLine(mx + 12, my + 6, mx + 12, my + 17, MouseColor);
    dMgr->DrawLine(mx + 13, my + 6, mx + 13, my + 16, MouseColor);
    dMgr->DrawLine(mx + 14, my + 7, mx + 14, my + 15, MouseColor);
    dMgr->DrawLine(mx + 15, my + 7, mx + 15, my + 14, MouseColor);
    dMgr->DrawLine(mx + 16, my + 8, mx + 16, my + 13, MouseColor);
    dMgr->DrawLine(mx + 17, my + 8, mx + 17, my + 12, MouseColor);
    dMgr->DrawLine(mx + 18, my + 9, mx + 18, my + 11, MouseColor);
    dMgr->DrawLine(mx + 19, my + 9, mx + 19, my + 10, MouseColor);
}

bool CHackMenu::IsMouseOverThis(int x, int y, int w, int h, bool isSelect) {
    if (!isSelect && IsSelectOpen) {
        return false;
    }

    int mox, moy;
    Interfaces::Surface()->SurfaceGetCursorPos(mox, moy);

    if (
        mox >= x &&
        mox <= x + w &&
        moy >= y &&
        moy <= y + h
        ) {
        return true;
    } else {
        return false;
    }
}

void CHackMenu::AddTab(const char* section) {
    int TabsWidth = MENUGET_INT("Sizes", "tabs_w");
    int TabsHeight = MENUGET_INT("Sizes", "tabs_h");

    int sectionBaseX = baseX + 5;
    int sectionBaseY = baseY + (TabsHeight * tabCount) - 1;

    Color background = MENUGET_COLOR("Colors", "item_background");
    Color indColor = MENUGET_COLOR("Colors", "colored_normal");
    if (activeTab == tabCount) {
        background = Color(background.r(), background.g(), background.b(), 150);
        indColor = MENUGET_COLOR("Colors", "colored_highlight");
    }
    if (IsMouseOverThis(sectionBaseX, sectionBaseY, TabsWidth, TabsHeight)) {
        background = Color(background.r(), background.g(), background.b(), 150);
        indColor = MENUGET_COLOR("Colors", "colored_highlight");
        if (mouseReleased) {
            activeTab = tabCount;
            mouseReleased = false;
        }
    }

    dMgr->DrawRect(sectionBaseX, sectionBaseY, TabsWidth, TabsHeight - 2, background);
    dMgr->DrawShadowString(1, sectionBaseX + 14, sectionBaseY + 12, MENUGET_COLOR("Colors", "label_text"), false, section);

    indColor.SetA(125);
    dMgr->DrawRect(sectionBaseX, sectionBaseY, 10, TabsHeight - 2, indColor, true);

    tabIndex[section] = tabCount;
    indexTab[tabCount] = section;

    tabCount++;
}

void CHackMenu::AddMenuItem(const char *section, const char* key, int col, int row, FieldType_t type) {
    int ButtonWidth = MENUGET_INT("Sizes", "button_w");
    int ButtonHeight = MENUGET_INT("Sizes", "button_h");

    int ButtonBaseX = baseX + MENUGET_INT("Sizes", "tabs_w") + 5 + ((ButtonWidth + 5) * (col - 1)) + 5;
    int ButtonBaseY = baseY + ((ButtonHeight + 5) * row);

    if (type == FieldType_t::FIELDTYPE_COLOR) {
        ButtonHeight *= 2;
        ButtonHeight += 5;
    }

    dMgr->DrawRect(ButtonBaseX, ButtonBaseY, ButtonWidth, ButtonHeight, MENUGET_COLOR("Colors", "item_background"));
    dMgr->DrawShadowString(1, ButtonBaseX + 18, ButtonBaseY + 9, MENUGET_COLOR("Colors", "label_text"), false, key);

    if (type == FieldType_t::FIELDTYPE_BOOL) {
        int CheckboxBaseX = ButtonBaseX + ButtonWidth - 21;
        int CheckboxBaseY = ButtonBaseY + 5;

        dMgr->DrawOutlineRect(CheckboxBaseX, CheckboxBaseY, 16, 16, MENUGET_COLOR("Colors", "label_text"));
        dMgr->DrawOutlineRect(CheckboxBaseX + 1, CheckboxBaseY + 1, 14, 14, MENUGET_COLOR("Colors", "label_text"));

        bool value = INIGET_BOOL(section, key);
        if (IsMouseOverThis(ButtonBaseX, ButtonBaseY, ButtonWidth, ButtonHeight) && mouseReleased) {
            value = !value;
            CSettingsManager::Instance("settings.ini")->SetBoolValue(section, key, value);
            mouseReleased = false;
        }

        if (value) {
            dMgr->DrawRect(CheckboxBaseX + 4, CheckboxBaseY + 4, 8, 8, MENUGET_COLOR("Colors", "colored_normal"), true);
        }
    } else if (type == FieldType_t::FIELDTYPE_FLOAT) {
        DrawSlider(section, key, ButtonBaseX, ButtonBaseY, INIGET_FLOAT(section, key), INIGET_FLOAT("MinValues", key), INIGET_FLOAT("MaxValues", key));
    } else if (type == FieldType_t::FIELDTYPE_INT) {
        DrawPercentSlider(section, key, ButtonBaseX, ButtonBaseY, INIGET_INT(section, key), 0, 100);
    } else if (type == FieldType_t::FIELDTYPE_COLOR) {
        DrawColorSet(section, key, ButtonBaseX, ButtonBaseY, INIGET_COLOR(section, key));
    } else if (type == FIELDTYPE_INPUT) {
        m_nCurrentPos[InputIndex] = 0;
        DrawInputField(section, key, ButtonBaseX, ButtonBaseY, INIGET_STRING(section, key));
    } else if (type == FieldType_t::FIELDTYPE_SELECT) {
        std::map<int, const char*> list;
        std::map<int, const char*> values;
        if(!strcmp(section, "AimHelper")) {
            list = CAim::GetBoneList();
            values = CAim::GetBoneValues();
        }
        if(!strcmp(section, "ClantagChanger")) {
            list = CClantagChanger::GetList();
            values = CClantagChanger::GetValues();
        }
        if(!strcmp(section, "Chams") && !strcmp(key, "chams_type")) {
            list = CChams::GetChamList();
            values = CChams::GetChamValues();
        }
        if (!strcmp(section, "Rage") && !strcmp(key, "antiaim_pitch")) {
            list = CAntiAim::GetPitchList();
            values = CAntiAim::GetPitchValues();
        }
        if (!strcmp(section, "Rage") && !strcmp(key, "antiaim_yaw")) {
            list = CAntiAim::GetYawList();
            values = CAntiAim::GetYawValues();
        }
        if (!strcmp(section, "Rage") && !strcmp(key, "engine_predict_mode")) {
            list = CEnginePrediction::GetModeList();
            values = CEnginePrediction::GetModeValues();
        }
        DrawSelectField(section, key, ButtonBaseX, ButtonBaseY, INIGET_INT(section, key), list, values);
    } else if (type == FieldType_t::FIELDTYPE_KEY) {
        DrawKeyField(section, key, ButtonBaseX, ButtonBaseY, INIGET_INT(section, key));
    }

    menuIndex[key] = ItemIndex;
    indexMenu[ItemIndex] = key;

    ItemIndex++;
}

void CHackMenu::DrawHeader() {
    dMgr->DrawRect(baseX, baseY, baseWidth, MENUGET_INT("Sizes", "header_h"), MENUGET_COLOR("Colors", "header_background"), true);
    if (!headerFont) {
        headerFont = dMgr->GetFont(MENUGET_STRING("Main", "menu_font"), 18, FontFlags::FONTFLAG_ANTIALIAS);
    }
    dMgr->DrawShadowString(headerFont, baseX + 10, baseY + 8, MENUGET_COLOR("Colors", "header_text"), false, "GO:SX Lite Settings");
}

void CHackMenu::DrawSaveButton() {
    int SaveButtonWidth = MENUGET_INT("Sizes", "save_button_w");
    int SaveButtonHeight = MENUGET_INT("Sizes", "save_button_h");

    int SaveButtonBaseX = (baseX + baseWidth) - (SaveButtonWidth + 10);
    int SaveButtonBaseY = (baseY + baseHeight);

    dMgr->DrawRect(SaveButtonBaseX, SaveButtonBaseY, SaveButtonWidth, SaveButtonHeight, MENUGET_COLOR("Colors", "colored_normal"), true);
    dMgr->DrawShadowString(1, SaveButtonBaseX + 15, SaveButtonBaseY + 7, MENUGET_COLOR("Colors", "label_text"), false, "Save Settings");

    if (IsMouseOverThis(SaveButtonBaseX, SaveButtonBaseY, SaveButtonWidth, SaveButtonHeight)) {
        if (mouseReleased) {
            CSettingsManager::Instance("settings.ini")->SaveSettings(false, true);
            mouseReleased = false;
            CloseMenu();
        }
    }
}

void CHackMenu::DrawCheckbox(int col, int row, bool value) {
    int ButtonBaseX = baseX + MENUGET_INT("Sizes", "tabs_w") + (MENUGET_INT("Sizes", "col_w") * (col - 1)) + 5;
    int ButtonBaseY = baseY + (MENUGET_INT("Sizes", "col_h") * row) + 3;
    int CheckboxBaseX = ButtonBaseX + 3 + MENUGET_INT("Sizes", "button_w") - 21;
    int CheckboxBaseY = ButtonBaseY + 3 + 5;
    dMgr->DrawOutlineRect(CheckboxBaseX, CheckboxBaseY, 16, 16, MENUGET_COLOR("Colors", "label_text"));
    dMgr->DrawOutlineRect(CheckboxBaseX + 1, CheckboxBaseY + 1, 14, 14, MENUGET_COLOR("Colors", "label_text"));

    if (value) {
        dMgr->DrawRect(CheckboxBaseX + 3, CheckboxBaseY + 3, 12, 12, MENUGET_COLOR("Colors", "colored_normal"), true);
    }
}

void CHackMenu::DrawColorSet(const char *section, const char *key, int x, int y, Color value) {
    int SliderWidth = MENUGET_INT("Sizes", "slider_w");

    int SliderBaseX = (x + MENUGET_INT("Sizes", "button_w")) - (SliderWidth + 5);
    int SliderBaseY = y + 8;

    DrawColorSlider(section, key, SliderBaseX, SliderBaseY, value.r(), Color(255, 0, 0, 150));

    SliderBaseY += 20;
    DrawColorSlider(section, key, SliderBaseX, SliderBaseY, value.g(), Color(0, 255, 0, 150));

    SliderBaseY += 20;
    DrawColorSlider(section, key, SliderBaseX, SliderBaseY, value.b(), Color(0, 0, 255, 150));

    if (value.a() < 255) {
        value.SetA(255);
    }

    dMgr->DrawRect(x + 20, y + MENUGET_INT("Sizes", "button_h") + 5, 120, 15, value, true);
}

void CHackMenu::DrawSlider(const char *section, const char* key, int x, int y, float value, float minValue, float maxValue) {
    int SliderWidth = MENUGET_INT("Sizes", "slider_w");

    int SliderBaseX = (x + MENUGET_INT("Sizes", "button_w")) - (SliderWidth + 5);
    int SliderBaseY = y + 19;

    float rangeValue = (0 - minValue) + maxValue;
    int sliderPercent = (int)(value / rangeValue * 100);

    dMgr->DrawRect(SliderBaseX, SliderBaseY, SliderWidth, 4, MENUGET_COLOR("Colors", "slider_background"), true);
    dMgr->DrawRect(SliderBaseX, SliderBaseY, sliderPercent, 4, MENUGET_COLOR("Colors", "colored_normal"), true);

    int PointerBaseX = (SliderBaseX - 6) + sliderPercent;
    int PointerBaseY = SliderBaseY - 4;

    if (IsMouseOverThis(PointerBaseX, PointerBaseY, 12, 16)) {
        if (Interfaces::InputSystem()->IsButtonDown(MOUSE_LEFT)) {
            int mouseX, mouseY;
            Interfaces::Surface()->SurfaceGetCursorPos(mouseX, mouseY);
            if (mouseX > SliderBaseX + SliderWidth) {
                mouseX = SliderBaseX + SliderWidth;
            } else if (mouseX < SliderBaseX) {
                mouseX = SliderBaseX;
            }
            if (mouseX != lastMousePosX) {
                int PixelFromBaseX = mouseX - SliderBaseX;
                value = (float)(rangeValue / 100 * PixelFromBaseX);
                sliderPercent = (int)(value / rangeValue * 100);
                PointerBaseX = (SliderBaseX - 6) + sliderPercent;
                CSettingsManager::Instance("settings.ini")->SetDoubleValue(section, key, (double)value);
            }
        }
    }

    dMgr->DrawRect(PointerBaseX + 1, PointerBaseY + 1, 12, 16, Color(0, 0, 0));
    dMgr->DrawRect(PointerBaseX, PointerBaseY, 12, 16, MENUGET_COLOR("Colors", "colored_highlight"), true);

    if (!sliderFont) {
        sliderFont = dMgr->GetFont(MENUGET_STRING("Main", "menu_font"), 12, FontFlags::FONTFLAG_ANTIALIAS);
    }

    float outputvalue;
    if (minValue < 0.f) {
        outputvalue = value - maxValue;
    } else {
        outputvalue = value;
    }

    char stringValue[128];
    sprintf(stringValue, "%f", outputvalue);

    dMgr->DrawShadowString(sliderFont, SliderBaseX + 15, SliderBaseY - 13, MENUGET_COLOR("Colors", "label_text"), false, stringValue);
}

void CHackMenu::DrawPercentSlider(const char *section, const char* key, int x, int y, int value, int minValue, int maxValue) {
    int SliderWidth = MENUGET_INT("Sizes", "slider_w");

    int SliderBaseX = (x + MENUGET_INT("Sizes", "button_w")) - (SliderWidth + 5);
    int SliderBaseY = y + 19;

    int sliderPercent = value;

    dMgr->DrawRect(SliderBaseX, SliderBaseY, SliderWidth, 4, MENUGET_COLOR("Colors", "slider_background"), true);
    dMgr->DrawRect(SliderBaseX, SliderBaseY, sliderPercent, 4, MENUGET_COLOR("Colors", "colored_normal"), true);

    int PointerBaseX = (SliderBaseX - 6) + sliderPercent;
    int PointerBaseY = SliderBaseY - 4;

    if (IsMouseOverThis(PointerBaseX, PointerBaseY, 12, 16)) {
        if (Interfaces::InputSystem()->IsButtonDown(MOUSE_LEFT)) {
            int PixelFromBaseX = 0;
            int mouseX, mouseY;
            Interfaces::Surface()->SurfaceGetCursorPos(mouseX, mouseY);
            if (mouseX > SliderBaseX + SliderWidth) {
                mouseX = SliderBaseX + SliderWidth;
            } else if (mouseX < SliderBaseX) {
                mouseX = SliderBaseX;
            }
            if (mouseX != lastMousePosX) {
                PixelFromBaseX = mouseX - SliderBaseX;
                value = PixelFromBaseX;
                sliderPercent = value;
                PointerBaseX = (SliderBaseX - 6) + sliderPercent;
                CSettingsManager::Instance("settings.ini")->SetIntValue(section, key, (int)value);
            }
        }
    }

    dMgr->DrawRect(PointerBaseX + 1, PointerBaseY + 1, 12, 16, Color(0, 0, 0));
    dMgr->DrawRect(PointerBaseX, PointerBaseY, 12, 16, MENUGET_COLOR("Colors", "colored_highlight"), true);

    if (!sliderFont) {
        sliderFont = dMgr->GetFont(MENUGET_STRING("Main", "menu_font"), 12, FontFlags::FONTFLAG_ANTIALIAS);
    }

    char stringValue[128];
    sprintf(stringValue, "%i %%", value);

    dMgr->DrawShadowString(sliderFont, SliderBaseX + 15, SliderBaseY - 13, MENUGET_COLOR("Colors", "label_text"), false, stringValue);
}

void CHackMenu::DrawColorSlider(const char *section, const char* key, int x, int y, int value, Color colorValue) {
    int SliderWidth = MENUGET_INT("Sizes", "slider_w");

    int SliderBaseX = x;
    int SliderBaseY = y;
    bool isRed      = colorValue.r() == 255 && colorValue.g() == 0 && colorValue.b() == 0;
    bool isGreen    = colorValue.r() == 0 && colorValue.g() == 255 && colorValue.b() == 0;
    bool isBlue     = colorValue.r() == 0 && colorValue.g() == 0 && colorValue.b() == 255;

    double calcValue = static_cast<double>(value);
    double sliderPercent = calcValue / 255 * 100;

    dMgr->DrawRect(SliderBaseX, SliderBaseY, SliderWidth, 4, MENUGET_COLOR("Colors", "slider_background"), true);
    dMgr->DrawRect(SliderBaseX, SliderBaseY, (int)floor(sliderPercent + 0.5), 4, colorValue, true);

    int PointerBaseX = (SliderBaseX - 6) + (int)floor(sliderPercent + 0.5);
    int PointerBaseY = SliderBaseY - 4;

    if (IsMouseOverThis(PointerBaseX, PointerBaseY, 12, 16)) {
        if (Interfaces::InputSystem()->IsButtonDown(MOUSE_LEFT)) {
            int PixelFromBaseX = 0;
            int mouseX, mouseY;
            Interfaces::Surface()->SurfaceGetCursorPos(mouseX, mouseY);
            if (mouseX > SliderBaseX + SliderWidth) {
                mouseX = SliderBaseX + SliderWidth;
            } else if (mouseX < SliderBaseX) {
                mouseX = SliderBaseX;
            }
            if (mouseX != lastMousePosX) {
                PixelFromBaseX = mouseX - SliderBaseX;
                calcValue = static_cast<double>(255) / 100 * PixelFromBaseX;
                sliderPercent = calcValue / 255 * 100;
                PointerBaseX = (SliderBaseX - 6) + (int)floor(sliderPercent + 0.5);
                Color currentColor = INIGET_COLOR(section, key);
                if (isRed) {
                    currentColor.SetR((int)floor(calcValue + 0.5));
                } else if (isGreen) {
                    currentColor.SetG((int)floor(calcValue + 0.5));
                } else if (isBlue) {
                    currentColor.SetB((int)floor(calcValue + 0.5));
                }

                CSettingsManager::Instance("settings.ini")->SetColorValue(section, key, currentColor);
            }
        }
    }

    dMgr->DrawRect(PointerBaseX + 1, PointerBaseY + 1, 12, 16, Color(0, 0, 0), true);
    dMgr->DrawRect(PointerBaseX, PointerBaseY, 12, 16, MENUGET_COLOR("Colors", "colored_highlight"), true);

    if (!colorSliderFont) {
        colorSliderFont = dMgr->GetFont(MENUGET_STRING("Main", "menu_font"), 12, FontFlags::FONTFLAG_ANTIALIAS);
    }

    char stringValue[128];
    sprintf(stringValue, "%i", (int)floor(calcValue + 0.5));
    dMgr->DrawShadowString(colorSliderFont, SliderBaseX - 30, SliderBaseY, MENUGET_COLOR("Colors", "label_text"), false, stringValue);
}

void CHackMenu::DrawInputField(const char *section, const char *key, int x, int y, std::string value) {
    int InputWidth = MENUGET_INT("Sizes", "input_w");
    int InputHeight = MENUGET_INT("Sizes", "input_h");

    int InputBaseX = (x + MENUGET_INT("Sizes", "button_w")) - (InputWidth + 5);
    int InputBaseY = y + 5;

    if (m_szCurrentString[InputIndex].empty() && value.length() > 1) {
        const char* current = value.c_str();
        for (size_t i = 0; i < value.length(); i++) {
            m_szCurrentString[InputIndex].push_back(current[i]);
            m_nCurrentPos[InputIndex]++;
        }
    }

    dMgr->DrawRect(InputBaseX, InputBaseY, InputWidth, InputHeight, MENUGET_COLOR("Colors", "input_background"));

    if (IsMouseOverThis(InputBaseX, InputBaseY, InputWidth, InputHeight)) {
        if (mouseReleased && activeInput != InputIndex) {
            activeInput = InputIndex;
            mouseReleased = false;
        }
    }

    if (IsMouseOverThis(baseX, baseY, baseWidth, baseHeight) && !IsMouseOverThis(InputBaseX, InputBaseY, InputWidth, InputHeight)) {
        if (mouseReleased && activeInput == InputIndex) {
            activeInput = 0;
            mouseReleased = false;
        }
    }

    if (activeInput == InputIndex) {
        dMgr->DrawOutlineRect(InputBaseX, InputBaseY, InputWidth, InputHeight, MENUGET_COLOR("Colors", "colored_highlight"));
    } else {
        dMgr->DrawOutlineRect(InputBaseX, InputBaseY, InputWidth, InputHeight, MENUGET_COLOR("Colors", "colored_normal"));
    }

    int TextBaseX = InputBaseX + 5;
    int TextBaseY = InputBaseY + 3;

    dMgr->DrawString(1, TextBaseX, TextBaseY, MENUGET_COLOR("Colors", "input_text"), false, m_szCurrentString[InputIndex].c_str());

    if (value != m_szCurrentString[InputIndex]) {
        CSettingsManager::Instance("settings.ini")->SetValue(section, key, m_szCurrentString[InputIndex].c_str());
    }

    InputIndex++;
}

void CHackMenu::DrawSelectField(const char *section, const char *key, int x, int y, int value, std::map<int, const char*> list, std::map<int, const char*> values) {
    int SelectFieldWidth = MENUGET_INT("Sizes", "select_field_w");
    int SelectFieldHeight = MENUGET_INT("Sizes", "select_field_h");

    int SelectBaseX = x + MENUGET_INT("Sizes", "button_w") - (SelectFieldWidth + 5 + SelectFieldHeight);
    int SelectBaseY = y + 3;

    Color borderColor = MENUGET_COLOR("Colors", "colored_normal");
    if (IsMouseOverThis(SelectBaseX, SelectBaseY, SelectFieldWidth + SelectFieldHeight, SelectFieldHeight)) {
        if (mouseReleased && activeSelect != selectIndex) {
            borderColor = MENUGET_COLOR("Colors", "colored_highlight");
            activeSelect = selectIndex;
            activeSelectList = list;
            activeSelectValues = values;
            activeSelectBaseX = SelectBaseX;
            activeSelectBaseY = SelectBaseY;
            activeSelectSection = section;
            activeSelectKey = key;
            mouseReleased = false;
        }
    }

    dMgr->DrawRect(SelectBaseX, SelectBaseY, SelectFieldWidth + SelectFieldHeight, SelectFieldHeight, MENUGET_COLOR("Colors", "input_background"));
    dMgr->DrawOutlineRect(SelectBaseX, SelectBaseY, SelectFieldWidth + SelectFieldHeight, SelectFieldHeight, borderColor);

    bool directionDown = true;
    if (activeSelect == selectIndex) {
        directionDown = false;
    }
    dMgr->DrawTriangle(14, 7, Color(0, 0, 0), SelectBaseX + SelectFieldWidth + 5, SelectBaseY + 9, directionDown);
    dMgr->DrawTriangle(14, 7, borderColor, SelectBaseX + SelectFieldWidth + 5, SelectBaseY + 9, directionDown);

    const char* selectedValue = values[value];

    dMgr->DrawString(1, SelectBaseX + 5, SelectBaseY + 6, MENUGET_COLOR("Colors", "input_text"), false, selectedValue);

    selectIndex++;
}

void CHackMenu::DrawSelectField(const char *section, const char *key, int x, int y, std::map<int, const char*> list, std::map<int, const char*> values, CSettingsManager* config) {
    int SelectFieldWidth = MENUGET_INT("Sizes", "select_field_w");
    int SelectFieldHeight = MENUGET_INT("Sizes", "select_field_h");

    int SelectBaseX = x + MENUGET_INT("Sizes", "button_w") - (SelectFieldWidth + 5 + SelectFieldHeight);
    int SelectBaseY = y + 3;

    Color borderColor = MENUGET_COLOR("Colors", "colored_normal");
    if (IsMouseOverThis(SelectBaseX, SelectBaseY, SelectFieldWidth + SelectFieldHeight, SelectFieldHeight)) {
        if (mouseReleased && activeSelect != selectIndex) {
            borderColor = MENUGET_COLOR("Colors", "colored_highlight");
            activeSelect = selectIndex;
            activeSelectList = list;
            activeSelectValues = values;
            activeSelectBaseX = SelectBaseX;
            activeSelectBaseY = SelectBaseY;
            activeSelectSection = section;
            activeSelectKey = key;
            mouseReleased = false;
        }
    }

    dMgr->DrawRect(SelectBaseX, SelectBaseY, SelectFieldWidth + SelectFieldHeight, SelectFieldHeight, MENUGET_COLOR("Colors", "input_background"));
    dMgr->DrawOutlineRect(SelectBaseX, SelectBaseY, SelectFieldWidth + SelectFieldHeight, SelectFieldHeight, borderColor);

    bool directionDown = true;
    if (activeSelect == selectIndex) {
        directionDown = false;
    }
    dMgr->DrawTriangle(14, 7, Color(0, 0, 0), SelectBaseX + SelectFieldWidth + 5, SelectBaseY + 9, directionDown);
    dMgr->DrawTriangle(14, 7, borderColor, SelectBaseX + SelectFieldWidth + 5, SelectBaseY + 9, directionDown);
    int value = config->GetSetting<int>(section, key);
    const char* selectedValue = values[value];

    dMgr->DrawString(1, SelectBaseX + 5, SelectBaseY + 6, MENUGET_COLOR("Colors", "input_text"), false, selectedValue);

    selectIndex++;
}

void CHackMenu::SaveMousePosition() {
    int mox, moy;
    Interfaces::Surface()->SurfaceGetCursorPos(mox, moy);
    if (!Interfaces::InputSystem()->IsButtonDown(MOUSE_LEFT)) {
        lastMousePosX = mox;
        lastMousePosY = moy;
    }
}

void CHackMenu::DrawActiveSelect() {
    if (activeSelect == 0) {
        return;
    }
    IsSelectOpen = true;

    int SelectFieldWidth = MENUGET_INT("Sizes", "select_field_w");

    int OptionHeight = MENUGET_INT("Sizes", "option_h");
    int marginTop = activeSelectBaseY + 1;
    int marginRight = activeSelectBaseX + 1;
    for (int i = 0; i < (int)activeSelectValues.size(); i++) {
        dMgr->DrawRect(marginRight, marginTop, SelectFieldWidth - 1, OptionHeight, MENUGET_COLOR("Colors", "input_background"));
        dMgr->DrawString(1, marginRight + 4, marginTop + 5, MENUGET_COLOR("Colors", "input_text"), false, activeSelectValues[i]);
        
        if (IsMouseOverThis(marginRight, marginTop, SelectFieldWidth, OptionHeight, true)) {
            if (mouseReleased) {
                CSettingsManager::Instance("settings.ini")->SetIntValue(activeSelectSection, activeSelectKey, i);
                activeSelect = 0;
                IsSelectOpen = false;
                mouseReleased = true;
            }
        }
        
        marginTop += OptionHeight;
    }
}

void CHackMenu::DrawKeyField(const char *section, const char *key, int x, int y, int keyValue) {
    int InputWidth = MENUGET_INT("Sizes", "input_w");
    int InputHeight = MENUGET_INT("Sizes", "input_h");

    int InputBaseX = (x + MENUGET_INT("Sizes", "button_w")) - (InputWidth + 5);
    int InputBaseY = y + 5;

    dMgr->DrawRect(InputBaseX, InputBaseY, InputWidth, InputHeight, MENUGET_COLOR("Colors", "input_background"));

    if (
        activeKeyInput != -1 &&
        activeKeyInput == keyInputIndex &&
        lastKey != -5
    ) {
        if ((ButtonCode_t)lastKey == ButtonCode_t::KEY_ESCAPE) {
            keyValue = 0;
        } else {
            keyValue = lastKey;
        }
        CSettingsManager::Instance("settings.ini")->SetIntValue(section, key, keyValue);
        activeKeyInput = -1;
        lastKey = -5;
    }

    if (IsMouseOverThis(InputBaseX, InputBaseY, InputWidth, InputHeight)) {
        if (mouseReleased && activeKeyInput != keyInputIndex) {
            activeKeyInput = keyInputIndex;
            keyInputWaiting = true;
            mouseReleased = false;
        }
    }

    if (activeKeyInput == keyInputIndex) {
        dMgr->DrawOutlineRect(InputBaseX, InputBaseY, InputWidth, InputHeight, MENUGET_COLOR("Colors", "colored_highlight"));
    } else {
        dMgr->DrawOutlineRect(InputBaseX, InputBaseY, InputWidth, InputHeight, MENUGET_COLOR("Colors", "colored_normal"));
    }

    int TextBaseX = InputBaseX + 5;
    int TextBaseY = InputBaseY + 3;

    const char* inputValue;
    if (keyInputWaiting) {
        inputValue = "Press any key ...";
    } else {
        inputValue = KeyNameForValue.at(keyValue);
    }
    dMgr->DrawString(1, TextBaseX, TextBaseY, MENUGET_COLOR("Colors", "input_text"), false, inputValue);

    keyInputIndex++;
}
