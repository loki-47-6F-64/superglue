import Foundation

class FileImpl : uFileInterface {
    func homeDir() -> String {
        return NSHomeDirectory();
    }
    
    func resourceBin(_ fileName: String) -> Data {
        let filePath = Bundle.main.path(forResource: fileName, ofType: nil)
        let fileURL = URL.init(string: filePath!)
        
        return try! Data(contentsOf: fileURL!)
    }
    
    func resourceTxt(_ fileName: String) -> String {
        let filePath = Bundle.main.path(forResource: fileName, ofType: nil)
        let fileURL = URL.init(string: filePath!)
        
        return try! String.init(contentsOf: fileURL!, encoding: String.Encoding.utf8) as String
    }
}
