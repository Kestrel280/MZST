//
//  Run.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 13/02/2022.
//

import Foundation
import RealmSwift

class Run: Object {
    @objc dynamic var athleteName: String = ""
    @objc dynamic var courseName: String = ""
    @objc dynamic var date: String = ""
    @objc dynamic var startTime: String = ""
    @objc dynamic var cp1: String = ""
    @objc dynamic var cp2: String = ""
    @objc dynamic var cp3: String = ""
    @objc dynamic var cp4: String = ""
    @objc dynamic var cp5: String = ""
    @objc dynamic var totalRunTime: String = ""
}
