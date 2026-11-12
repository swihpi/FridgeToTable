#pragma once
#include "Domain.h"
#include <juce_events/juce_events.h>
#include <functional>
#include <thread>

class CodexEngine {
public:
    using Update = std::function<void(const juce::String&, bool, const juce::String&)>;
    static juce::String detectedExecutable();
    void setExecutable(juce::String path) { executable = std::move(path); }
    static void checkConnection(juce::String path, Update callback);
    void generate(const juce::String& prompt, Update callback);
    void generatePicture(const Recipe& recipe, Update callback);
    void cancel();
    ~CodexEngine();
private:
    void generateImpl(const juce::String& prompt, Update callback, bool pictureOnly);
    juce::String executable;
    std::atomic<bool> cancelled { false };
    std::thread worker;
};

juce::String buildRecipePrompt(const std::vector<PantryItem>& pantry,
                               const std::vector<juce::String>& appliances,
                               int servings);
juce::var recipeSchema();
bool recipeFromJSON(const juce::String& json, Recipe& recipe, juce::String& error);

