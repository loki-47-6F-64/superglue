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

class Bluetooth : NSObject, uBlueController, CBCentralManagerDelegate, CBPeripheralDelegate, CLLocationManagerDelegate {
    var centralManager : CBCentralManager?
    let locationManager = CLLocationManager()
    
    var blueCallback : uBlueCallback?
    var blueView : uBlueViewCallback?
    
    // Only works for single connectections
    //TODO: Use dictionary
    var serviceCount = 0
    public override init() {
        super.init()
        
        self.centralManager = CBCentralManager.init(delegate: self, queue: nil)
        
        locationManager.delegate = self
        locationManager.startRangingBeacons(in: CLBeaconRegion())
    }
    
    func centralManagerDidUpdateState(_ central: CBCentralManager) {
        blueView.map { $0.onPowerStateChange(Bluetooth.fromState(newState: central.state)) }
    }
    
    func centralManager(_ central: CBCentralManager, didDiscover peripheral: CBPeripheral, advertisementData: [String : Any], rssi RSSI: NSNumber) {
        let name = peripheral.name
        let addr = peripheral.identifier.uuidString
        
        blueCallback?.onScanResult(
            uBlueScanResult.init(
                dev: uBlueDevice.init(name: name, address: addr),
                advertisingSid: nil,
                txPower: nil,
                rssi: Int32(truncating:RSSI),
                dataComplete: true,
                connectable: true))
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverServices error: Error?) {
        serviceCount = peripheral.services!.count
        
        for service in peripheral.services! {
            peripheral.discoverCharacteristics(nil, for: service)
        }
    }
    
    func peripheral(_ peripheral: CBPeripheral, didDiscoverCharacteristicsFor service: CBService, error: Error?) {
        serviceCount -= 1
        if(serviceCount == 0) {
            blueCallback?.onGattServicesDiscovered(GattBind.init(central: centralManager!, peripheral: peripheral), result: true)
        }
    }
    
    func centralManager(_ central: CBCentralManager, didConnect peripheral: CBPeripheral) {
        blueCallback?.onGattConnectionStateChange(
            GattBind.init(central: central, peripheral: peripheral),
            newState: uBlueGattConnectionState.connected)
    }
    
    func centralManager(_ central: CBCentralManager, didDisconnectPeripheral peripheral: CBPeripheral, error: Error?) {
        blueCallback?.onGattConnectionStateChange(
            GattBind.init(central: central, peripheral: peripheral),
            newState: uBlueGattConnectionState.disconnected)
    }
    
    func connectGatt(_ dev: uBlueDevice) {
        let uuid = UUID.init(uuidString: dev.address)!
        
        let peripheral = (centralManager?.retrievePeripherals(withIdentifiers: [uuid])[0])!
        
        centralManager?.connect(peripheral, options: nil)
    }
    
    func scan(_ enable: Bool) {
        if(enable) {
            centralManager?.scanForPeripherals(withServices: nil, options: nil)
        }
        else {
            centralManager?.stopScan()
        }
    }
    
    func isEnabled() -> Bool {
        return centralManager?.state == CBManagerState.poweredOn
    }
    
    func locationManager(_ manager: CLLocationManager, didRangeBeacons beacons: [CLBeacon], in region: CLBeaconRegion) {
        for beacon in beacons {
            blueCallback?.onBeaconUpdate(
                uBlueBeacon(device: uBlueDevice(name: nil, address: "dummy-addr"),
                uuid: beacon.proximityUUID.uuidString,
                major: Int32(truncating: beacon.major),
                minor: Int32(truncating: beacon.minor),
                distance: beacon.accuracy.advanced(by: 0.0)))
        }
    }
    
    static func fromState(newState : CBManagerState) -> uBluePowerState {
        switch newState {
        case .poweredOff, .unauthorized, .unsupported, .unknown:
            return uBluePowerState.off
        case .poweredOn:
            return uBluePowerState.on
        case .resetting:
            return uBluePowerState.turningOn
        }
    }
}
