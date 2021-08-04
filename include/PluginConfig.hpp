#pragma once

#include "main.hpp"

#include <string>
#include <unordered_map>

//#include "beatsaber-hook/shared/config/config-utils.hpp"
#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(PluginConfig,

CONFIG_VALUE(CustomScrollSpeed, float, "Custom Scroll Speed", 120);
    CONFIG_INIT_FUNCTION(
            CONFIG_INIT_VALUE(CustomScrollSpeed);
    )
)