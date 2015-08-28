#include "config.hpp"
#include "thread_t.hpp"
#include <log_interface.hpp>

std::shared_ptr<gen::LogInterface> logManager;
std::shared_ptr<gen::ThreadInterface> threadManager;
std::shared_ptr<gen::FileInterface> fileManager;

namespace gen {

void CommonInterface::config(
  const std::shared_ptr<LogInterface> &log_obj,
  const std::shared_ptr<ThreadInterface> &thread_obj,
  const std::shared_ptr<FileInterface> &file_manager
) {
  logManager = log_obj;
  threadManager = thread_obj;
  fileManager = file_manager;
  
  logManager->log(log_severity::DEBUG, "Registered logging object");
}
}