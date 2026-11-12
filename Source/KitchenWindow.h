#pragma once
#include "CodexEngine.h"
#include "IngredientCatalog.h"
#include <juce_gui_extra/juce_gui_extra.h>

class KitchenWindow final : public juce::DocumentWindow {
public:
    KitchenWindow();
    ~KitchenWindow() override;
    void closeButtonPressed() override;
private:
    class MainView;
    std::unique_ptr<MainView> view;
};

extern "C" void ShareRecipeText(const char* text,void* nativeView);
extern "C" void ActivateFridgeToTable();


extern "C" void ShareRecipeWithImage(const char* text,const char* imagePath);
extern "C" void ShareRecipeImage(const char* imagePath,void* nativeView);
