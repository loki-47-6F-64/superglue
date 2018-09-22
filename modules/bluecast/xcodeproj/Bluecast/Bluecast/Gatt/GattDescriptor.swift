//
//  GattDescriptor.swift
//  Bluecast
//
//  Created by Petra Farber on 22-09-18.
//  Copyright © 2018 Loki. All rights reserved.
//

import Foundation
import CoreBluetooth

class GattDescriptor : uBlueGattDescriptor {
    let descriptor : CBDescriptor
    
    init(descriptor : CBDescriptor) {
        self.descriptor = descriptor
    }
    
    func uuid() -> String {
        return descriptor.uuid.uuidString
    }
}
