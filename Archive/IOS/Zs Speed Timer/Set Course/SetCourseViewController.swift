//
//  SetCourseViewController.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 13/02/2022.
//

import UIKit
import Alamofire

class SetCourseViewController: UIViewController {
    @IBOutlet weak var courseNameTextField: UITextField!
    @IBOutlet weak var checkpointsOrderLabel: UILabel!
    var checkpointsOrderArray = [String]()
    var utils = Utils()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        //Set Course Name Text Field Stroke
        self.courseNameTextField.layer.cornerRadius = 8
        self.courseNameTextField.layer.borderWidth = 1
        self.courseNameTextField.layer.borderColor = UIColor.black.cgColor
        
        //Keyboard Handling
        let tap = UITapGestureRecognizer(target: self, action: #selector(UIInputViewController.dismissKeyboard))
        view.addGestureRecognizer(tap)
        
        NotificationCenter.default.addObserver(self, selector: #selector(checkpointPushButtonPress(_:)), name: Notification.Name("checkpointPushButtonPress"), object: nil)
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        //Update the state
        MainViewController.state = .SET_COURSE
    }
    
    //MARK: - Save Button Pressed
    @IBAction func saveButtonPressed(_ sender: Any) {
        if (self.courseNameTextField.text!.isEmpty) {
            self.utils.showAlert(message: "Please enter the course name!", view: self)
            
        } else if (self.checkpointsOrderArray.count < 2) {
            self.utils.showAlert(message: "Minimum number of checkpoints is 2.", view: self)
        
        } else {
            let saveOrderAlert = UIAlertController(title: "Save Order", message: "Are you sure you want to save this order?", preferredStyle: .alert)
            
            saveOrderAlert.addAction(UIAlertAction(title: "YES", style: .destructive, handler: { action in
                self.saveOrder()
            }))

            saveOrderAlert.addAction(UIAlertAction(title: "NO", style: .cancel, handler: nil))
            self.present(saveOrderAlert, animated: true, completion: nil)
        }
    }
    
    @objc
    func dismissKeyboard() {
        view.endEditing(true)
    }
    
    @IBAction func donePressed(_ sender: Any) {
        dismissKeyboard()
    }
    
    //MARK: - Notification Function called from Main View Controller
    @objc
    func checkpointPushButtonPress(_ notification: Notification) {
        let checkpointId: String = notification.userInfo?["id"] as! String
        
        //Run on main thread
        DispatchQueue.main.async {
            if (!self.checkpointsOrderArray.contains(checkpointId) && self.checkpointsOrderArray.count < 5) { //If already not added and maximum checkpoints are 5
                self.checkpointsOrderArray.append(checkpointId)
            }
            
            //Clear the array
            self.checkpointsOrderLabel.text = ""
            //Update the Label with the Checkpoint Ids
            for (index, element) in self.checkpointsOrderArray.enumerated() {
                self.checkpointsOrderLabel.text! += String(index + 1) + ") " + element + "\n"
            }
        }
    }
    
    //MARK: - Save Order in Preferences
    func saveOrder() {
        //Save the course name in Preferences
        Utils.Configuration.value(value: self.courseNameTextField.text!.trimString(), forKey: "c_n") //c_n = Course Name
        
        //Clear the Preferences for Checkpoints Order
        for i in 1 ... 5 {
            let prefKey: String = "c_p_" + String(i)
            Utils.Configuration.value(value: "-1", forKey: prefKey)
        }
        
        //Save the order in Preferenes
        for (index, element) in self.checkpointsOrderArray.enumerated() {
            let prefKey: String = "c_p_" + String(index + 1)
            Utils.Configuration.value(value: element, forKey: prefKey)
        }
        
        self.utils.showAlert(message: "Order Saved Successfully!", view: self)
        
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0, execute: {
            self.navigationController!.popToRootViewController(animated: true)
        })
    }
}
