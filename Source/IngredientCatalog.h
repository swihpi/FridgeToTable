#pragma once
#include "Domain.h"
#include <vector>

class IngredientCatalog {
public:
    static const std::vector<Food>& all();
    static std::vector<Food> matching(const juce::String& query, const juce::String& category);
    static Food custom(const juce::String& name);
};

