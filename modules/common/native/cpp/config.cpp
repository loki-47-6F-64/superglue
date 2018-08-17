#include <type_traits>
#include <log_interface.hpp>
#include <log_severity.hpp>

#include <permission_interface.hpp>
#include <permission.hpp>
#include <permission_callback.hpp>

#include "config.hpp"
#include "thread_t.hpp"

std::shared_ptr<gen::LogInterface> logManager;
std::shared_ptr<gen::ThreadInterface> threadManager;
std::shared_ptr<gen::FileInterface> fileManager;

namespace gen {

void CommonInterface::init(
  const std::shared_ptr<LogInterface> &log_manager,
  const std::shared_ptr<ThreadInterface> &thread_manager,
  const std::shared_ptr<FileInterface> &file_manager
) {
  logManager = log_manager;
  threadManager = thread_manager;
  fileManager = file_manager;
  
  logManager->log(LogSeverity::DEBUG, "Registered logging object");
}
}
