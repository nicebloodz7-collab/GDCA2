#pragma once
#include "../core/Config.hpp"

namespace sa::game {
// Lee TODOS los ajustes de mod.json y los convierte en un Config.
// Se vuelve a leer en cada reinicio de intento: los cambios aplican al próximo intento.
sa::Config readConfig();
}
