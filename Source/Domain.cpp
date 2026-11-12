#include "Domain.h"
#include "IngredientCatalog.h"
#include <sstream>
#include <iomanip>

juce::String recipeAsPlainText(const Recipe& recipe) {
    juce::String out=juce::String(recipe.title).trim()+"\n";
    out += "\n";
    if(!recipe.description.empty())out += juce::String(recipe.description).trim()+"\n\n";
    out += "MEAL DETAILS\nNumber of people: "+juce::String(recipe.servings)+"\nCooking time: "+juce::String(recipe.minutes)+" minutes\n";
    out += "Appliances: "+juce::String(recipe.appliance)+"\nEstimated calories per person: "+juce::String(recipe.caloriesPerServing)+" kcal\n\n";
    out += "INGREDIENTS & WEIGHTS\nTotal quantities for "+juce::String(recipe.servings)+" people\n\n";
    for(const auto& item:recipe.ingredients){
        auto grams=juce::String(item.grams,1);if(grams.endsWith(".0"))grams=grams.dropLastCharacters(2);
        out += "- "+juce::String(item.name)+": "+grams+" g\n";
    }
    out += "\nCOOKING INSTRUCTIONS\n\n";
    for(size_t i=0;i<recipe.steps.size();++i)out += juce::String((int)i+1)+". "+juce::String(recipe.steps[i]).trim()+"\n\n";
    out += "Created by FridgeToTable App - made by Stefan Willems\n";
    return out;
}

double caloriesFor(const std::vector<RecipeIngredient>& items) {
    double total = 0.0;
    for (const auto& item : items) {
        for (const auto& food : IngredientCatalog::all()) {
            if (juce::String(food.name).equalsIgnoreCase(juce::String(item.name))) {
                total += food.kcalPer100g * item.grams / 100.0;
                break;
            }
        }
    }
    return total;
}

juce::String recipeAsMarkdown(const Recipe& recipe) {
    juce::String out = "# " + juce::String(recipe.title) + "\n\n" + juce::String(recipe.description) + "\n\n";
    out += "**" + juce::String(recipe.minutes) + " min | " + juce::String(recipe.servings) + " servings | ~" + juce::String(recipe.caloriesPerServing) + " kcal / person**\n\n";
    out += "## Ingredients\n";
    for (const auto& item : recipe.ingredients)
        out += "- " + juce::String(item.name) + " - " + juce::String((int) std::round(item.grams)) + " g" + (item.inPantry ? "" : " *(to get)*") + "\n";
    out += "\n## Method\n";
    for (size_t i = 0; i < recipe.steps.size(); ++i) {
        auto step=juce::String(recipe.steps[i]);int headingEnd=step.indexOfChar('\n');
        out += juce::String((int) i + 1) + ". ";
        if(headingEnd>0)out += "**"+step.substring(0,headingEnd)+"**\n   "+step.substring(headingEnd+1).replace("\n","\n   ");
        else out += step;
        out += "\n\n";
    }
    out += "\n*Estimated nutrition. Powered by Codex.*\n";
    return out;
}

