#include "KitchenWindow.h"
#include "IngredientBrowser.h"
#include "RecipeDetail.h"
#include "SavedRecipeLibrary.h"
#include "StudioAssets.h"
#include <cmath>

namespace {
const juce::Colour bg {0xff141511}, panel {0xff1d1f18}, panel2 {0xff25281f}, line {0xff444735}, white {0xfff6f1de}, muted {0xffa5a797}, lime {0xffdbf65b}, limeDark {0xffa2c91d};
juce::Font font(float size, bool bold=false) { return juce::Font(juce::FontOptions(size, bold ? juce::Font::bold : juce::Font::plain)); }

class StudioLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    StudioLookAndFeel(){setColour(juce::PopupMenu::backgroundColourId,panel);setColour(juce::PopupMenu::textColourId,white);setColour(juce::PopupMenu::highlightedBackgroundColourId,line);setColour(juce::PopupMenu::highlightedTextColourId,lime);}
    void drawProgressBar(juce::Graphics& g,juce::ProgressBar&,int w,int h,double value,const juce::String& text) override {
        g.setColour(panel2);g.fillRoundedRectangle(0,0,(float)w,(float)h,7);g.setColour(lime.withAlpha(.7f));g.fillRoundedRectangle(0,0,(float)w*(float)juce::jlimit(0.0,1.0,value),(float)h,7);g.setColour(white);g.setFont(font(10,true));g.drawText(text,0,0,w,h,juce::Justification::centred);
    }
    void drawButtonBackground(juce::Graphics& g, juce::Button& button, const juce::Colour& background,
                              bool hovered, bool down) override {
        auto fill = button.getToggleState() ? button.findColour(juce::TextButton::buttonOnColourId) : background;
        if (down) fill = fill.brighter(0.08f); else if (hovered) fill = fill.brighter(0.04f);
        auto rect=button.getLocalBounds().toFloat().reduced(.7f);
        if(background==lime) {g.setGradientFill(juce::ColourGradient(juce::Colour(0xffedff82),0,0,lime,0,(float)button.getHeight(),false));g.fillRoundedRectangle(rect,rect.getHeight()*.48f);}
        else {g.setColour(fill);g.fillRoundedRectangle(rect,button.getClickingTogglesState()?12.0f:rect.getHeight()*.48f);}
        if(button.getClickingTogglesState()){g.setColour(button.getToggleState()?juce::Colour(0xff8e9c3c):line);g.drawRoundedRectangle(rect,12,0.8f);}

    }
    void drawButtonText(juce::Graphics& g, juce::TextButton& button, bool hovered, bool down) override {
        if(!button.getClickingTogglesState()) {
            if(button.getButtonText()=="+" || button.getButtonText()=="-" || button.getButtonText()=="x"){g.setColour(button.findColour(juce::TextButton::textColourOffId));g.setFont(font(12));g.drawText(button.getButtonText(),button.getLocalBounds(),juce::Justification::centred);return;}
            juce::LookAndFeel_V4::drawButtonText(g,button,hovered,down);return;
        }
        const bool selected=button.getToggleState(); const float x=(float)button.getWidth()-47.0f;
        g.setColour(selected?lime:juce::Colour(0xff566057));g.fillRoundedRectangle(x,10,32,16,8);g.setColour(white);g.fillEllipse(x+(selected?18.0f:2.0f),12,12,12);
        g.setColour(selected?white:muted);g.setFont(font(13,true));g.drawText(button.getButtonText().fromFirstOccurrenceOf("   ",false,false),43,0,button.getWidth()-100,button.getHeight(),juce::Justification::centredLeft);
        g.setColour(selected?white:muted); auto name=button.getButtonText();juce::Path icon;
        if(name.contains("Blender")){icon.startNewSubPath(16,8);icon.lineTo(32,8);icon.lineTo(29,24);icon.lineTo(19,24);icon.closeSubPath();icon.startNewSubPath(24,24);icon.lineTo(24,28);icon.startNewSubPath(17,30);icon.lineTo(31,30);g.strokePath(icon,juce::PathStrokeType(1.3f));g.drawLine(18,5,30,5,1.3f);}
        else if(name.contains("Stovetop")){g.drawRoundedRectangle(13,9,22,20,3,1.2f);for(int burnerX:{17,26})for(int burnerY:{13,22})g.drawEllipse((float)burnerX,(float)burnerY,5,5,1);}
        else if(name.contains("Microwave")){g.drawRoundedRectangle(12,10,25,18,3,1.2f);g.drawRoundedRectangle(15,13,15,12,2,1);g.fillEllipse(32,14,2,2);g.fillEllipse(32,20,2,2);}
        else if(name.contains("Air fryer")){g.drawRoundedRectangle(16,6,18,25,5,1.2f);g.drawHorizontalLine(15,18,32);g.drawRoundedRectangle(22,18,7,4,1,1);g.fillEllipse(23,10,3,3);}
        else if(name.contains("Rice")||name.contains("Slow")){g.drawRoundedRectangle(15,14,20,16,5,1.2f);g.drawLine(16,12,34,12,1.2f);g.drawLine(22,9,28,9,1.2f);g.drawLine(12,19,15,19,1.2f);g.drawLine(35,19,38,19,1.2f);}
        else if(name.contains("Deep")){g.drawRoundedRectangle(14,15,21,14,3,1.2f);g.drawLine(18,11,31,11,1.2f);g.drawLine(24,11,24,5,1.2f);g.drawLine(24,5,33,5,1.2f);}
        else{g.drawRoundedRectangle(14,9,21,21,3,1.2f);g.drawHorizontalLine(15,17,32);g.drawRoundedRectangle(17,18,15,8,1,1);g.fillEllipse(18,11,2,2);g.fillEllipse(25,11,2,2);}

    }
    void drawComboBox(juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box) override {
        g.setColour(box.findColour(juce::ComboBox::backgroundColourId));
        g.fillRoundedRectangle(0.5f,0.5f,(float)width-1.0f,(float)height-1.0f,10.0f);
        g.setColour(box.findColour(juce::ComboBox::outlineColourId));
        g.drawRoundedRectangle(0.5f,0.5f,(float)width-1.0f,(float)height-1.0f,10.0f,1.0f);
        juce::Path arrow; arrow.addTriangle((float)width-26.0f,(float)height*0.43f,(float)width-16.0f,(float)height*0.43f,(float)width-21.0f,(float)height*0.57f);
        g.setColour(juce::Colour(0xff88948c)); g.fillPath(arrow);
    }
    void drawTextEditorOutline(juce::Graphics& g, int width, int height, juce::TextEditor& editor) override {
        g.setColour(editor.findColour(juce::TextEditor::outlineColourId));
        g.drawRoundedRectangle(0.5f,0.5f,(float)width-1.0f,(float)height-1.0f,8.0f,1.0f);
    }
};

class PantryRow final : public juce::Component, private juce::Button::Listener {
public:
    PantryRow(PantryItem item, std::function<void(double)> change, std::function<void()> removeFn)
        : value(std::move(item)), onChange(std::move(change)), onRemove(std::move(removeFn)),photo(StudioAssets::foodImage(value.food)) {
        addAndMakeVisible(name); name.setText(juce::String(value.food.name), juce::dontSendNotification); name.setColour(juce::Label::textColourId, white); name.setFont(font(12, true));
        addAndMakeVisible(amount); amount.setText(juce::String((int)value.grams), juce::dontSendNotification); amount.setInputRestrictions(6, "0123456789."); amount.setJustification(juce::Justification::centred); amount.setColour(juce::TextEditor::backgroundColourId, panel2); amount.setColour(juce::TextEditor::textColourId, white); amount.setColour(juce::TextEditor::outlineColourId, line); amount.setFont(font(13));
        amount.onReturnKey = [this] { commit(); }; amount.onFocusLost = [this] { commit(); };
        minus.setButtonText("-"); plus.setButtonText("+"); remove.setButtonText("x");
        for (auto* b : {&minus,&plus,&remove}) { addAndMakeVisible(*b); b->addListener(this); b->setColour(juce::TextButton::buttonColourId, panel2); b->setColour(juce::TextButton::textColourOffId, muted); }
        remove.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffd99a8e));
    }
    void paint(juce::Graphics& g) override {
        auto r=getLocalBounds().toFloat().reduced(.7f);g.setGradientFill(juce::ColourGradient(juce::Colour(0xff2c2f23),0,0,juce::Colour(0xff20221b),0,(float)getHeight(),false));g.fillRoundedRectangle(r,29);g.setColour(line);g.drawRoundedRectangle(r,29,.8f);
        if(photo.isValid())g.drawImage(photo,juce::Rectangle<float>(5,8,53,49),juce::RectanglePlacement::centred);else StudioAssets::drawFoodGlyph(g,value.food,{5,8,53,49});
        g.setColour(muted);g.setFont(font(9));g.drawText("Weight",63,31,39,21,juce::Justification::left);g.drawText("g",getWidth()-37,31,15,21,juce::Justification::centred);
    }
    void resized() override {
        const auto w=getWidth();name.setBounds(62,9,w-87,19);remove.setBounds(w-26,6,20,20);
        minus.setBounds(w-112,32,18,19);amount.setBounds(w-94,31,50,21);plus.setBounds(w-20,32,17,19);
        for(auto* b:{&minus,&plus,&remove})b->setColour(juce::TextButton::buttonColourId,juce::Colours::transparentBlack);
    }
    PantryItem item() const { auto v=value; v.grams=amount.getText().getDoubleValue(); return v; }
private:
    void commit() { auto n=juce::jlimit(1.0, 99999.0, amount.getText().getDoubleValue()); value.grams=n; amount.setText(juce::String((int)n),juce::dontSendNotification); onChange(n); }
    void buttonClicked(juce::Button* b) override { if (b==&remove) onRemove(); else { auto v=amount.getText().getDoubleValue(); amount.setText(juce::String(juce::jmax(1.0, v+(b==&plus ? 50.0 : -50.0))),juce::dontSendNotification); commit(); } }
    PantryItem value; std::function<void(double)> onChange; std::function<void()> onRemove; juce::Label name; juce::TextEditor amount; juce::TextButton minus,plus,remove;juce::Image photo;
};
}

class KitchenWindow::MainView final : public juce::Component, private juce::Button::Listener, private juce::ComboBox::Listener, private juce::Timer {
public:
    MainView() : recipesFile(juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getChildFile("FridgeToTable/saved-recipes.json")) {
        setOpaque(true); setLookAndFeel(&studioLookAndFeel);
        auto executable = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        heroImage = juce::ImageFileFormat::loadFrom(executable.getParentDirectory().getParentDirectory().getChildFile("Resources/studio-hero.png"));
        addAndMakeVisible(brand); brand.setText("Fridge to Table",juce::dontSendNotification); brand.setColour(juce::Label::textColourId,white); brand.setFont(font(14,true));
        addAndMakeVisible(tagline); tagline.setText("AI COOKING ASSISTANT",juce::dontSendNotification); tagline.setColour(juce::Label::textColourId,muted); tagline.setFont(font(10,true));
        addNav("Create", ""); addNav("Saved recipes", ""); addNav("My kitchen", ""); addNav("Guide", ""); addNav("Codex Setup", "");
        addAndMakeVisible(pageTitle); pageTitle.setText("From your kitchen\nto a great meal",juce::dontSendNotification); pageTitle.setColour(juce::Label::textColourId,white); pageTitle.setFont(juce::Font(juce::FontOptions("Georgia", 42.0f, juce::Font::plain)));
        addAndMakeVisible(subtitle); subtitle.setText("Tell me what you have. I'll make it work.",juce::dontSendNotification); subtitle.setColour(juce::Label::textColourId,muted); subtitle.setFont(font(14));
        addAndMakeVisible(sectionIngredients); sectionIngredients.setText("Ingredients",juce::dontSendNotification); sectionIngredients.setColour(juce::Label::textColourId,white); sectionIngredients.setFont(font(18,true));
        addAndMakeVisible(sectionAppliances); sectionAppliances.setText("Appliances",juce::dontSendNotification); sectionAppliances.setColour(juce::Label::textColourId,white); sectionAppliances.setFont(font(18,true));
        addAndMakeVisible(sectionPeople); sectionPeople.setText("WHO'S AT THE TABLE?",juce::dontSendNotification); sectionPeople.setColour(juce::Label::textColourId,muted); sectionPeople.setFont(font(10,true));
        addAndMakeVisible(catalogue); catalogue.setEditableText(false); catalogue.setTextWhenNothingSelected("Quick add an ingredient..."); catalogue.setJustificationType(juce::Justification::centredLeft); catalogue.setColour(juce::ComboBox::backgroundColourId,panel2); catalogue.setColour(juce::ComboBox::textColourId,white); catalogue.setColour(juce::ComboBox::outlineColourId,line);
        int id=1; for (const auto& food : IngredientCatalog::all()) { catalogue.addItem(juce::String(food.name)+"  /  "+juce::String(food.category),id++); }
        catalogue.addListener(this);
        add.setButtonText("+  Add ingredient"); add.setColour(juce::TextButton::buttonColourId,juce::Colours::transparentBlack); add.setColour(juce::TextButton::textColourOffId,lime); add.addListener(this); addAndMakeVisible(add);
        addAndMakeVisible(hint); hint.setText("Your ingredients. Your quantities. A meal made around them.",juce::dontSendNotification); hint.setColour(juce::Label::textColourId,muted); hint.setFont(font(11));
        addAndMakeVisible(pantryViewport); pantryViewport.setViewedComponent(&pantryContent,false); pantryViewport.setScrollBarsShown(true,false); pantryViewport.setColour(juce::ScrollBar::thumbColourId,juce::Colour(0xff59635b));
        peopleMinus.setButtonText("-"); peoplePlus.setButtonText("+"); for(auto* b:{&peopleMinus,&peoplePlus}) { addAndMakeVisible(*b); b->addListener(this); b->setColour(juce::TextButton::buttonColourId,panel2); b->setColour(juce::TextButton::textColourOffId,white); }
        addAndMakeVisible(peopleCount); peopleCount.setText("4",juce::dontSendNotification); peopleCount.setColour(juce::Label::textColourId,white); peopleCount.setFont(font(16,true)); peopleCount.setJustificationType(juce::Justification::centred);
        int i=0; for (auto item : {"Oven","Air fryer","Blender","Microwave","Stovetop","Rice cooker","Slow cooker","Deep fryer"}) { applianceButtons.emplace_back(std::make_unique<juce::TextButton>(juce::String("o   ")+item)); auto* b=applianceButtons.back().get(); b->setClickingTogglesState(true); b->setToggleState(i==4,juce::dontSendNotification); b->setColour(juce::TextButton::buttonColourId,panel2); b->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff33420f)); b->setColour(juce::TextButton::textColourOffId,muted); b->setColour(juce::TextButton::textColourOnId,lime); b->addListener(this); addAndMakeVisible(*b); ++i; }
        addAndMakeVisible(generate); generate.setButtonText("Generate recipes"); generate.setColour(juce::TextButton::buttonColourId,lime); generate.setColour(juce::TextButton::textColourOffId,juce::Colour(0xff1b2410)); generate.addListener(this);
        addAndMakeVisible(progressBar); progressBar.setPercentageDisplay(false); progressBar.setColour(juce::ProgressBar::foregroundColourId,lime); progressBar.setColour(juce::ProgressBar::backgroundColourId,panel2); progressBar.setVisible(false);
        addAndMakeVisible(nextRecipe); nextRecipe.setButtonText("Decline - next recipe"); nextRecipe.setColour(juce::TextButton::buttonColourId,panel2);nextRecipe.setColour(juce::TextButton::textColourOffId,white);nextRecipe.onClick=[this]{if(recipeChoices.empty())return;choiceIndex=(choiceIndex+1)%recipeChoices.size();recipe=recipeChoices[choiceIndex];updateRecipe();requestPicture(false);};nextRecipe.setEnabled(false);
        addAndMakeVisible(status); status.setText("Ready when you are",juce::dontSendNotification); status.setColour(juce::Label::textColourId,muted); status.setFont(font(11));
        addAndMakeVisible(recipeTitle); recipeTitle.setText("Tonight, sorted.",juce::dontSendNotification); recipeTitle.setColour(juce::Label::textColourId,white); recipeTitle.setFont(juce::Font(juce::FontOptions("Georgia",25.0f,juce::Font::bold)));
        addAndMakeVisible(recipeDescription); recipeDescription.setColour(juce::Label::textColourId,muted); recipeDescription.setFont(font(12)); recipeDescription.setMinimumHorizontalScale(0.8f); recipeDescription.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(recipeMeta); recipeMeta.setColour(juce::Label::textColourId,lime); recipeMeta.setFont(font(12,true));
        addAndMakeVisible(calorieNumber); calorieNumber.setColour(juce::Label::textColourId,white); calorieNumber.setFont(font(23,true));
        addAndMakeVisible(calorieCaption); calorieCaption.setText("est. kcal / person",juce::dontSendNotification); calorieCaption.setColour(juce::Label::textColourId,muted); calorieCaption.setFont(font(10));
        addAndMakeVisible(ingredientsHeading); ingredientsHeading.setText("WHAT GOES IN",juce::dontSendNotification); ingredientsHeading.setColour(juce::Label::textColourId,lime); ingredientsHeading.setFont(font(10,true));
        addAndMakeVisible(recipeIngredients); recipeIngredients.setColour(juce::Label::textColourId,white); recipeIngredients.setFont(font(12)); recipeIngredients.setJustificationType(juce::Justification::topLeft);
        addAndMakeVisible(stepsHeading); stepsHeading.setText("THE METHOD",juce::dontSendNotification); stepsHeading.setColour(juce::Label::textColourId,lime); stepsHeading.setFont(font(10,true));
        addAndMakeVisible(recipeSteps); recipeSteps.setColour(juce::Label::textColourId,white); recipeSteps.setFont(font(12)); recipeSteps.setJustificationType(juce::Justification::topLeft);
        viewRecipe.setButtonText("View full recipe"); addAndMakeVisible(viewRecipe); viewRecipe.onClick=[this]{showFullRecipe();}; viewRecipe.setColour(juce::TextButton::buttonColourId,lime); viewRecipe.setColour(juce::TextButton::textColourOffId,bg);
        save.setButtonText("Save recipe"); share.setButtonText("Send to..."); pictureShare.setButtonText("Send Picture to...");exportButton.setButtonText("Export");
        for(auto* b:{&save,&share,&pictureShare,&exportButton}) { addAndMakeVisible(*b); b->addListener(this); b->setColour(juce::TextButton::buttonColourId,panel2); b->setColour(juce::TextButton::textColourOffId,white); }
        addAndMakeVisible(footer); footer.setText("POWERED BY CODEX   |   NUTRITION IS AN ESTIMATE",juce::dontSendNotification); footer.setColour(juce::Label::textColourId,juce::Colour(0xff727b74)); footer.setFont(font(9,true));
        ingredientsHeading.setVisible(false);recipeIngredients.setVisible(false);stepsHeading.setVisible(false);recipeSteps.setVisible(false);
        nav.front()->setToggleState(true,juce::dontSendNotification);
        restoreSaved(); if(!savedRecipes.empty()) recipe=savedRecipes.front(); seedPantry(); refreshPantry(); updateRecipe();
        engine.setExecutable(codexSettings().loadFileAsString().trim());
        if (!codexSettings().existsAsFile()) {
            juce::Component::SafePointer<MainView> safe(this);
            juce::MessageManager::callAsync([safe] { if (safe) safe->showCodexSetup(); });
        }
    }
    ~MainView() override { engine.cancel(); setLookAndFeel(nullptr); }

    void paint(juce::Graphics& g) override {
        g.fillAll(bg);float w=(float)getWidth();
        g.setColour(juce::Colour(0xff1e201b));g.fillRect(0,0,getWidth(),52);g.setColour(line.withAlpha(.5f));g.drawHorizontalLine(51,0,w);
        g.setColour(lime);g.drawRoundedRectangle(28,16,22,20,5,1.5f);g.drawEllipse(27,10,9,10,1.5f);g.drawEllipse(34,8,11,11,1.5f);g.drawEllipse(43,11,9,9,1.5f);
        if(heroImage.isValid()){juce::Graphics::ScopedSaveState state(g);g.reduceClipRegion(juce::Rectangle<int>(0,52,getWidth(),204));g.drawImage(heroImage,juce::Rectangle<float>(0,52,w,240),juce::RectanglePlacement::fillDestination);g.setGradientFill(juce::ColourGradient(bg.withAlpha(.05f),0,150,bg,0,256,false));g.fillRect(0,52,getWidth(),204);}
        g.setColour(line.withAlpha(.45f));g.drawHorizontalLine(418,38,w-38);g.drawHorizontalLine(575,38,w-38);
        auto card=juce::Rectangle<float>(38,696,w-76,174);g.setColour(juce::Colour(0xff1b1d16));g.fillRoundedRectangle(card,18);g.setColour(juce::Colour(0xff737d36));g.drawRoundedRectangle(card,18,.9f);
        if(recipeImage.isValid()){juce::Graphics::ScopedSaveState state(g);juce::Path mask;mask.addRoundedRectangle(38,696,280,174,18);g.reduceClipRegion(mask);g.drawImage(recipeImage,juce::Rectangle<float>(38,696,280,174),juce::RectanglePlacement::fillDestination);}
        else {g.setColour(panel2);g.fillRoundedRectangle(39,697,278,172,17);g.setColour(muted);g.setFont(font(12));g.drawFittedText(busy?"Creating your dish photograph...":"Your next favourite meal starts here",67,747,220,65,juce::Justification::centred,3);}
        g.setColour(lime);g.setFont(font(9,true));g.drawText("A RECIPE AROUND YOUR KITCHEN",340,705,getWidth()-390,18,juce::Justification::left);
    }
    void resized() override {
        int w=getWidth();int content=w-76;
        brand.setBounds(64,14,235,24);tagline.setBounds(38,76,350,18);tagline.setColour(juce::Label::textColourId,lime);tagline.setFont(font(10,true));
        for(size_t i=0;i<nav.size();++i)nav[i]->setBounds(w-568+(int)i*106,11,100,30);
        pageTitle.setBounds(34,103,660,102);subtitle.setBounds(38,211,570,39);
        sectionIngredients.setBounds(38,269,260,28);catalogue.setBounds(w-548,270,316,30);add.setBounds(w-224,269,188,32);hint.setBounds(38,306,content,18);
        pantryViewport.setBounds(35,333,w-66,76);int columns=4;int cell=(w-92)/columns;int rows=(int)((pantryRows.size()+3)/4);pantryContent.setSize(w-82,juce::jmax(68,rows*72));
        for(size_t i=0;i<pantryRows.size();++i)pantryRows[i]->setBounds(3+(int)(i%4)*cell,(int)(i/4)*72,cell-9,64);
        sectionAppliances.setBounds(38,432,260,27);
        for(size_t i=0;i<applianceButtons.size();++i)applianceButtons[i]->setBounds(38+(int)(i%4)*((content+12)/4),469+(int)(i/4)*44,(content-36)/4,35);
        sectionPeople.setBounds(38,553,160,18);peopleMinus.setBounds(192,546,28,28);peopleCount.setBounds(221,546,35,28);peoplePlus.setBounds(257,546,28,28);
        generate.setBounds(38,591,content,44);status.setBounds(38,643,content,18);progressBar.setBounds(38,668,content,15);
        recipeTitle.setBounds(340,728,w-520,49);recipeMeta.setBounds(340,780,w-390,19);recipeDescription.setBounds(340,805,w-540,24);
        calorieNumber.setBounds(w-153,730,103,32);calorieCaption.setBounds(w-153,765,112,18);
        viewRecipe.setBounds(340,835,155,29);save.setBounds(38,884,124,30);share.setBounds(171,884,100,30);pictureShare.setBounds(280,884,155,30);exportButton.setBounds(444,884,83,30);nextRecipe.setBounds(w-235,884,197,30);footer.setBounds(540,890,juce::jmax(0,w-785),17);
    }
    void setNav(const juce::String& name) { pageTitle.setText(name=="My recipes" ? "Your saved\nfavourites." : "From your kitchen\nto a great meal",juce::dontSendNotification); }
private:
    juce::File codexSettings() const {
        return recipesFile.getSiblingFile("codex-executable.txt");
    }

    void showCodexSetup() {
        class Setup final : public juce::Component {
        public:
            Setup(juce::String savedPath, std::function<bool(const juce::String&)> savePath) {
                setSize(620, 278);
                auto detected = CodexEngine::detectedExecutable();
                intro.setText("Fridge to Table uses Codex on this Mac. Install Codex and sign in before testing the connection.", juce::dontSendNotification);
                pathLabel.setText("Codex executable path (optional)", juce::dontSendNotification);
                path.setText(savedPath, false);
                path.setTextToShowWhenEmpty(detected.isEmpty() ? "Enter the full path to Codex" : detected, muted);
                feedback.setText("Leave the path blank to use automatic detection.", juce::dontSendNotification);
                for (auto* label : {&intro, &pathLabel, &feedback}) {
                    label->setColour(juce::Label::textColourId, white);
                    label->setJustificationType(juce::Justification::centredLeft);
                    addAndMakeVisible(*label);
                }
                addAndMakeVisible(path);
                addAndMakeVisible(test);
                addAndMakeVisible(later);
                intro.setBounds(22, 15, 576, 53);
                pathLabel.setBounds(22, 74, 576, 22);
                path.setBounds(22, 101, 576, 32);
                feedback.setBounds(22, 143, 576, 64);
                later.setBounds(22, 220, 125, 34);
                test.setBounds(425, 220, 173, 34);
                test.setColour(juce::TextButton::buttonColourId, lime);
                test.setColour(juce::TextButton::textColourOffId, bg);
                later.onClick = [this] { close(); };
                test.onClick = [this, savePath] {
                    auto candidate = path.getText().trim();
                    test.setEnabled(false);
                    path.setEnabled(false);
                    feedback.setText("Checking the app server and Codex account…", juce::dontSendNotification);
                    juce::Component::SafePointer<Setup> safe(this);
                    CodexEngine::checkConnection(candidate, [safe, candidate, savePath](const juce::String&, bool, const juce::String& error) {
                        if (!safe) return;
                        if (error.isEmpty() && savePath(candidate)) {
                            safe->close();
                            return;
                        }
                        safe->path.setEnabled(true);
                        safe->test.setEnabled(true);
                        safe->feedback.setText(error.isEmpty() ? "Could not save the Codex settings." : error, juce::dontSendNotification);
                    });
                };
            }

        private:
            void close() {
                if (auto* window = findParentComponentOfClass<juce::DialogWindow>())
                    window->exitModalState(0);
            }
            juce::Label intro, pathLabel, feedback;
            juce::TextEditor path;
            juce::TextButton test{"Test and save"}, later{"Close"};
        };

        juce::Component::SafePointer<MainView> safe(this);
        auto savePath = [safe](const juce::String& path) {
            if (!safe) return false;
            auto file = safe->codexSettings();
            if (file.getParentDirectory().createDirectory().failed() || !file.replaceWithText(path + "\n"))
                return false;
            safe->engine.setExecutable(path);
            safe->status.setText("Codex is ready.", juce::dontSendNotification);
            return true;
        };
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(new Setup(codexSettings().loadFileAsString().trim(), savePath));
        options.dialogTitle = "Set Up Codex";
        options.dialogBackgroundColour = bg;
        options.escapeKeyTriggersCloseButton = true;
        options.useNativeTitleBar = true;
        options.resizable = false;
        options.launchAsync();
    }

    void buttonClicked(juce::Button* b) override {
        if (b == nav.back().get()) { showCodexSetup(); return; }
        if (b==&add) { browseIngredients(); return; }
        if (b==&peopleMinus || b==&peoplePlus) { servings=juce::jlimit(1,12,servings+(b==&peoplePlus?1:-1)); peopleCount.setText(juce::String(servings),juce::dontSendNotification); return; }
        if (b==&generate) { generateRecipe(); return; }
        if (b==&save) { saveRecipe(recipe); return; }
        if (b==&share) { sendRecipeText(recipe); return; }
        if (b==&pictureShare) { sendRecipePicture(recipe); return; }
        if (b==&exportButton) { exportRecipe(); return; }
        for(size_t i=0;i<nav.size();++i) if(b==nav[i].get()) { activeNav=(int)i; for(size_t j=0;j<nav.size();++j) nav[j]->setToggleState(i==j,juce::dontSendNotification); if(i==1) { setNav("My recipes"); showSavedRecipe(); } else if(i==2) { setNav("My kitchen"); status.setText("Your kitchen inventory is ready to edit above.",juce::dontSendNotification); } else if(i==3) { setNav("How it works"); status.setText("Select ingredients, set quantities, choose appliances and people, then create a recipe.",juce::dontSendNotification); } else setNav("Create recipe"); repaint(); }
    }
    void comboBoxChanged(juce::ComboBox* box) override { if(box==&catalogue && catalogue.getSelectedId()>0) addSelected(); }
    void browseIngredients() {
        juce::Component::SafePointer<MainView> safe(this);
        auto* browser=new IngredientBrowser([safe](Food food,double grams){ if(!safe) return; for(auto& item:safe->pantry) if(item.food.name==food.name) { safe->hint.setText("Already selected - edit the grams below.",juce::dontSendNotification); return; } safe->pantry.push_back({food,grams}); safe->refreshPantry(); safe->hint.setText("Added "+juce::String(food.name),juce::dontSendNotification); });
        juce::DialogWindow::LaunchOptions options; options.content.setOwned(browser); options.dialogTitle="Choose your ingredients"; options.dialogBackgroundColour=bg; options.escapeKeyTriggersCloseButton=true; options.useNativeTitleBar=true; options.resizable=false; options.launchAsync();
    }
    void showFullRecipe() {
        if(recipe.steps.empty()) return;
        juce::Component::SafePointer<MainView> safe(this);
        auto* detail=new RecipeDetail(recipe,isRecipeSaved(recipe),
            [safe](const Recipe& r){return safe && safe->saveRecipe(r);},
            [safe](const Recipe& r){return safe && safe->sendRecipeText(r);},
            [safe](const Recipe& r){return safe && safe->sendRecipePicture(r);},
            [safe](const Recipe& r){return safe && safe->exportRecipeText(r);});
        juce::DialogWindow::LaunchOptions options; options.content.setOwned(detail); options.dialogTitle=recipe.title; options.dialogBackgroundColour=bg; options.useNativeTitleBar=true; options.escapeKeyTriggersCloseButton=true; options.resizable=false; options.launchAsync();
    }
    void addSelected() {
        auto text=catalogue.getText().trim(); if(text.isEmpty()) return;
        Food food=IngredientCatalog::custom(text);
        if(catalogue.getSelectedId()>0 && catalogue.getSelectedId()<=(int)IngredientCatalog::all().size()) food=IngredientCatalog::all()[(size_t)catalogue.getSelectedId()-1];
        for (const auto& item : pantry) if (juce::String(item.food.name).equalsIgnoreCase(juce::String(food.name))) { hint.setText("Already in your kitchen - update its quantity below.",juce::dontSendNotification); return; }
        pantry.push_back({food,200.0}); refreshPantry(); catalogue.setText({},juce::dontSendNotification); hint.setText("Added. Add anything else you have on hand.",juce::dontSendNotification);
    }
    void seedPantry() {
        for(const auto& name : {"Tomatoes","Ground pork","Basil","Pasta"}) for(const auto& f:IngredientCatalog::all()) if(name==f.name) pantry.push_back({f, name==std::string("Tomatoes")?200.0:name==std::string("Ground pork")?500.0:name==std::string("Basil")?10.0:300.0});
    }
    void refreshPantry() {
        pantryRows.clear(); pantryContent.removeAllChildren();
        for(size_t i=0;i<pantry.size();++i) {
            auto row=std::make_unique<PantryRow>(pantry[i],[this,i](double n){if(i<pantry.size()) pantry[i].grams=n;},[this,i]{if(i<pantry.size()) pantry.erase(pantry.begin()+(long)i); refreshPantry();});
            pantryContent.addAndMakeVisible(*row); pantryRows.push_back(std::move(row));
        }
        pantryContent.setSize(650,(int)pantryRows.size()*54); for(size_t i=0;i<pantryRows.size();++i) pantryRows[i]->setBounds(3,(int)i*54+2,632,48); resized();
    }
    std::vector<juce::String> selectedAppliances() const { std::vector<juce::String> a; for(const auto& b:applianceButtons) if(b->getToggleState()) a.push_back(b->getButtonText().fromFirstOccurrenceOf("   ",false,false)); return a; }
    void generateRecipe() {
        if(pantry.empty()) { status.setText("Add at least one ingredient to continue.",juce::dontSendNotification); return; }
        if(selectedAppliances().empty()) { status.setText("Choose at least one cooking appliance.",juce::dontSendNotification); return; }
        startProgress(70.0,0.0,0.65); nextRecipe.setEnabled(false); generate.setEnabled(false); status.setText("Starting Codex app-server...",juce::dontSendNotification);
        juce::StringArray appliances; for(auto& a:selectedAppliances()) appliances.add(a);
        auto prompt=buildRecipePrompt(pantry,selectedAppliances(),servings);
        const auto requestedServings=servings; const auto requestedPantry=pantry;
        juce::Component::SafePointer<MainView> safe(this);
        engine.generate(prompt,[this,safe,requestedServings,requestedPantry](const juce::String& response,bool done,const juce::String& error){
            if(!safe) return;
            if(!done) { status.setText(response,juce::dontSendNotification); return; }
            generate.setEnabled(true); finishProgress(false);
            if(error.isNotEmpty()) { status.setText(error,juce::dontSendNotification); return; }
            auto batch=juce::JSON::parse(response); auto* choices=batch.getProperty("recipes",juce::var()).getArray();
            if(!choices || choices->size()<5) {status.setText("Codex must return at least five complete recipe choices. Please retry.",juce::dontSendNotification);return;}
            std::vector<Recipe> validated;
            for(const auto& raw:*choices) {
                Recipe next; juce::String errorText;
                if(!recipeFromJSON(juce::JSON::toString(raw,true),next,errorText) || next.servings!=requestedServings) {status.setText("A recipe in the collection is incomplete or has the wrong serving count. Please retry.",juce::dontSendNotification);return;}
                for(auto& ingredient:next.ingredients){bool found=false;for(const auto& item:requestedPantry) if(juce::String(item.food.name).equalsIgnoreCase(juce::String(ingredient.name))){found=true;if(ingredient.grams>item.grams+0.1){status.setText("A recipe exceeds your available ingredient weights. Please retry.",juce::dontSendNotification);return;}}if(!found){status.setText("A recipe uses an unavailable ingredient. Please retry.",juce::dontSendNotification);return;}ingredient.inPantry=true;}
                bool complete=true;double calories=0;for(auto& ingredient:next.ingredients){bool known=false;for(auto& food:IngredientCatalog::all())if(juce::String(food.name).equalsIgnoreCase(juce::String(ingredient.name))&&food.kcalPer100g>=0){calories+=food.kcalPer100g*ingredient.grams/100.0;known=true;break;}if(!known)complete=false;}if(complete)next.caloriesPerServing=(int)std::round(calories/next.servings);
                next.imagePath.clear();
                validated.push_back(std::move(next));
            }
            recipeChoices=std::move(validated);choiceIndex=0;recipe=recipeChoices.front();nextRecipe.setEnabled(true);
            updateRecipe(); requestPicture(true);
        });
    }
    void startProgress(double seconds,double base=0.0,double span=0.95) {
        busy=true;progressStarted=juce::Time::getMillisecondCounterHiRes();progressDuration=seconds;progressBase=base;progressSpan=span;progressValue=base;progressBar.setVisible(true);startTimerHz(12);
    }
    void finishProgress(bool success) {
        busy=false;stopTimer();progressValue=success?1.0:0.0;progressBar.setTextToDisplay(success?"Complete - 100%":"");
        juce::Component::SafePointer<MainView> safe(this);juce::Timer::callAfterDelay(1200,[safe]{if(safe&&!safe->busy)safe->progressBar.setVisible(false);});
    }
    void timerCallback() override {
        auto elapsed=(juce::Time::getMillisecondCounterHiRes()-progressStarted)/1000.0;
        progressValue=progressBase+progressSpan*juce::jmin(0.98,1.0-std::exp(-2.0*elapsed/progressDuration));
        progressBar.setTextToDisplay("Estimated progress - "+juce::String((int)(progressValue*100))+"%");
    }
    void requestPicture(bool afterPlanning) {
        auto prefix="Recipe "+juce::String((int)choiceIndex+1)+" of "+juce::String((int)recipeChoices.size());
        if(juce::File(recipe.imagePath).existsAsFile()) {finishProgress(true);status.setText(prefix+" - picture ready",juce::dontSendNotification);return;}
        startProgress(35.0,afterPlanning?0.65:0.0,afterPlanning?0.30:0.95);generate.setEnabled(false);nextRecipe.setEnabled(false);
        status.setText(prefix+" - creating its dish picture...",juce::dontSendNotification);
        juce::Component::SafePointer<MainView> safe(this);const auto requestedIndex=choiceIndex;
        engine.generatePicture(recipe,[this,safe,requestedIndex,prefix](const juce::String& response,bool done,const juce::String& error){
            if(!safe)return;if(!done)return;
            generate.setEnabled(true);nextRecipe.setEnabled(true);
            if(error.isNotEmpty()){finishProgress(false);status.setText(error,juce::dontSendNotification);return;}
            juce::File generatedImage(response);auto preview=juce::ImageFileFormat::loadFrom(generatedImage);
            if(!preview.isValid()){finishProgress(false);status.setText("The dish picture could not be loaded.",juce::dontSendNotification);return;}
            auto directory=recipesFile.getParentDirectory().getChildFile("DishImages");directory.createDirectory();auto imageFile=directory.getChildFile(juce::Uuid().toString()+".jpg");
            const auto scale=juce::jmin(1.0,768.0/juce::jmax(preview.getWidth(),preview.getHeight()));
            if(scale<1.0)preview=preview.rescaled(juce::jmax(1,(int)std::round(preview.getWidth()*scale)),juce::jmax(1,(int)std::round(preview.getHeight()*scale)),juce::Graphics::highResamplingQuality);
            juce::JPEGImageFormat jpeg;jpeg.setQuality(0.82f);bool saved=false;{juce::FileOutputStream out(imageFile);if(out.openedOk())saved=jpeg.writeImageToStream(preview,out);}
            if(!saved){finishProgress(false);status.setText("Could not save the compact dish picture.",juce::dontSendNotification);return;}
            if(requestedIndex<recipeChoices.size())recipeChoices[requestedIndex].imagePath=imageFile.getFullPathName().toStdString();
            if(choiceIndex==requestedIndex){recipe=recipeChoices[requestedIndex];updateRecipe();}
            finishProgress(true);status.setText(prefix+" ready - decline to see the next proposal",juce::dontSendNotification);
        });
    }
    void updateRecipe() {
        recipeImage=juce::ImageFileFormat::loadFrom(juce::File(recipe.imagePath));
        recipeTitle.setText(recipe.title,juce::dontSendNotification); recipeDescription.setText(recipe.description,juce::dontSendNotification);
        recipeMeta.setText(juce::String(recipe.minutes)+" MIN     |     "+juce::String(recipe.servings)+" SERVINGS     |     "+juce::String(recipe.appliance).toUpperCase(),juce::dontSendNotification);
        calorieNumber.setText(juce::String(recipe.caloriesPerServing),juce::dontSendNotification);
        juce::String ing; for(const auto& item:recipe.ingredients) ing += (item.inPantry?"IN  ":"ADD  ")+juce::String(item.name)+"   " + juce::String((int)std::round(item.grams))+" g\n";
        if(ing.isEmpty()) ing="Your selected ingredients\nwill appear here.";
        recipeIngredients.setText(ing,juce::dontSendNotification);
        juce::String steps; for(size_t i=0;i<recipe.steps.size();++i) steps += juce::String(i+1)+".  "+juce::String(recipe.steps[i])+"\n\n";
        if(steps.isEmpty()) steps="Generate a recipe to see\nthe cooking steps.";
        recipeSteps.setText(steps,juce::dontSendNotification);save.setButtonText(isRecipeSaved(recipe)?"Saved":"Save recipe");pictureShare.setEnabled(juce::File(recipe.imagePath).existsAsFile());repaint();
    }
    bool isRecipeSaved(const Recipe& r) const {
        auto text=recipeAsPlainText(r);for(const auto& saved:savedRecipes)if(recipeAsPlainText(saved)==text)return true;return false;
    }
    bool saveRecipe(const Recipe& r) {
        if(r.steps.empty() || r.ingredients.empty()) {status.setText("Create a recipe before saving or sharing it.",juce::dontSendNotification);return false;}
        auto previous=savedRecipes;auto text=recipeAsPlainText(r);
        savedRecipes.erase(std::remove_if(savedRecipes.begin(),savedRecipes.end(),[&](const Recipe& saved){return recipeAsPlainText(saved)==text;}),savedRecipes.end());
        savedRecipes.insert(savedRecipes.begin(),r);
        if(!persistSaved()){savedRecipes=std::move(previous);status.setText("Could not save the recipe. Check local folder permissions.",juce::dontSendNotification);return false;}
        save.setButtonText(isRecipeSaved(recipe)?"Saved":"Save recipe");status.setText("Saved to My recipes",juce::dontSendNotification);return true;
    }
    bool prepareRecipeText(const Recipe& r) {
        if(!saveRecipe(r))return false;
        auto text=recipeAsPlainText(r);auto directory=recipesFile.getParentDirectory().getChildFile("RecipeTexts");
        auto name=juce::File::createLegalFileName(juce::String(r.title)).substring(0,100)+"-"+juce::String::toHexString(text.hashCode64())+".txt";
        if(directory.createDirectory().failed() || !directory.getChildFile(name).replaceWithText(text)){
            status.setText("Recipe saved, but its TXT copy could not be written. Sharing was stopped.",juce::dontSendNotification);return false;
        }
        return true;
    }
    bool sendRecipeText(const Recipe& r) {
        if(!prepareRecipeText(r))return false;
        auto* owner=juce::Component::getCurrentlyModalComponent();auto text=recipeAsPlainText(r);ShareRecipeText(text.toRawUTF8(),(owner?owner:this)->getWindowHandle());status.setText("Recipe and TXT saved. Text share sheet opened.",juce::dontSendNotification);return true;
    }
    bool sendRecipePicture(const Recipe& r) {
        if(!juce::File(r.imagePath).existsAsFile()){status.setText("This recipe has no dish picture available yet.",juce::dontSendNotification);return false;}
        if(!saveRecipe(r))return false;
        auto* owner=juce::Component::getCurrentlyModalComponent();ShareRecipeImage(r.imagePath.c_str(),(owner?owner:this)->getWindowHandle());status.setText("Picture-only share sheet opened.",juce::dontSendNotification);return true;
    }
    void showSavedRecipe() {
        if(savedRecipes.empty()) { status.setText("No saved recipes yet. Use Save recipe to keep one.",juce::dontSendNotification); return; }
        juce::Component::SafePointer<MainView> safe(this);
        auto* library=new SavedRecipeLibrary(savedRecipes,
            [safe](const Recipe& r){if(!safe)return false;safe->recipe=r;safe->updateRecipe();safe->showFullRecipe();return true;},
            [safe](const Recipe& r){return safe && safe->sendRecipeText(r);},
            [safe](const Recipe& r){return safe && safe->sendRecipePicture(r);},
            [safe](const Recipe& r){return safe && safe->exportRecipeText(r);});
        juce::DialogWindow::LaunchOptions options;options.content.setOwned(library);options.dialogTitle="Saved recipes";options.dialogBackgroundColour=bg;options.useNativeTitleBar=true;options.escapeKeyTriggersCloseButton=true;options.resizable=false;options.launchAsync();
    }
    void exportRecipe() {
        if(recipe.steps.empty())return;
        juce::PopupMenu menu;menu.addItem(1,"Plain text (.txt)");menu.addItem(2,"Markdown (.md)");auto snapshot=recipe;
        juce::Component::SafePointer<MainView> safe(this);menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&exportButton),[safe,snapshot](int choice){if(!safe || choice==0)return;if(choice==1)safe->exportRecipeText(snapshot);else safe->exportRecipeFile(snapshot,false);});
    }
    bool exportRecipeText(const Recipe& r) {
        if(!prepareRecipeText(r))return false;exportRecipeFile(r,true);return true;
    }
    void exportRecipeFile(const Recipe& r,bool plainText) {
        auto extension=plainText?".txt":".md";auto content=plainText?recipeAsPlainText(r):recipeAsMarkdown(r);
        auto chooser=std::make_shared<juce::FileChooser>("Export recipe",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile(juce::File::createLegalFileName(r.title)+extension),"*"+juce::String(extension));
        juce::Component::SafePointer<MainView> safe(this);
        chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe,chooser,content,extension](const juce::FileChooser& fc){auto f=fc.getResult();if(!safe || f==juce::File())return;f=f.withFileExtension(extension);safe->status.setText(f.replaceWithText(content)?"Recipe exported to "+f.getFullPathName():"Could not write the exported recipe.",juce::dontSendNotification);});
    }
    bool persistSaved() {
        juce::Array<juce::var> array;
        for(const auto& r:savedRecipes) {auto* o=new juce::DynamicObject();o->setProperty("imagePath",juce::String(r.imagePath));o->setProperty("markdown",recipeAsMarkdown(r));o->setProperty("plainText",recipeAsPlainText(r));o->setProperty("title",juce::String(r.title));o->setProperty("description",juce::String(r.description));o->setProperty("minutes",r.minutes);o->setProperty("servings",r.servings);o->setProperty("kcal",r.caloriesPerServing);o->setProperty("appliance",juce::String(r.appliance));juce::Array<juce::var> ingredients;for(auto& x:r.ingredients){auto* i=new juce::DynamicObject();i->setProperty("name",juce::String(x.name));i->setProperty("grams",x.grams);i->setProperty("inPantry",x.inPantry);ingredients.add(juce::var(i));}o->setProperty("ingredients",ingredients);juce::Array<juce::var> steps;for(auto& x:r.steps)steps.add(juce::String(x));o->setProperty("steps",steps);array.add(juce::var(o));}
        if(recipesFile.getParentDirectory().createDirectory().failed()) return false; return recipesFile.replaceWithText(juce::JSON::toString(juce::var(array),true));
    }
    void restoreSaved() {
        if(!recipesFile.existsAsFile()) return; auto data=juce::JSON::parse(recipesFile.loadFileAsString()); auto* array=data.getArray(); if(!array)return;
        for(auto item:*array){auto* o=item.getDynamicObject();if(!o)continue;Recipe r;r.imagePath=o->getProperty("imagePath").toString().toStdString();r.title=o->getProperty("title").toString().toStdString();r.description=o->getProperty("description").toString().toStdString();r.minutes=(int)o->getProperty("minutes");r.servings=(int)o->getProperty("servings");r.caloriesPerServing=(int)o->getProperty("kcal");r.appliance=o->getProperty("appliance").toString().toStdString();if(auto* a=o->getProperty("ingredients").getArray())for(auto x:*a)if(auto* q=x.getDynamicObject())r.ingredients.push_back({q->getProperty("name").toString().toStdString(),(double)q->getProperty("grams"),(bool)q->getProperty("inPantry")});if(auto* a=o->getProperty("steps").getArray())for(auto x:*a)r.steps.push_back(x.toString().toStdString());savedRecipes.push_back(std::move(r));}
    }
    void addNav(const juce::String& title, const juce::String&) { nav.push_back(std::make_unique<juce::TextButton>(title));auto* b=nav.back().get();b->setColour(juce::TextButton::buttonColourId,juce::Colours::transparentBlack);b->setColour(juce::TextButton::buttonOnColourId,juce::Colour(0xff303b18));b->setColour(juce::TextButton::textColourOffId,muted);b->setColour(juce::TextButton::textColourOnId,lime);b->setClickingTogglesState(false);b->addListener(this);addAndMakeVisible(*b);}

    bool busy=false; double progressStarted=0,progressDuration=35,progressBase=0,progressSpan=.95; double progressValue=0.0; juce::ProgressBar progressBar{progressValue}; juce::TextButton nextRecipe; std::vector<Recipe> recipeChoices; size_t choiceIndex=0;
    juce::File recipesFile; std::vector<PantryItem> pantry; std::vector<Recipe> savedRecipes; Recipe recipe; CodexEngine engine; StudioLookAndFeel studioLookAndFeel; juce::Image heroImage,recipeImage; int servings=4, activeNav=0;
    juce::Label brand,tagline,pageTitle,subtitle,sectionIngredients,sectionAppliances,sectionPeople,hint,peopleCount,status,recipeTitle,recipeDescription,recipeMeta,calorieNumber,calorieCaption,ingredientsHeading,recipeIngredients,stepsHeading,recipeSteps,footer;
    juce::ComboBox catalogue; juce::TextButton add,peopleMinus,peoplePlus,generate,save,share,pictureShare,exportButton,viewRecipe;
    juce::Viewport pantryViewport; juce::Component pantryContent; std::vector<std::unique_ptr<PantryRow>> pantryRows;
    std::vector<std::unique_ptr<juce::TextButton>> applianceButtons, nav;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainView)
};

KitchenWindow::KitchenWindow() : juce::DocumentWindow("Fridge to Table",bg,juce::DocumentWindow::allButtons,true) {
    view=std::make_unique<MainView>();setContentOwned(view.get(),true);view.release();setUsingNativeTitleBar(true);setResizable(true,true);centreWithSize(1040,930);setResizeLimits(980,930,1280,1100);setVisible(true);toFront(true);ActivateFridgeToTable();
    // Build the product rail after native window creation.
    if(auto* v=dynamic_cast<MainView*>(getContentComponent())) {
        // nav controls are initialized below through a tiny setup bridge.
        juce::ignoreUnused(v);
    }
}
KitchenWindow::~KitchenWindow() = default;
void KitchenWindow::closeButtonPressed() { juce::JUCEApplication::getInstance()->systemRequestedQuit(); }


