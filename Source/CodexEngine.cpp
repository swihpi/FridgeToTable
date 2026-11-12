#include "CodexEngine.h"
#include "IngredientCatalog.h"
#include <algorithm>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace {
constexpr const char* engineModel = "gpt-6-luna";
constexpr const char* engineEffort = "low";
juce::String locateCodex() {
    const char* candidates[] = {
        "/Applications/ChatGPT.app/Contents/Resources/codex-cli/CodexCLI.app/Contents/MacOS/codex",
        "/opt/homebrew/bin/codex", "/usr/local/bin/codex"
    };
    for (auto* path : candidates)
        if (access(path, X_OK) == 0) return path;
    for (const auto& directory : juce::StringArray::fromTokens(juce::String(std::getenv("PATH")), ":", "")) {
        if (!juce::File::isAbsolutePath(directory)) continue;
        auto path = juce::File(directory).getChildFile("codex");
        if (path.existsAsFile() && access(path.getFullPathName().toRawUTF8(), X_OK) == 0)
            return path.getFullPathName();
    }
    return {};
}

class AppServer {
public:
    bool start(const juce::String& overridePath) {
        auto executable = overridePath.isEmpty() ? locateCodex() : overridePath;
        if (!juce::File::isAbsolutePath(executable) || !juce::File(executable).existsAsFile()
            || access(executable.toRawUTF8(), X_OK) != 0) return false;
        int inPipe[2], outPipe[2], errPipe[2];
        if (pipe(inPipe) != 0 || pipe(outPipe) != 0 || pipe(errPipe) != 0) return false;
        posix_spawn_file_actions_t actions;
        posix_spawn_file_actions_init(&actions);
        posix_spawn_file_actions_adddup2(&actions, inPipe[0], STDIN_FILENO);
        posix_spawn_file_actions_adddup2(&actions, outPipe[1], STDOUT_FILENO);
        posix_spawn_file_actions_adddup2(&actions, errPipe[1], STDERR_FILENO);
        posix_spawn_file_actions_addclose(&actions, inPipe[1]);
        posix_spawn_file_actions_addclose(&actions, outPipe[0]);
        posix_spawn_file_actions_addclose(&actions, errPipe[0]);
        posix_spawn_file_actions_addclose(&actions, errPipe[1]);
        std::vector<std::string> args = {executable.toStdString(), "app-server", "--listen", "stdio://", "-c", "sandbox_mode=\"read-only\"", "-c", "approval_policy=\"never\""};
        std::vector<char*> argv;
        for (auto& arg : args) argv.push_back(arg.data());
        argv.push_back(nullptr);
        const int rc = posix_spawnp(&pid, argv[0], &actions, nullptr, argv.data(), environ);
        posix_spawn_file_actions_destroy(&actions);
        close(inPipe[0]); close(outPipe[1]); close(errPipe[1]);
        if (rc != 0) { close(inPipe[1]); close(outPipe[0]); close(errPipe[0]); return false; }
        input = inPipe[1]; output = outPipe[0]; errorOutput = errPipe[0];
        fcntl(output, F_SETFL, fcntl(output, F_GETFL) | O_NONBLOCK);
        fcntl(errorOutput, F_SETFL, fcntl(errorOutput, F_GETFL) | O_NONBLOCK);
        alive = true;
        return true;
    }
    ~AppServer() { stop(); }
    void stop() {
        if (input >= 0) { close(input); input = -1; }
        if (output >= 0) { close(output); output = -1; }
        if (errorOutput >= 0) { close(errorOutput); errorOutput = -1; }
        if (pid > 0) { kill(pid, SIGTERM); waitpid(pid, nullptr, 0); pid = -1; }
        alive = false;
    }
    bool send(const juce::var& value) {
        const auto message = juce::JSON::toString(value, true) + "\n";
        std::vector<char> storage((size_t)message.getNumBytesAsUTF8() + 1);
        message.copyToUTF8(storage.data(), storage.size());
        const char* bytes = storage.data();
        size_t length = (size_t)message.getNumBytesAsUTF8();
        while (length > 0) {
            auto n = write(input, bytes, length);
            if (n <= 0) return false;
            bytes += n; length -= (size_t)n;
        }
        return true;
    }
    bool readLine(juce::String& line, int timeoutMs) {
        const auto end = juce::Time::getMillisecondCounter() + (juce::uint32)timeoutMs;
        while (alive && juce::Time::getMillisecondCounter() < end) {
            auto pos = buffer.indexOf("\n");
            if (pos >= 0) { line = buffer.substring(0, pos).trim(); buffer = buffer.substring(pos + 1); return true; }
            pollfd descriptors[2] { { output, POLLIN, 0 }, { errorOutput, POLLIN, 0 } };
            ::poll(descriptors, errorOutput >= 0 ? 2 : 1, 100);
            char chunk[8192]; auto n = ::read(output, chunk, sizeof(chunk));
            if (n > 0) buffer += juce::String::fromUTF8(chunk, (int)n);
            else if (n == 0) alive = false;
            else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) alive = false;
            if (errorOutput >= 0) { char errors[4096]; auto count = ::read(errorOutput, errors, sizeof(errors)); if (count > 0) errorBuffer += juce::String::fromUTF8(errors, (int)count); }
            int status = 0;
            if (pid > 0 && waitpid(pid, &status, WNOHANG) == pid) { pid = -1; alive = false; }
        }
        return false;
    }
    juce::String diagnostic() const {
        auto value = errorBuffer.trim();
        const auto marker = value.lastIndexOf("Error:");
        if (marker >= 0) return value.substring(marker).trim();
        return value;
    }
    bool request(const juce::String& method, const juce::var& params, int id,
                 juce::var& response, int timeoutMs, const std::function<void(const juce::var&)>& event = {}) {
        auto* obj = new juce::DynamicObject(); obj->setProperty("jsonrpc", "2.0"); obj->setProperty("id", id); obj->setProperty("method", method); obj->setProperty("params", params);
        if (!send(juce::var(obj))) return false;
        const auto until = juce::Time::getMillisecondCounter() + (juce::uint32)timeoutMs;
        while (alive && juce::Time::getMillisecondCounter() < until) {
            juce::String line;
            if (!readLine(line, 500)) continue;
            auto message = juce::JSON::parse(line);
            if (auto* object = message.getDynamicObject()) {
                if (object->hasProperty("id") && (int)object->getProperty("id") == id) { response = object->getProperty("result"); return !object->hasProperty("error"); }
                if (event) event(message);
            }
        }
        return false;
    }
    bool notification(const juce::String& method) {
        auto* obj = new juce::DynamicObject(); obj->setProperty("jsonrpc", "2.0"); obj->setProperty("method", method); obj->setProperty("params", juce::var(new juce::DynamicObject()));
        return send(juce::var(obj));
    }
    bool alive = false;
private:
    pid_t pid = -1; int input = -1, output = -1, errorOutput = -1; juce::String buffer, errorBuffer;
};

juce::var object(std::initializer_list<std::pair<juce::String, juce::var>> fields) {
    auto* o = new juce::DynamicObject();
    for (const auto& f : fields) o->setProperty(f.first, f.second);
    return juce::var(o);
}
juce::var stringArray(const juce::StringArray& strings) {
    juce::Array<juce::var> a; for (const auto& s : strings) a.add(s); return juce::var(a);
}
}

static juce::var recipeObjectSchema() {
    auto str = [] { return object({{"type","string"}}); };
    auto number = [] { return object({{"type","number"}}); };
    auto boolean = [] { return object({{"type","boolean"}}); };
    auto ingredient = object({{"type","object"}, {"additionalProperties",false},
        {"properties",object({{"name",str()},{"grams",number()},{"inPantry",boolean()}})},
        {"required",stringArray({"name","grams","inPantry"})}});
    return object({{"type","object"},{"additionalProperties",false},
        {"properties",object({{"imagePath",str()},{"title",str()},{"description",str()},{"minutes",number()},{"servings",number()},{"appliance",str()},{"caloriesPerServing",number()},
            {"ingredients",object({{"type","array"},{"items",ingredient}})},
            {"steps",object({{"type","array"},{"minItems",6},{"maxItems",18},{"items",object({{"type","string"},{"minLength",180}})}})}})},
        {"required",stringArray({"imagePath","title","description","minutes","servings","appliance","caloriesPerServing","ingredients","steps"})}});
}

juce::var recipeSchema() {
    return object({{"type","object"},{"additionalProperties",false},{"properties",object({{"recipes",object({{"type","array"},{"minItems",5},{"maxItems",5},{"items",recipeObjectSchema()}})}})},{"required",stringArray({"recipes"})}});
}

juce::String buildRecipePrompt(const std::vector<PantryItem>& pantry, const std::vector<juce::String>& appliances, int servings) {
    juce::String prompt = "Create exactly FIVE distinct delicious, practical hot meal recipes using the user's kitchen inventory. Each recipe is an independent alternative, not part of one combined meal. Vary cooking technique and texture without inventing unavailable ingredients. Return all five in the recipes array. Respond ONLY with the JSON object matching the supplied schema.\n";
    prompt += "COOKING METHOD CONTRACT FOR ALL FIVE RECIPES: Teach a first-time home cook how to make the entire dish, rather than summarising a method for an experienced cook. Usually write 8-14 meaningful stages; a simple dish may use 6 fully explained stages, and never exceed 18. Split preparation, appliance setup, cooking, combining, final adjustment and serving naturally; never pad the method with repetitive stages merely to reach a count. Each steps array entry must start with a short descriptive heading, then a newline, then 3-5 complete explanatory sentences (typically 50-90 words). Do not put step numbers into the strings; the app numbers them. Every stage must contain at least 180 characters of substantive guidance. More words must add usable technique, not repetition or motivational filler. Apply this standard equally to alternatives 2-5.\n";
    prompt += "For EACH cooking stage, explicitly answer: WHAT ingredient and how much goes in now; HOW to handle it (cut size, layering, breaking up, stirring or turning technique); WHICH selected appliance and suitable ordinary pan/dish to use; WHAT heat/temperature/power and lid position; HOW LONG and how often to check; WHAT the cook should see, hear or feel when ready; WHAT to do next. Include only the answers relevant to that stage. For preparation stages explain the physical technique, approximate size and where to set prepared food aside. Name the actual ingredients instead of vague references such as add everything. If an ingredient is divided between stages, state each quantity and ensure the parts add up to its recipe total.\n";
    prompt += "Never leave a stage as 'brown the meat', 'cook the vegetables', 'make the sauce', 'cook until done', 'season to taste' or 'serve'. Explain how to accomplish it. Example of the required depth and format (adapt to actual inventory): 'Brown the mince gently\\nPlace the mince in a wide frying pan over medium heat and spread it into a thin layer. Leave it undisturbed for about 2 minutes, then use a spatula to break it into roughly 1 cm pieces. Continue cooking for 6-8 minutes, turning the pieces every minute, until the surface is browned and any released liquid has mostly evaporated. If the pan starts to catch, reduce the heat and add 15 ml water, scraping the base gently; do not add oil unless it is in the inventory.' This example is technique guidance, not an instruction to add mince to every recipe. Browning is not a substitute for a final safe-doneness check.\n";
    prompt += "Start with mise en place: specify washing, peeling if needed, chopping sizes, draining, thawing if relevant, and separating ingredients used at different stages. Describe suitable pan or dish size, when to preheat, whether to cover with a lid, how frequently to stir or turn, and how to avoid crowding. Give measured water additions in millilitres. Explain overlapping tasks so the meal finishes together. Include relevant doneness checks and food-safe internal temperatures where appropriate, resting or draining, final texture adjustments using only available ingredients or water, and how to divide and serve the requested portions. Explain useful reasons briefly, such as reserving pasta water to loosen the sauce.\n";
    prompt += "Be concrete about appliance-specific settings: oven temperature in Celsius and fan/conventional distinction, rack position and preheating; air-fryer temperature, preheating if needed, single-layer loading, batch size and when to shake/turn; microwave wattage, suitable microwave-safe container, covering/venting, stirring and standing time; or stovetop low/medium/high heat and pan preheating. Only use appliances selected. Selected appliances are available options, not a requirement to use every one. Ordinary utensils, pans, pots and compatible dishes may be assumed; do not assume an extra powered appliance. If using multiple appliances, explain precisely when to start each component, what to do while it cooks, and how to keep components ready without drying them out.\n";
    prompt += "Include relevant practical corrections for sticking, drying out, excess liquid or uneven browning at the point they may occur. State safe-doneness checks appropriate to the actual ingredients, without treating colour alone as proof of safety. Finish with a purposeful final stage explaining consistency adjustment using only available ingredients or measured water, any rest, how to combine components and divide into the requested portions. Never invent oil, salt, seasoning, garnishes or missing ingredients to make the method sound complete. Total recipe minutes must include preparation, preheating, cooking and resting, accounting for tasks done in parallel. CHRONOLOGY: Each stage must follow the actual cooking timeline. Put overlapping or keeping-warm instructions alongside the component cooking stage where needed. Perform all doneness checks, tasting, texture corrections and any resting BEFORE plating. The LAST stage is the actual division into portions and serving; do not add retrospective cooking, reheating, keeping-warm, troubleshooting or hygiene stages after food has been served. Before returning, silently check all five methods for vague steps, missing quantities, appliance settings, sequencing and unexplained techniques, and expand any weak stage.\n";
    prompt += "Set imagePath to an empty string for now. The application will create the dish photographs in the next phase.\n";
    prompt += "Hard constraints: serve exactly " + juce::String(servings) + " people; use only the appliances listed; use no more than the available amount of any listed ingredient; ingredient amounts must be total recipe grams. If an item cannot be used, omit it. Do not add ANY ingredients not listed, including oils, seasonings, or garnishes. Water is the only implicit ingredient; describe it in steps rather than ingredient entries. Set inPantry=true for every ingredient. If amounts are small, describe the resulting modest portions honestly. Give realistic, ordered cooking steps and an honest approximate kcal per serving. Use English, metric grams, common Romanian supermarket ingredients, and food-safe temperatures. Do not claim exact nutrition.\n\nAvailable food (ingredient: grams available):\n";
    for (const auto& item : pantry) prompt += "- " + juce::String(item.food.name) + ": " + juce::String((int)std::round(item.grams)) + " g\n";
    prompt += "\nAvailable appliances: ";
    for (size_t i = 0; i < appliances.size(); ++i) prompt += (i ? ", " : "") + appliances[i];
    prompt += "\n\nUse ingredient names that match the inventory/catalog where possible. Prefer using up quantities sensibly. The application independently estimates calories from ingredient weights and its nutrition table; your kcal value is a secondary estimate.";
    return prompt;
}

bool recipeFromJSON(const juce::String& json, Recipe& recipe, juce::String& error) {
    auto parsed = juce::JSON::parse(json);
    auto* o = parsed.getDynamicObject();
    if (!o) { error = "Codex returned a response that was not valid recipe JSON."; return false; }
    recipe.imagePath=o->getProperty("imagePath").toString().toStdString();
    recipe.title = o->getProperty("title").toString().toStdString();
    recipe.description = o->getProperty("description").toString().toStdString();
    recipe.minutes = juce::jlimit(5, 300, (int)o->getProperty("minutes"));
    recipe.servings = juce::jlimit(1, 16, (int)o->getProperty("servings"));
    recipe.appliance = o->getProperty("appliance").toString().toStdString();
    recipe.caloriesPerServing = juce::jlimit(0, 3000, (int)o->getProperty("caloriesPerServing"));
    recipe.ingredients.clear(); recipe.steps.clear();
    auto* ingredientArray=o->getProperty("ingredients").getArray(); auto* stepArray=o->getProperty("steps").getArray();
    if(!ingredientArray || !stepArray) { error="Recipe is missing its ingredient list or method."; return false; }
    for (auto item : *ingredientArray) {
        auto* ingredient = item.getDynamicObject(); if (!ingredient) continue;
        recipe.ingredients.push_back({ingredient->getProperty("name").toString().toStdString(),
            juce::jmax(0.0, (double)ingredient->getProperty("grams")), (bool)ingredient->getProperty("inPantry")});
    }
    for (auto step : *stepArray) recipe.steps.push_back(step.toString().toStdString());
    for(const auto& ingredient:recipe.ingredients) if(!std::isfinite(ingredient.grams) || ingredient.grams<=0) {error="Recipe contains an invalid ingredient weight.";return false;}
    if (recipe.title.empty() || recipe.steps.empty() || recipe.ingredients.empty()) { error = "The generated recipe was missing its title or cooking method."; return false; }
    return true;
}

juce::String CodexEngine::detectedExecutable() { return locateCodex(); }

void CodexEngine::checkConnection(juce::String path, Update callback) {
    std::thread([executablePath = std::move(path), onComplete = std::move(callback)] {
        juce::String error;
        {
            AppServer server;
            juce::var response;
            auto init = object({{"clientInfo", object({{"name", "fridgetotable"}, {"version", "0.2.0"}})}});
            if (!server.start(executablePath))
                error = "Choose a runnable Codex executable, or install Codex and try again.";
            else if (!server.request("initialize", init, 1, response, 15000) || !server.notification("initialized"))
                error = "Codex started but did not initialize. Check that your Codex installation is up to date.";
            else if (!server.request("account/read", object({{"refreshToken", false}}), 2, response, 15000))
                error = "Could not check the Codex account. Open Codex and check your sign-in.";
            else if (response.getProperty("account", {}).isVoid() && (bool) response.getProperty("requiresOpenaiAuth", true))
                error = "Sign in to Codex on this Mac, then test the connection again.";
            else if (!server.request("thread/start", object({{"sandbox", "read-only"}, {"approvalPolicy", "never"}, {"model", engineModel}}), 3, response, 20000))
                error = "Codex could not start a recipe session with the configured model.";
        }
        juce::MessageManager::callAsync([onComplete, error] { if (onComplete) onComplete({}, true, error); });
    }).detach();
}

void CodexEngine::generate(const juce::String& prompt, Update callback) {generateImpl(prompt,std::move(callback),false);}
void CodexEngine::generatePicture(const Recipe& recipe, Update callback) {
    auto prompt=juce::String("Use native image generation to create exactly ONE separate photorealistic food photograph of this finished recipe. Match the ingredients and cooking method. Small landscape app preview, approximately 768 x 512 or nearest supported small size; standard/low quality. No Full HD, 1080p, 2K, 4K, collage, text or branding. Dark ceramic plate, charcoal kitchen, soft natural side lighting. Do not use shell, SVG or external image URLs. Generate one picture only.\n")+recipeAsMarkdown(recipe);
    generateImpl(prompt,std::move(callback),true);
}
void CodexEngine::generateImpl(const juce::String& prompt, Update callback, bool pictureOnly) {
    cancel(); if (worker.joinable()) worker.join(); cancelled = false;
    worker = std::thread([this, prompt, callback, pictureOnly, executablePath = executable] {
        auto emit = [callback](const juce::String& status, bool done, const juce::String& error = {}) {
            juce::MessageManager::callAsync([callback, status, done, error] { if (callback) callback(status, done, error); });
        };
        AppServer server;
        if (!server.start(executablePath)) { emit({}, true, "Codex CLI could not be started. Install or sign in to Codex, then try again."); return; }
        emit("Connecting to Codex...", false);
        juce::var response;
        auto init = object({{"clientInfo",object({{"name","fridgetotable"},{"title","Fridge to Table"},{"version","0.2.0"}})},{"capabilities",object({{"experimentalApi",true}})}});
        if (!server.request("initialize", init, 1, response, 15000) || !server.notification("initialized")) {
            const auto detail = server.diagnostic();
            emit({}, true, detail.isNotEmpty() ? "Codex app-server could not initialize: " + detail : "Codex app-server could not initialize. Check that Codex is signed in and up to date."); return;
        }
        emit("Preparing your recipe...", false);
        auto threadParams = object({{"sandbox","read-only"},{"approvalPolicy","never"},{"model",engineModel}});
        if (!server.request("thread/start", threadParams, 2, response, 20000)) {
            emit({}, true, "Codex couldn't start a recipe session. Check your account and try again."); return;
        }
        auto* threadObj = response.getDynamicObject();
        juce::String threadId;
        if (threadObj) if (auto* thread = threadObj->getProperty("thread").getDynamicObject()) threadId = thread->getProperty("id").toString();
        if (threadId.isEmpty()) { emit({}, true, "Codex did not return a session ID."); return; }
        if(pictureOnly) {
            juce::Array<juce::var> input;input.add(object({{"type","text"},{"text",prompt}}));
            server.send(object({{"id",3},{"method","turn/start"},{"params",object({{"threadId",threadId},{"effort",engineEffort},{"input",juce::var(input)}})}}));
            emit("Creating this recipe's dish picture...",false);
            juce::String path,line;bool done=false,failed=false;auto deadline=juce::Time::getMillisecondCounter()+300000;
            while(!cancelled && server.alive && !done && juce::Time::getMillisecondCounter()<deadline){
                if(!server.readLine(line,400))continue;auto event=juce::JSON::parse(line);auto method=event.getProperty("method",{}).toString();auto params=event.getProperty("params",{});
                if(event.hasProperty("error")){failed=true;break;}
                if(method=="item/completed"){auto item=params.getProperty("item",{});if(item.getProperty("type",{}).toString()=="imageGeneration")path=item.getProperty("savedPath",{}).toString();}
                if(method=="turn/completed"){done=true;failed=params.getProperty("turn",{}).getProperty("status",{}).toString()!="completed";}
            }
            server.stop();if(cancelled)return;
            if(failed||!done||path.isEmpty())emit({},true,"Codex could not create this dish picture. You can move to the next proposal.");else emit(path,true);
            return;
        }
        auto* textInput = new juce::DynamicObject(); textInput->setProperty("type","text"); textInput->setProperty("text",prompt);
        juce::Array<juce::var> input; input.add(juce::var(textInput));
        auto turnParams = object({{"threadId",threadId},{"input",juce::var(input)},{"outputSchema",recipeSchema()},{"effort",engineEffort},
            {"approvalPolicy","never"},{"sandboxPolicy",object({{"type","readOnly"},{"networkAccess",false}})}});
        emit("Building a meal from your pantry...", false);
        if (!server.send(object({{"jsonrpc","2.0"},{"id",3},{"method","turn/start"},{"params",turnParams}}))) {
            emit({}, true, "Could not send the recipe request to Codex."); return;
        }
        juce::String finalText, line;
        bool started = false, completed = false, failed = false;
        auto deadline = juce::Time::getMillisecondCounter() + 900000;
        while (!cancelled && juce::Time::getMillisecondCounter() < deadline && server.alive && !completed) {
            if (!server.readLine(line, 400)) continue;
            auto msg = juce::JSON::parse(line); auto* m = msg.getDynamicObject(); if (!m) continue;
            const auto method = m->getProperty("method").toString(); auto params = m->getProperty("params"); auto* p = params.getDynamicObject();
            if (m->hasProperty("id") && (int)m->getProperty("id") == 3) {
                if (m->hasProperty("error")) { failed = true; completed = true; }
                else started = true;
            }
            if ((method == "item/started" || method == "item/completed") && p) {
                auto* item=p->getProperty("item").getDynamicObject();
                if(item && item->getProperty("type").toString()=="imageGeneration") emit(method=="item/started" ? "Creating your AI dish photographs..." : "Dish photograph created. Preparing the next choice...",false);
            }
            if (method == "item/agentMessage/delta" && p) {
                auto delta = p->getProperty("delta").toString(); finalText += delta;
                if (!finalText.isEmpty()) emit("Codex is shaping the method...", false);
            }
            if (method == "turn/completed" && p) {
                auto turn = p->getProperty("turn"); auto* t = turn.getDynamicObject();
                completed = true;
                if (t && t->getProperty("status").toString() != "completed") failed = true;
            }
            juce::ignoreUnused(started);
        }
        if (cancelled) return;
        if (!completed || failed) { emit({}, true, "Codex couldn't complete this recipe collection. Please retry."); return; }
        auto first=finalText.indexOf("{");auto last=finalText.lastIndexOf("}");if(first>=0&&last>first)finalText=finalText.substring(first,last+1);
        auto batch=juce::JSON::parse(finalText);auto* recipes=batch.getProperty("recipes",juce::var()).getArray();
        if(!recipes || recipes->size()!=5){emit({},true,"Codex did not return five recipe alternatives.");return;}
        server.stop();
        emit(juce::JSON::toString(batch,true),true);
    });
}

void CodexEngine::cancel() { cancelled = true; }
CodexEngine::~CodexEngine() { cancel(); if (worker.joinable()) worker.join(); }

