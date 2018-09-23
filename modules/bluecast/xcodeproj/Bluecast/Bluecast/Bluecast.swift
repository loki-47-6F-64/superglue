import Foundation

import Common
public class SuperGlueBluecast {
    static var loaded = false;
    
    public static var bluetooth : Bluetooth?
    public static func load() {
        if(!loaded) {
            SuperGlueCommon.load()
            
            bluetooth = Bluetooth()
            loaded = true;
        }
    }
}
