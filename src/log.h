//
// Created by Michal Přikryl on 22.12.2025.
// Copyright (c) 2025 slynxcz. All rights reserved.
//
#pragma once

#include <memory>
#include <spdlog/spdlog.h>

namespace Template {
    class Log {
    public:
        static void Init();
        static void Close();

        static std::shared_ptr<spdlog::logger>& GetLogger() { return m_FP_logger; }

    private:
        static std::shared_ptr<spdlog::logger> m_FP_logger;
    };
}

#define FP_TRACE(fmt, ...)    ::Template::Log::GetLogger()->trace("- [ " fmt " ] -", ##__VA_ARGS__)
#define FP_DEBUG(fmt, ...)    ::Template::Log::GetLogger()->debug("- [ " fmt " ] -", ##__VA_ARGS__)
#define FP_INFO(fmt, ...)     ::Template::Log::GetLogger()->info("- [ " fmt " ] -", ##__VA_ARGS__)
#define FP_WARN(fmt, ...)     ::Template::Log::GetLogger()->warn("- [ " fmt " ] -", ##__VA_ARGS__)
#define FP_ERROR(fmt, ...)    ::Template::Log::GetLogger()->error("- [ " fmt " ] -", ##__VA_ARGS__)
#define FP_CRITICAL(fmt, ...) ::Template::Log::GetLogger()->critical("- [ " fmt " ] -", ##__VA_ARGS__)
