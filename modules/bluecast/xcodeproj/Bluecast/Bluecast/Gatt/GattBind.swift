//
//  GattBind.swift
//  Bluecast
//
//  Created by Petra Farber on 22-09-18.
//  Copyright © 2018 Loki. All rights reserved.
//

import Foundation
import CoreBluetooth

class GattBind : uBlueGatt {
    let peripheral : CBPeripheral
    let central    : CBCentralManager
    
    init(central : CBCentralManager, peripheral : CBPeripheral) {
        self.peripheral = peripheral
        self.central    = central
    }
    
    func close() {}
    
    func read(_ characteristic: uBlueGattCharacteristic?) {
        let charac = characteristic as! GattCharaceristic
        
        peripheral.readValue(for: charac.characteristic)
    }
    
    func disconnect() {
        central.cancelPeripheralConnection(peripheral)
    }
    
    func discoverServices() -> Bool {
        peripheral.discoverServices(nil)
        
        return true
    }
    
    func services() -> [uBlueGattService] {
        if(peripheral.services == nil) {
            return []
        }
        
        return peripheral.services!.map { GattService(service: $0) }
    }
}
