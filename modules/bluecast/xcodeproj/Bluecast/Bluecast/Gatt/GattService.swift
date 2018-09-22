//
//  GattService.swift
//  Bluecast
//
//  Created by Petra Farber on 22-09-18.
//  Copyright © 2018 Loki. All rights reserved.
//

import Foundation
import CoreBluetooth

class GattService : uBlueGattService {
    let service : CBService
    
    init(service : CBService) {
        self.service = service
    }
    
    func characteristics() -> [uBlueGattCharacteristic] {
        if(service.characteristics == nil) {
            return []
        }
        
        return service.characteristics!.map { GattCharaceristic(characteristic: $0) }
    }
    
    func uuid() -> String {
        return service.uuid.uuidString
    }
}
