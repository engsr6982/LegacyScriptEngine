#include "PluginManager.h"

#include "Entry.h"
#include "Plugin.h"
#include "legacy/engine/EngineManager.h"
#include "legacy/engine/EngineOwnData.h"

#include <ScriptX/ScriptX.h>
#include <exception>
#include <fmt/format.h>
#include <ll/api/Logger.h>
#include <ll/api/io/FileUtils.h>
#include <ll/api/plugin/Plugin.h>
#include <ll/api/plugin/PluginManager.h>
#include <memory>
#include <stdexcept>

#ifdef LEGACY_SCRIPT_ENGINE_BACKEND_LUA

constexpr auto BaseLibFileName   = "BaseLib.lua";
constexpr auto PluginManagerName = "lse-lua";

#endif

#ifdef LEGACY_SCRIPT_ENGINE_BACKEND_QUICKJS

constexpr auto BaseLibFileName   = "BaseLib.js";
constexpr auto PluginManagerName = "lse-quickjs";

#endif

// Do not use legacy headers directly, otherwise there will be tons of errors.
void BindAPIs(script::ScriptEngine* engine);
void LLSERemoveTimeTaskData(script::ScriptEngine* engine);
auto LLSERemoveAllEventListeners(script::ScriptEngine* engine) -> bool;
auto LLSERemoveCmdRegister(script::ScriptEngine* engine) -> bool;
auto LLSERemoveCmdCallback(script::ScriptEngine* engine) -> bool;
auto LLSERemoveAllExportedFuncs(script::ScriptEngine* engine) -> bool;

namespace lse {

PluginManager::PluginManager() : ll::plugin::PluginManager(PluginManagerName) {}

auto PluginManager::load(ll::plugin::Manifest manifest) -> bool {
    auto& logger = getSelfPluginInstance().getLogger();

    logger.info("loading plugin {}", manifest.name);

    if (hasPlugin(manifest.name)) {
        throw std::runtime_error("plugin already loaded");
    }

    auto& scriptEngine = *EngineManager::newEngine(manifest.name);

    try {
        script::EngineScope engineScope(scriptEngine);

        // Set plugins's logger title
        ENGINE_OWN_DATA()->logger.title = manifest.name;
        ENGINE_OWN_DATA()->pluginName   = manifest.name;

        BindAPIs(&scriptEngine);

        auto& self = getSelfPluginInstance();

        // Load BaseLib.
        auto baseLibPath    = self.getPluginDir() / "baselib" / BaseLibFileName;
        auto baseLibContent = ll::file_utils::readFile(baseLibPath);
        if (!baseLibContent) {
            throw std::runtime_error(fmt::format("failed to read BaseLib at {}", baseLibPath.string()));
        }
        scriptEngine.eval(baseLibContent.value());

        // Load the plugin entry.
        auto pluginDir          = std::filesystem::canonical(ll::plugin::getPluginsRoot() / manifest.name);
        auto entryPath          = pluginDir / manifest.entry;
        auto pluginEntryContent = ll::file_utils::readFile(entryPath);
        if (!pluginEntryContent) {
            throw std::runtime_error(fmt::format("failed to read plugin entry at {}", entryPath.string()));
        }
        scriptEngine.eval(pluginEntryContent.value());

    } catch (const std::exception& e) {
        LLSERemoveTimeTaskData(&scriptEngine);
        LLSERemoveAllEventListeners(&scriptEngine);
        LLSERemoveCmdRegister(&scriptEngine);
        LLSERemoveCmdCallback(&scriptEngine);
        LLSERemoveAllExportedFuncs(&scriptEngine);

        scriptEngine.getData().reset();

        EngineManager::unregisterEngine(&scriptEngine);

        throw;
    }

    auto plugin = std::make_shared<Plugin>(manifest);
    if (!addPlugin(manifest.name, plugin)) {
        throw std::runtime_error(fmt::format("failed to register plugin {}", manifest.name));
    }

    return true;
}

auto PluginManager::unload(std::string_view name) -> bool {
    auto& logger = getSelfPluginInstance().getLogger();

    auto plugin = std::static_pointer_cast<Plugin>(getPlugin(name));

    logger.info("unloading plugin {}", name);

    auto& scriptEngine = *EngineManager::getEngine(std::string(name));

    LLSERemoveTimeTaskData(&scriptEngine);
    LLSERemoveAllEventListeners(&scriptEngine);
    LLSERemoveCmdRegister(&scriptEngine);
    LLSERemoveCmdCallback(&scriptEngine);
    LLSERemoveAllExportedFuncs(&scriptEngine);

    scriptEngine.getData().reset();

    EngineManager::unregisterEngine(&scriptEngine);

    scriptEngine.destroy(); // TODO: use unique_ptr to manage the engine.

    if (!erasePlugin(name)) {
        throw std::runtime_error(fmt::format("failed to unregister plugin {}", name));
    }

    return false;
}

} // namespace lse
