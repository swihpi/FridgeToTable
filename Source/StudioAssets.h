#pragma once
#include "Domain.h"
#include <juce_gui_extra/juce_gui_extra.h>
namespace StudioAssets {
inline juce::File resource(const juce::String& name){return juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory().getParentDirectory().getChildFile("Resources/"+name);}
inline const juce::Image& atlas(){static const auto image=juce::ImageFileFormat::loadFrom(resource("ingredient-atlas.png"));return image;}
inline juce::Image vegetableImage(const Food& food){
 // Stable row-major order in the dedicated photographic vegetable sheet.
 static const char* names[]={"sweet potatoes","red onions","shallots","garlic","leeks","spring onions","parsnips","celery root","celery stalks","beetroot","cabbage","red cabbage","savoy cabbage","kale","spinach","swiss chard"};
 static const auto sheet=juce::ImageFileFormat::loadFrom(resource("vegetables-through-chard.png"));
 if(!sheet.isValid())return {};
 auto name=juce::String(food.name).toLowerCase();
 for(int i=0;i<16;++i)if(name==names[i]){
  int x0=(i%4)*sheet.getWidth()/4,y0=(i/4)*sheet.getHeight()/4;
  int x1=(i%4+1)*sheet.getWidth()/4,y1=(i/4+1)*sheet.getHeight()/4;
  return sheet.getClippedImage({x0,y0,x1-x0,y1-y0});
 }
 return {};
}
inline int foodIndex(const Food& food){
 auto n=juce::String(food.name).toLowerCase();
 if(n.contains("tomato"))return 0;if(n.contains("ground")||n.contains("minced"))return 1;if(n=="basil")return 2;
 if(n=="pasta"||n=="penne"||n=="rigatoni"||n=="macaroni"||n=="fusilli")return 3;
 if(n=="carrots")return 4;if(n=="potatoes")return 5;if(n=="onions")return 6;if(n=="mushrooms"||n=="button mushrooms")return 7;if(n=="red bell pepper"||n=="bell pepper")return 8;
 if(n=="sea bream"||n=="dorada")return 9;if(n=="shrimp"||n=="prawns")return 10;if(n=="eggs")return 11;if(n=="parmesan"||n=="cheddar"||n=="cheese")return 12;if(n=="milk"||n=="whole milk")return 13;
 if(n.contains("rice")&&!n.contains("noodle")&&!n.contains("flour"))return 14;if(n=="red kidney beans"||n=="kidney beans")return 15;if(n=="apples")return 16;if(n=="olive oil")return 17;return -1;
}
inline juce::Image foodImage(const Food& food){
 auto vegetable=vegetableImage(food);if(vegetable.isValid())return vegetable;
 const auto& source=atlas();int i=foodIndex(food);if(!source.isValid()||i<0)return {};
 static const int rects[19][4]={{15,30,300,280},{330,45,295,255},{635,20,295,290},{960,30,300,270},{0,350,275,265},{280,360,250,240},{535,340,225,270},{780,350,245,250},{1040,340,215,265},{0,690,285,190},{290,655,245,250},{540,645,240,255},{790,650,245,240},{1050,635,200,295},{0,975,280,270},{285,990,245,240},{540,965,240,280},{800,935,225,310},{1040,990,230,240}};
 auto* r=rects[i];float scale=source.getWidth()/1280.0f;return source.getClippedImage({(int)(r[0]*scale),(int)(r[1]*scale),(int)(r[2]*scale),(int)(r[3]*scale)});
}
inline void drawFoodGlyph(juce::Graphics& g,const Food& food,juce::Rectangle<float> bounds){
 g.setColour(juce::Colour(0xff303627));g.fillRoundedRectangle(bounds.reduced(6),12);auto c=bounds.getCentre();g.setColour(juce::Colour(0xffccd3a9));
 if(food.category=="Vegetables"||food.category=="Fresh herbs"||food.category=="Fruit for cooking"){
  juce::Path leaf;leaf.startNewSubPath(c.x-11,c.y+12);leaf.cubicTo(c.x-18,c.y-12,c.x+4,c.y-15,c.x+15,c.y-15);leaf.cubicTo(c.x+18,c.y+5,c.x+3,c.y+18,c.x-11,c.y+12);g.strokePath(leaf,juce::PathStrokeType(1.4f));g.drawLine(c.x-14,c.y+16,c.x+8,c.y-9,1.4f);
 }else{auto initials=juce::String(food.name).substring(0,1).toUpperCase();g.setFont(juce::Font(juce::FontOptions("Georgia",27.0f,juce::Font::plain)));g.drawText(initials,bounds,juce::Justification::centred);}
}
class BrowserTheme final:public juce::LookAndFeel_V4 {
public:
 BrowserTheme(){setColour(juce::ComboBox::backgroundColourId,juce::Colour(0xff25281f));setColour(juce::ComboBox::outlineColourId,juce::Colour(0xff444735));setColour(juce::ComboBox::textColourId,juce::Colour(0xfff6f1de));setColour(juce::TextEditor::backgroundColourId,juce::Colour(0xff25281f));setColour(juce::TextEditor::outlineColourId,juce::Colour(0xff444735));setColour(juce::TextEditor::textColourId,juce::Colour(0xfff6f1de));setColour(juce::TextButton::buttonColourId,juce::Colour(0xff303526));setColour(juce::TextButton::textColourOffId,juce::Colour(0xffdbf65b));setColour(juce::ScrollBar::thumbColourId,juce::Colour(0xff697449));setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff1d1f18));setColour(juce::PopupMenu::textColourId,juce::Colour(0xfff6f1de));}
 void drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool hover,bool)override{g.setColour(hover?colour.brighter(.06f):colour);g.fillRoundedRectangle(b.getLocalBounds().toFloat(),12);}
};
class FoodTile final : public juce::TextButton {
public:
 explicit FoodTile(Food f):food(std::move(f)),photo(foodImage(food)){setButtonText(juce::String(food.name));}
 void paintButton(juce::Graphics& g,bool hover,bool down) override {
  auto r=getLocalBounds().toFloat().reduced(1);g.setColour(juce::Colour(hover?0xff34372b:0xff22241f));g.fillRoundedRectangle(r,14);g.setColour(juce::Colour(down?0xffd9f35b:0xff454738));g.drawRoundedRectangle(r,14,1);
  if(photo.isValid())g.drawImage(photo,juce::Rectangle<float>(10,9,64,64),juce::RectanglePlacement::centred);else drawFoodGlyph(g,food,{10,9,64,64});
  g.setColour(juce::Colour(0xfff5f1df));g.setFont(juce::Font(juce::FontOptions(13.0f,juce::Font::bold)));g.drawFittedText(food.name,84,17,getWidth()-96,38,juce::Justification::centredLeft,2);
  g.setColour(juce::Colour(0xff919585));g.setFont(juce::Font(juce::FontOptions(10.0f)));g.drawText(food.category,84,57,getWidth()-96,17,juce::Justification::left);
 }
private:Food food;juce::Image photo;
};
}
