import Foundation

import Common
public class SuperGlueBluecast {
    static var loaded = false;
    
    public static func load() {
        if(!loaded) {
            SuperGlueCommon.load()
            
            loaded = true;
        }
    }
}
