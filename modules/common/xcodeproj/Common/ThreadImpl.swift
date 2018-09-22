//
//  ThreadImpl.swift
//  Common
//
//  Created by Petra Farber on 21-09-18.
//  Copyright © 2018 Werkstation 4. All rights reserved.
//

import Foundation

class ThreadImpl : NSObject, uThreadInterface {
    func run(callBack : uThreadCallback) {
        callBack.run()
    }
    
    func create(_ callBack: uThreadCallback?) {
        performSelector(inBackground:#selector(run), with:callBack)
    }
}
