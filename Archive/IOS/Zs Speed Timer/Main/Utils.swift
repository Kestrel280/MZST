//
//  Utils.swift
//  Nextgen RT Hub
//
//  Created by Muhammad Hammad on 21/01/2022.
//

import Foundation
import UIKit
import MBProgressHUD

class Utils: NSObject {
    // MARK: Show Alert Function
    func showAlert(message:String, view: UIViewController) {
        let alertDisapperTimeInSeconds = 2.00
        
        let alert = UIAlertController(title: nil, message: message, preferredStyle: .actionSheet)
        view.present(alert, animated: true)
        
        DispatchQueue.main.asyncAfter(deadline: DispatchTime.now() + alertDisapperTimeInSeconds) {
          alert.dismiss(animated: true)
        }
    }
    
    // MARK: Loading HUD Functions
    func showLoadingHUD(view: UIView) {
        MBProgressHUD.showAdded(to: view, animated: true)
    }
    
    func hideLoadingHUD(view: UIView) {
        MBProgressHUD.hide(for: view, animated: true)
    }
    
    // MARK: Configuration
    public class Configuration {
        static func value<T>(defaultValue: T, forKey key: String) -> T{
            let preferences = UserDefaults.standard
            return preferences.object(forKey: key) == nil ? defaultValue : preferences.object(forKey: key) as! T
        }

        static func value(value: Any, forKey key: String){
            UserDefaults.standard.set(value, forKey: key)
        }

    }
}
