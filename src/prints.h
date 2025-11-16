//
// Created by Michal Přikryl on 10.07.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once
#include <string>
#include <vector>
#include "schema/CCSPlayerController.h"

namespace TemplatePlugin::Prints
{
    enum class HudDestination : int
    {
        Center = 1,
        Alert = 2,
        Chat = 3
    };

    void ClientPrintNative(CPlayerSlot slot, HudDestination dest, const char* msg);

    std::string ReplaceColorTags(const std::string& input);

    std::string ReplaceFormatTags(const std::string& input);

    template <typename... Args>
    std::string FormatMessage(const std::string& input, Args&&... args)
    {
        std::string formatted = input;

        std::array<std::string, sizeof...(Args)> replacements = {(std::to_string(std::forward<Args>(args)))...};
        size_t index = 0;

        for (const auto& value : replacements)
        {
            size_t pos = formatted.find("%i");
            if (pos == std::string::npos)
                pos = formatted.find("%s");

            if (pos != std::string::npos)
            {
                formatted.replace(pos, 2, value);
                index++;
            }
        }

        formatted = ReplaceFormatTags(formatted);
        formatted = ReplaceColorTags(formatted);

        return formatted;
    }

    void ChatToPlayer(CCSPlayerController* player, const std::string& msg);
}
