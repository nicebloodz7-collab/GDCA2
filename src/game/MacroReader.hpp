#pragma once
#include <filesystem>
#include <string>

#include "../core/Macro.hpp"

namespace sa::game {

struct MacroLoadResult {
    sa::Macro macro;
    std::string error; // vacío si todo salió bien
    bool ok() const { return error.empty() && !macro.empty(); }
};

// Lee .gdr2 / .gdr (GDR2) o un .txt simple ("frame botón jugador down" por línea).
MacroLoadResult readMacro(std::filesystem::path const& path);

}
