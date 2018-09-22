import Foundation

public class SuperGlueCommon {
    static var loaded = false
    
    public static func load() {
        if(!loaded) {
            uCommonInterface.init(LogImpl.init(), threadManager: ThreadImpl.init(), fileManager: FileImpl.init())
            
            loaded = true;
        }
    }
}
