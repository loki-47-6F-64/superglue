//
//  uBlueController.swift
//  Bluecast
//
//  Created by Petra Farber on 21-09-18.
//  Copyright © 2018 Loki. All rights reserved.
//

import Foundation
import CoreBluetooth
import CoreLocation

public class Bluetooth : NSObject, uBlueController, CBCentralManagerDelegate, CBPeripheralDelegate, CLLocationManagerDelegate {
    
    var centralManager : CBCentralManager?
    
    let locationManager = CLLocationManager()
    let region = CLBeaconRegion(proximityUUID: UUID.init(uuidString: "ffebb31d-ae63-4c4f-bc76-4658073cf483")!, identifier: "uniqueID")
    
    public var blueCallback : uBlueCallback?
    
    // Only works for single connectections
    //TODO: Use dictionary
    var serviceCount : [UUID : Int] = [:]
    var peripherals : [CBPeripheral] = []
    
    public override init() {
        super.init()
        
        self.centralManager = CBCentralManager.init(delegate: self, queue: nil)
        
        blueCallback = uBlueCastInterface.config(self)
        locationManager.delegate = self
    }
    
    public func centralManagerDidUpdateState(_ central: CBCentralManager) {
        blueCallback?.onBluePowerStateChange(Bluetooth.fromState(newState: central.state))
    }
    
    public func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        let name = peripheral.name
        let addr = peripheral.identifier.uuidString
        
        blueCallback?.onScanResult(
            uBlueScanResult.init(
                dev: uBlueDevice.init(name: name, address: addr),
                rssi: Int32(truncating:RSSI))
        )
    }
    
    public func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        serviceCount[peripheral.identifier] = peripheral.services!.count
        
        for service in peripheral.services! {
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }
    
    public func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        serviceCount[peripheral.identifier]! -= 1
        if(serviceCount[peripheral.identifier] == 0) {
            blueCallback?.onGattServicesDiscovered(GattBind.init(central: centralManager!, peripheral: peripheral), result: true)
        }
    }
  
  public func peripheral(_ peripheral: CBPeripheral, didUpdateValueFor characteristic: CBCharacteristic, error: Error?) {
    blueCallback?.onCharacteristicRead(
      GattBind(central: centralManager!,peripheral: peripheral),
      characteristic: GattCharaceristic(characteristic: characteristic),
      result: true)
  }
    
    public func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        blueCallback?.onGattConnectionStateChange(
            GattBind.init(central: central, peripheral: peripheral),
            newState: uBlueGattConnectionState.connected)
    }
    
    public func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        DispatchQueue.main.async {
            // There is at leas one peripheral connected
            for i in 0 ... self.peripherals.count - 1 {
                if(self.peripherals[i] == peripheral) {
                    self.peripherals.remove(at: i)
                    
                    break
                }
            }
        }
        
        blueCallback?.onGattConnectionStateChange(
            GattBind.init(central: central, peripheral: peripheral),
            newState: uBlueGattConnectionState.disconnected)
    }
    
    public func connectGatt(_ dev: uBlueDevice) {
        let uuid = UUID.init(uuidString: dev.address)!
        
        let peripheral = (centralManager?.retrievePeripherals(withIdentifiers: [uuid])[0])!
        
        DispatchQueue.main.async {
            self.peripherals.append(peripheral);
        }
        
        peripheral.delegate = self
        centralManager?.connect(peripheral, options: nil)
    }
    
    public func beaconScan(_ enable: Bool) {
        if(enable) {
            locationManager.startRangingBeacons(in: region)
        }
        else {
            locationManager.stopRangingBeacons(in: region)
        }
    }
    
    public func peripheralScan(_ enable: Bool) {
        if(enable) {
            centralManager?.scanForPeripherals(withServices: nil, options: nil)
        }
        else {
            centralManager?.stopScan()
        }
    }
    
    public func isEnabled() -> Bool {
        return centralManager?.state == CBManagerState.poweredOn
    }
    
    public func locationManager(_ manager: CLLocationManager, didRangeBeacons beacons: [CLBeacon], in region: CLBeaconRegion) {
        for beacon in beacons {
            blueCallback?.onBeaconUpdate(uBlueBeacon(
                uuid: beacon.proximityUUID.uuidString,
                major: Int32(truncating: beacon.major),
                minor: Int32(truncating: beacon.minor),
                distance: beacon.accuracy.advanced(by: 0.0))
            )
        }
    }
    
    private static func fromState(newState : CBManagerState) -> uBluePowerState {
        switch newState {
        case .poweredOff, .unauthorized, .unsupported, .unknown, .resetting:
            return uBluePowerState.off
        case .poweredOn:
            return uBluePowerState.on
        }
    }
}
