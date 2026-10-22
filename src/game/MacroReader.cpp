#include "MacroReader.hpp"

#include <Geode/Geode.hpp>
#include <gdr/gdr.hpp>

#include <fstream>
#include <sstream>

using namespace geode::prelude;

namespace sa::game {

// Replay GDR2 sin extensiones propias.
struct ShowcaseReplay : gdr::Replay<ShowcaseReplay, gdr::Input<>> {
    ShowcaseReplay() : Replay("ShowcaseAssist", 1) {}
};

static MacroLoadResult readGdr(std::filesystem::path const& path) {
    MacroLoadResult out;

    char magic[3] = {};
    {
        std::ifstream f(path, std::ios::binary);
        f.read(magic, 3);
        if (!f || std::string_view(magic, 3) != "GDR") {
            out.error = "Es un GDR1 (formato viejo). Convertilo con GDR-converter a GDR2.";
            return out;
        }
    }

    auto res = ShowcaseReplay::importData(path);
    if (res.isErr()) {
        out.error = res.unwrapErr();
        return out;
    }
    auto const& replay = res.unwrap();
    if (replay.platformer) {
        out.error = "Las macros de platformer no están soportadas.";
        return out;
    }

    std::vector<sa::RawInput> raw;
    raw.reserve(replay.inputs.size());
    for (auto const& in : replay.inputs) {
        // button: 1 = Jump, 2 = Left, 3 = Right (igual que PlayerButton)
        raw.push_back({(int)in.frame, (int)in.button, !in.player2, in.down});
    }
    out.macro = sa::Macro::build(std::move(raw), replay.levelInfo.id);
    if (out.macro.empty()) out.error = "La macro no tiene clicks.";
    return out;
}

static MacroLoadResult readText(std::filesystem::path const& path) {
    MacroLoadResult out;
    std::ifstream f(path);
    if (!f.is_open()) {
        out.error = "No se pudo abrir el archivo.";
        return out;
    }
    std::vector<sa::RawInput> raw;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        int frame, button, player, down;
        if (!(ss >> frame >> button >> player >> down)) continue;
        raw.push_back({frame, button, player == 1, down != 0});
    }
    out.macro = sa::Macro::build(std::move(raw));
    if (out.macro.empty()) out.error = "La macro no tiene clicks.";
    return out;
}

MacroLoadResult readMacro(std::filesystem::path const& path) {
    auto ext = utils::string::toLower(path.extension().string());
    if (ext == ".gdr2" || ext == ".gdr") return readGdr(path);
    return readText(path);
}

} // namespace sa::game
