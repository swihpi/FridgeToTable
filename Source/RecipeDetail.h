#pragma once
#include "Domain.h"
#include "StudioAssets.h"
#include <juce_gui_extra/juce_gui_extra.h>
class RecipeDetail final : public juce::Component {
    class Page final : public juce::Component {
    public:
        Recipe recipe; juce::Image dish;
        explicit Page(Recipe r):recipe(std::move(r)){dish=juce::ImageFileFormat::loadFrom(juce::File(recipe.imagePath));}
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colour(0xff141511));
            auto text=[&](const juce::String& s,int x,int y,int w,int h,float size,juce::Colour colour,bool bold=false){g.setColour(colour);g.setFont(juce::Font(juce::FontOptions(size,bold?juce::Font::bold:juce::Font::plain)));g.drawFittedText(s,x,y,w,h,juce::Justification::topLeft,20);};
            const juce::Colour white(0xfff6f1de), muted(0xffa5a797), lime(0xffdbf65b);
            if(dish.isValid()){g.drawImage(dish,juce::Rectangle<float>(32,20,750,250),juce::RectanglePlacement::fillDestination);g.addTransform(juce::AffineTransform::translation(0,280));}
            text("YOUR RECIPE",32,25,700,20,12,lime,true);
            text(recipe.title,32,55,750,84,32,white,true);
            text(juce::String(recipe.minutes)+" MIN   /   "+juce::String(recipe.servings)+" PEOPLE   /   ~"+juce::String(recipe.caloriesPerServing)+" KCAL PER PERSON",32,145,750,28,14,lime,true);
            text(recipe.description,32,187,750,90,16,muted);
            text("INGREDIENTS  /  TOTAL RECIPE WEIGHTS",32,290,750,24,13,lime,true);
            int y=328;
            for(const auto& i:recipe.ingredients){g.setColour(juce::Colour(0xff25281f));g.fillRoundedRectangle(32,(float)y,750,38,8);text(i.name,45,y+9,550,24,16,white);text(juce::String(i.grams,0)+" g",645,y+9,110,24,16,lime,true);y+=46;}
            y+=24;text("LET'S COOK",32,y,750,24,13,lime,true);y+=42;
            for(size_t n=0;n<recipe.steps.size();++n){text(juce::String((int)n+1),32,y,45,42,24,lime,true);auto layout=stepLayout(recipe.steps[n]);int height=juce::jmax(62,(int)std::ceil(layout.getHeight())+16);layout.draw(g,{90.0f,(float)y,680.0f,(float)height});y+=height+24;}
            text("Calories are estimates. Dish photography is AI generated.",32,y+12,750,50,13,muted);
        }
        static juce::TextLayout stepLayout(const std::string& s){
            juce::AttributedString content;content.setLineSpacing(4.0f);
            auto body=juce::String::fromUTF8(s.c_str());int headingEnd=body.indexOfChar('\n');
            const juce::Colour white(0xfff6f1de);
            if(headingEnd>0){content.append(body.substring(0,headingEnd)+"\n",juce::Font(juce::FontOptions(18.0f,juce::Font::bold)),white);body=body.substring(headingEnd+1);}
            content.append(body,juce::Font(juce::FontOptions(17.0f)),white);
            juce::TextLayout layout;layout.createLayout(content,680.0f);return layout;
        }
        static int stepHeight(const std::string& s){return juce::jmax(62,(int)std::ceil(stepLayout(s).getHeight())+16);}
        int pageHeight() const {int h=(dish.isValid()?720:440)+(int)recipe.ingredients.size()*46;for(auto& s:recipe.steps)h+=stepHeight(s)+24;return h;}
    };
public:
    using Action=std::function<bool(const Recipe&)>;
    RecipeDetail(const Recipe& r,bool saved,Action onSave,Action onText,Action onPicture,Action onExport):page(r){
        setLookAndFeel(&theme);setSize(840,780);
        addAndMakeVisible(viewport);viewport.setViewedComponent(&page,false);
        for(auto* b:{&save,&send,&picture,&exportText}){
            addAndMakeVisible(*b);b->setColour(juce::TextButton::buttonColourId,juce::Colour(0xff303526));b->setColour(juce::TextButton::textColourOffId,juce::Colour(0xffdbf65b));
        }
        save.setButtonText(saved?"Saved":"Save Recipe");send.setButtonText("Send to...");picture.setButtonText("Send Picture to...");exportText.setButtonText("Export TXT");
        picture.setEnabled(juce::File(r.imagePath).existsAsFile());
        addAndMakeVisible(notice);notice.setFont(juce::Font(juce::FontOptions(12.0f)));notice.setColour(juce::Label::textColourId,juce::Colour(0xffa5a797));
        notice.setText("Send the recipe as plain text, or share its picture separately.",juce::dontSendNotification);
        save.onClick=[this,onSave]{if(onSave(page.recipe)){save.setButtonText("Saved");notice.setText("Saved to your recipes.",juce::dontSendNotification);}else showFailure();};
        send.onClick=[this,onText]{if(onText(page.recipe)){save.setButtonText("Saved");notice.setText("Recipe and TXT saved in the app. Choose a destination in the share sheet.",juce::dontSendNotification);}else showFailure();};
        picture.onClick=[this,onPicture]{if(onPicture(page.recipe)){save.setButtonText("Saved");notice.setText("Sharing the dish picture only. Choose a destination in the share sheet.",juce::dontSendNotification);}else showFailure();};
        exportText.onClick=[this,onExport]{if(onExport(page.recipe)){save.setButtonText("Saved");notice.setText("Recipe and TXT saved in the app. Choose where to export a copy.",juce::dontSendNotification);}else showFailure();};
    }
    ~RecipeDetail() override {setLookAndFeel(nullptr);}
    void paint(juce::Graphics& g) override {g.fillAll(juce::Colour(0xff1d1f18));g.setColour(juce::Colour(0xff444735));g.drawHorizontalLine(91,0,(float)getWidth());}
    void resized() override {
        save.setBounds(26,14,142,36);send.setBounds(180,14,142,36);picture.setBounds(334,14,190,36);exportText.setBounds(getWidth()-178,14,146,36);notice.setBounds(26,57,getWidth()-52,24);
        viewport.setBounds(0,92,getWidth(),getHeight()-92);page.setSize(820,page.pageHeight());
    }
private:
    void showFailure(){notice.setText("Could not save or share this recipe. Check the main window for details.",juce::dontSendNotification);}
    StudioAssets::BrowserTheme theme;
    Page page; juce::Viewport viewport;
    juce::TextButton save,send,picture,exportText;juce::Label notice;
};
