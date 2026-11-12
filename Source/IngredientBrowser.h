#pragma once
#include "IngredientCatalog.h"
#include "StudioAssets.h"
#include <juce_gui_extra/juce_gui_extra.h>

class IngredientBrowser final : public juce::Component {
public:
    explicit IngredientBrowser(std::function<void(Food,double)> choose) : onChoose(std::move(choose)) {
        setLookAndFeel(&theme);setSize(940,650);
        addAndMakeVisible(title); title.setText("Your kitchen, ingredient by ingredient",juce::dontSendNotification); title.setFont(juce::Font(juce::FontOptions(25.0f,juce::Font::bold))); title.setColour(juce::Label::textColourId,juce::Colours::white);
        addAndMakeVisible(categories); categories.addItem("All foods",1); juce::StringArray names; for(auto& f:IngredientCatalog::all()) if(!names.contains(f.category)) names.add(f.category); names.sort(true); int id=2; for(auto& n:names) categories.addItem(n,id++); categories.setSelectedId(1); categories.onChange=[this]{ rebuild(); };
        addAndMakeVisible(search); search.setTextToShowWhenEmpty("Optional search - or browse the food icons below",juce::Colours::grey); search.onTextChange=[this]{rebuild();};
        addAndMakeVisible(weightLabel);weightLabel.setText("WEIGHT (g)",juce::dontSendNotification);weightLabel.setColour(juce::Label::textColourId,juce::Colours::white);addAndMakeVisible(weight);weight.setInputRestrictions(7,"0123456789.");weight.setText("200");
        addAndMakeVisible(viewport); viewport.setViewedComponent(&grid,false); viewport.setScrollBarsShown(true,false);
        addAndMakeVisible(custom); custom.setButtonText("Add this as a custom ingredient"); custom.onClick=[this]{ if(search.getText().trim().isNotEmpty()) onChoose(IngredientCatalog::custom(search.getText().trim()),juce::jmax(1.0,weight.getText().getDoubleValue())); };
        addAndMakeVisible(note); note.setText("Tap food to add it. Adjust grams in your kitchen. Custom ingredients are always supported.",juce::dontSendNotification); note.setColour(juce::Label::textColourId,juce::Colour(0xffa9b2a8));
        rebuild();
    }
    ~IngredientBrowser() override {setLookAndFeel(nullptr);}
    void paint(juce::Graphics& g) override { g.fillAll(juce::Colour(0xff151a16)); }
    void resized() override { title.setBounds(24,18,880,40); categories.setBounds(24,74,245,38); search.setBounds(282,74,420,38);weightLabel.setBounds(718,76,100,34);weight.setBounds(820,74,92,38); viewport.setBounds(20,128,900,443); custom.setBounds(24,583,350,36); note.setBounds(385,580,530,45); }
private:
    void rebuild() {
        buttons.clear(); grid.removeAllChildren(); auto foods=IngredientCatalog::matching(search.getText(),categories.getText());
        for(size_t i=0;i<foods.size();++i) { const auto food=foods[i]; auto b=std::make_unique<StudioAssets::FoodTile>(food); b->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff29322a)); b->setColour(juce::TextButton::textColourOffId,juce::Colours::white); b->onClick=[this,food]{ onChoose(food,juce::jmax(1.0,weight.getText().getDoubleValue())); }; b->setBounds(4+(int)(i%4)*219,4+(int)(i/4)*88,207,76); grid.addAndMakeVisible(*b); buttons.push_back(std::move(b)); }
        grid.setSize(880,juce::jmax(440,(int)((foods.size()+3)/4)*88));
    }
    StudioAssets::BrowserTheme theme;std::function<void(Food,double)> onChoose; juce::Label title,note,weightLabel; juce::TextEditor weight; juce::ComboBox categories; juce::TextEditor search; juce::TextButton custom; juce::Viewport viewport; juce::Component grid; std::vector<std::unique_ptr<juce::TextButton>> buttons;
};
