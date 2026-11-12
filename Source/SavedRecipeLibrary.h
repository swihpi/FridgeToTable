#pragma once
#include "StudioAssets.h"

class SavedRecipeLibrary final : public juce::Component {
public:
    using Action=std::function<bool(const Recipe&)>;
    SavedRecipeLibrary(const std::vector<Recipe>& recipes,Action onOpen,Action onText,Action onPicture,Action onExport){
        setLookAndFeel(&theme);setSize(840,640);addAndMakeVisible(title);title.setText("Your saved recipes",juce::dontSendNotification);title.setFont(juce::Font(juce::FontOptions("Georgia",28.0f,juce::Font::bold)));title.setColour(juce::Label::textColourId,juce::Colour(0xfff6f1de));
        addAndMakeVisible(subtitle);subtitle.setText("Open a recipe, send its instructions, or share its dish picture separately.",juce::dontSendNotification);subtitle.setColour(juce::Label::textColourId,juce::Colour(0xffa5a797));
        addAndMakeVisible(viewport);viewport.setViewedComponent(&content,false);viewport.setScrollBarsShown(true,false);viewport.getVerticalScrollBar().setColour(juce::ScrollBar::thumbColourId,juce::Colour(0xff697449));
        for(const auto& recipe:recipes){auto row=std::make_unique<Row>(recipe,onOpen,onText,onPicture,onExport);content.addAndMakeVisible(*row);rows.push_back(std::move(row));}
    }
    ~SavedRecipeLibrary() override {setLookAndFeel(nullptr);}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff141511));}
    void resized() override {
        title.setBounds(26,20,780,40);subtitle.setBounds(26,65,780,26);viewport.setBounds(16,110,getWidth()-32,getHeight()-126);
        content.setSize(viewport.getWidth()-18,juce::jmax(viewport.getHeight(),(int)rows.size()*164));
        for(size_t i=0;i<rows.size();++i)rows[i]->setBounds(4,(int)i*164,content.getWidth()-8,154);
    }
private:
    class Row final : public juce::Component {
    public:
        Row(Recipe r,Action onOpen,Action onText,Action onPicture,Action onExport):recipe(std::move(r)),photo(juce::ImageFileFormat::loadFrom(juce::File(recipe.imagePath))){
            addAndMakeVisible(title);title.setText(recipe.title,juce::dontSendNotification);title.setFont(juce::Font(juce::FontOptions(17.0f,juce::Font::bold)));title.setColour(juce::Label::textColourId,juce::Colour(0xfff6f1de));
            addAndMakeVisible(meta);meta.setText(juce::String(recipe.servings)+" people  /  "+juce::String(recipe.minutes)+" min  /  ~"+juce::String(recipe.caloriesPerServing)+" kcal per person",juce::dontSendNotification);meta.setColour(juce::Label::textColourId,juce::Colour(0xffa5a797));
            for(auto* b:{&open,&send,&picture,&exportText})addAndMakeVisible(*b);
            open.setButtonText("Open recipe");send.setButtonText("Send to...");picture.setButtonText("Send Picture to...");exportText.setButtonText("Export TXT");picture.setEnabled(photo.isValid());
            addAndMakeVisible(notice);notice.setFont(juce::Font(juce::FontOptions(11.0f)));notice.setColour(juce::Label::textColourId,juce::Colour(0xffa5a797));
            open.onClick=[this,onOpen]{onOpen(recipe);};
            send.onClick=[this,onText]{notice.setText(onText(recipe)?"Recipe and TXT saved. Choose a destination in the share sheet.":"Could not save or share. Check the main window for details.",juce::dontSendNotification);};
            picture.onClick=[this,onPicture]{notice.setText(onPicture(recipe)?"Sharing the dish picture only.":"Could not share the picture. Check the main window for details.",juce::dontSendNotification);};
            exportText.onClick=[this,onExport]{notice.setText(onExport(recipe)?"Recipe and TXT saved. Exporting a copy.":"Could not export. Check the main window for details.",juce::dontSendNotification);};
        }
        void paint(juce::Graphics& g) override {
            auto r=getLocalBounds().toFloat().reduced(1);g.setColour(juce::Colour(0xff22241f));g.fillRoundedRectangle(r,15);g.setColour(juce::Colour(0xff444735));g.drawRoundedRectangle(r,15,1);
            if(photo.isValid()){juce::Graphics::ScopedSaveState state(g);juce::Path clip;clip.addRoundedRectangle(12,12,126,128,10);g.reduceClipRegion(clip);g.drawImage(photo,juce::Rectangle<float>(12,12,126,128),juce::RectanglePlacement::fillDestination);}
            else {g.setColour(juce::Colour(0xff303526));g.fillRoundedRectangle(12,12,126,128,10);g.setColour(juce::Colour(0xffdbf65b));g.setFont(juce::Font(juce::FontOptions("Georgia",38.0f,juce::Font::plain)));g.drawText(juce::String(recipe.title).substring(0,1),12,12,126,128,juce::Justification::centred);}
        }
        void resized() override {
            title.setBounds(154,13,getWidth()-172,30);meta.setBounds(154,45,getWidth()-172,24);
            open.setBounds(154,80,112,32);send.setBounds(274,80,108,32);picture.setBounds(390,80,156,32);exportText.setBounds(554,80,110,32);notice.setBounds(154,119,getWidth()-172,23);
        }
    private:
        Recipe recipe;juce::Image photo;juce::Label title,meta,notice;juce::TextButton open,send,picture,exportText;
    };
    StudioAssets::BrowserTheme theme;juce::Label title,subtitle;juce::Viewport viewport;juce::Component content;std::vector<std::unique_ptr<Row>> rows;
};
