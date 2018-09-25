#pragma once

#include <generated-src/common_interface.hpp>

namespace gen {
class LogInterface;
class ThreadInterface;
class FileInterface;
}

extern std::shared_ptr<gen::LogInterface> logManager;
extern std::shared_ptr<gen::ThreadInterface> threadManager;
extern std::shared_ptr<gen::FileInterface> fileManager;