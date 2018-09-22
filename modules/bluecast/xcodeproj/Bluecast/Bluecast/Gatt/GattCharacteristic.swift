//
//  GattCharacteristic.swift
//  Bluecast
//
//  Created by Petra Farber on 22-09-18.
//  Copyright © 2018 Loki. All rights reserved.
//

import Foundation
import CoreBluetooth

class GattCharaceristic : uBlueGattCharacteristic {
    let characteristic : CBCharacteristic
    
    init(characteristic : CBCharacteristic) {
        self.characteristic = characteristic
    }
    
    func descriptors() -> [uBlueGattDescriptor] {
        if(characteristic.descriptors == nil) {
            return []
        }
        
        return characteristic.descriptors!.map { GattDescriptor(descriptor: $0) }
    }
    
    func getStringValue(_ offset: Int32) -> String {
        let data = characteristic.value!
        
        return String(data: data.suffix(from: Int(offset)), encoding: String.Encoding.utf8)!
    }
    
    func getValue() -> Data {
        return characteristic.value!
    }
    
    func uuid() -> String {
        return characteristic.uuid.uuidString
    }
}
