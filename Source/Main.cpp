#include "KitchenWindow.h"

class FridgeToTableApplication final : public juce::JUCEApplication {
public:
    const juce::String getApplicationName() override { return "Fridge to Table"; }
    const juce::String getApplicationVersion() override { return "0.1.0"; }
    bool moreThanOneInstanceAllowed() override { return false; }
    void initialise(const juce::String&) override {
        juce::LookAndFeel::setDefaultLookAndFeel(&lookAndFeel);
        mainWindow = std::make_unique<KitchenWindow>();
    }
    void shutdown() override { mainWindow.reset(); juce::LookAndFeel::setDefaultLookAndFeel(nullptr); }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(const juce::String&) override {}
private:
    juce::LookAndFeel_V4 lookAndFeel;
    std::unique_ptr<KitchenWindow> mainWindow;
};

START_JUCE_APPLICATION(FridgeToTableApplication)

