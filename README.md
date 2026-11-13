# Fridge to Table

**Decide what to cook with what you already have.**

Fridge to Table is a macOS app for the moment you open the fridge and wonder what you can make. Tell it what’s in your kitchen, how much you have, which appliances you can use, and how many people are eating. Codex suggests five meals based on those constraints, with quantities and cooking instructions for each proposal.

[Download the Apple silicon DMG](https://github.com/swihpi/FridgeToTable/releases/download/v0.1.0/FridgeToTable-0.1.0-macos-arm64.dmg) · [Release notes and checksum](https://github.com/swihpi/FridgeToTable/releases/latest)

![Fridge to Table kitchen with ingredients, appliance controls and a skillet recipe suggestion](docs/screenshots/kitchen.png)

## From a fridge check to dinner

Say you have potatoes, sausage and spinach, a stovetop, and four people to feed. Enter the weights you have on hand, enable **Stovetop**, and set the serving count to four. A sausage, potato and spinach skillet is one example of what the app can suggest with those inputs.

1. **Describe your kitchen.** Search or browse the ingredient catalogue, add what you have, and enter quantities in grams. Select the appliances you can use.
2. **Pick a meal.** Generate five proposals. Use **Decline – next recipe** to see another option. The app requests a dish image for the proposal it displays.
3. **Cook or keep it for later.** Open **View full recipe** to see total ingredient weights, cooking time, estimated calories per serving, and numbered directions. Save the recipe, export it as text, or use macOS sharing to send the recipe text and dish image separately.

| Full recipe | Cooking instructions |
| --- | --- |
| ![Full recipe with generated skillet photo and ingredient weights](docs/screenshots/recipe.png) | ![Detailed cooking steps in the recipe window](docs/screenshots/instructions.png) |

## How Codex fits in

Fridge to Table starts the Codex app server on your Mac and uses your signed-in Codex session for recipe and image generation. The recipe request includes the ingredients and quantities you enter, your selected appliances, and the serving count. Generation requires internet access and is subject to your Codex account’s access and usage limits.

Saved recipes are stored on your Mac and can be reopened from **Saved recipes**. The displayed dish photo is generated, and the calorie figures are estimates.

## Install and set up

1. Download the DMG for Apple silicon, open it, and drag **FridgeToTable** to **Applications**. The preview requires macOS 26 or later and is ad hoc signed. It has no Apple notarization ticket.
2. Install Codex and sign in on your Mac. The app uses your local Codex account and requires internet access for generation.
3. Open **FridgeToTable** and select **Test and save** on the setup screen. Leave the path blank to detect Codex automatically, or enter the full path to your Codex executable. Reopen **Codex Setup** from the toolbar whenever you need to change it.

## Build from source

Install CMake 3.24+ and the Xcode command line tools. The release uses the following JUCE revision.

```sh
git clone https://github.com/juce-framework/JUCE.git external/JUCE
git -C external/JUCE checkout 72782788ce18c2d4d760b28e0921d6ffc6431102
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=26.0 -DJUCE_DIR="$PWD/external/JUCE"
cmake --build build --target FridgeToTable -j4
open build/FridgeToTable_artefacts/Release/FridgeToTable.app
```

Distributed under [AGPL-3.0](LICENSE). The pinned [JUCE source](https://github.com/juce-framework/JUCE/tree/72782788ce18c2d4d760b28e0921d6ffc6431102) is available under its AGPL option.
