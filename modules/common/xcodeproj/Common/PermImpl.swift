import Foundation

import CoreBluetooth
import CoreLocation

// TODO modify behaviour superglue for requesting permissions
private class LocationManagerDelegate : NSObject, CLLocationManagerDelegate {
    let callback : uPermissionCallback
    
    init(callback : uPermissionCallback) {
        self.callback = callback
    }
    
    func locationManager(_ manager: CLLocationManager, didChangeAuthorization status: CLAuthorizationStatus) {
        let granted =
            status == CLAuthorizationStatus.authorizedWhenInUse ||
            status == CLAuthorizationStatus.authorizedAlways
        
        callback.result(uPermission.coarseLocation, granted: granted)
    }
}

public class PermImpl : uPermissionInterface {
    static var locManager : CLLocationManager?
    
    public init() {}
    public func has(_ perm: uPermission) -> Bool {
        switch perm {
        case .bluetooth, .bluetoothAdmin:
            // return CBPeripheralManager.authorizationStatus() == CBPeripheralManagerAuthorizationStatus.authorized
            return true
        case .coarseLocation:
            return CLLocationManager.authorizationStatus() == CLAuthorizationStatus.authorizedWhenInUse
        }
    }
    
    public func request(_ perm: uPermission, f: uPermissionCallback?) {
        switch perm {
        case .bluetooth, .bluetoothAdmin:
            f!.result(perm, granted: false)
        case .coarseLocation:
            if(PermImpl.locManager == nil) {
                PermImpl.locManager = CLLocationManager()
            }
            PermImpl.locManager!.delegate = LocationManagerDelegate(callback: f!)
            PermImpl.locManager!.requestWhenInUseAuthorization()
        }
    }
}
