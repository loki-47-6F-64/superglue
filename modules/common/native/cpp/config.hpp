#pragma once

#include <common_interface.hpp>

#include <log_interface.hpp>
#include <thread_interface.hpp>
#include <file_interface.hpp>

extern std::shared_ptr<gen::LogInterface> logManager;
extern std::shared_ptr<gen::ThreadInterface> threadManager;
extern std::shared_ptr<gen::FileInterface> fileManager;