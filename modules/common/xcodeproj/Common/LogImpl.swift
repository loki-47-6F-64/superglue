import Foundation

func toString(severity : uLogSeverity) -> String {
    switch severity {
    case .debug:
        return "Debug"
    case .error:
        return "Error"
    case .info:
        return "Info"
    case .warn:
        return "Warn"
    }
}

class LogImpl : uLogInterface {
    func log(_ serverity: uLogSeverity, message: String) {
        NSLog("Superglue [/@]", message)
    }
}
