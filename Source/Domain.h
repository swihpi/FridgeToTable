#pragma once
#include <juce_core/juce_core.h>
#include <string>
#include <vector>

struct Food {
    std::string name;
    std::string category;
    std::string emoji;
    double kcalPer100g;
};

struct PantryItem {
    Food food;
    double grams = 100.0;
};

struct RecipeIngredient {
    std::string name;
    double grams = 0.0;
    bool inPantry = false;
};

struct Recipe {
    std::string title = "A recipe will appear here";
    std::string description = "Choose your ingredients and appliances, then let Codex shape a meal around what you have.";
    int minutes = 35;
    int servings = 4;
    int caloriesPerServing = 0;
    std::string appliance = "Stovetop";
    std::string imagePath;
    std::vector<RecipeIngredient> ingredients;
    std::vector<std::string> steps;
};

double caloriesFor(const std::vector<RecipeIngredient>& items);
juce::String recipeAsMarkdown(const Recipe& recipe);
juce::String recipeAsPlainText(const Recipe& recipe);

