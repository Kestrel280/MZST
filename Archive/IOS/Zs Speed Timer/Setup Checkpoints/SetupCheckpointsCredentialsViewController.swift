//
//  SetupCheckpointsCredentialsViewController.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 11/02/2022.
//

import UIKit
import Alamofire

class SetupCheckpointsCredentialsViewController: UIViewController {
    @IBOutlet weak var ssidTextField: UITextField!
    @IBOutlet weak var passwordTextField: UITextField!
    @IBOutlet weak var showPasswordButton: UIButton!
    @IBOutlet weak var availableNetworksButton: UIButton!
    
    var passwordShown: Bool = false
    let utils = Utils()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        //Hide Password
        passwordTextField.isSecureTextEntry = true
        
        //Set SSID Text Field Stroke
        self.ssidTextField.layer.cornerRadius = 8
        self.ssidTextField.layer.borderWidth = 1
        self.ssidTextField.layer.borderColor = UIColor.black.cgColor
        
        //Set Password Text Field Stroke
        self.passwordTextField.layer.cornerRadius = 8
        self.passwordTextField.layer.borderWidth = 1
        self.passwordTextField.layer.borderColor = UIColor.black.cgColor
        
        //Keyboard Handling
        let tap = UITapGestureRecognizer(target: self, action: #selector(UIInputViewController.dismissKeyboard))
        view.addGestureRecognizer(tap)
        
        NotificationCenter.default.addObserver(self, selector: #selector(keyboardWillShow), name: UIResponder.keyboardWillShowNotification, object: nil)
        NotificationCenter.default.addObserver(self, selector: #selector(keyboardWillHide), name: UIResponder.keyboardWillHideNotification, object: nil)

        
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.0, execute: {
            //Send a request to 192.168.4.1 after 1 second
            self.utils.showLoadingHUD(view: self.view)
            self.sendRequestToCheckpoint()
        })
    }
    
    @objc
    func dismissKeyboard() {
        view.endEditing(true)
    }
    
    @objc func keyboardWillShow(notification: NSNotification) {
        if let keyboardSize = (notification.userInfo?[UIResponder.keyboardFrameEndUserInfoKey] as? NSValue)?.cgRectValue {
            if self.view.frame.origin.y == 0 {
                self.view.frame.origin.y -= keyboardSize.height
            }
        }
    }

    @objc func keyboardWillHide(notification: NSNotification) {
        if self.view.frame.origin.y != 0 {
            self.view.frame.origin.y = 0
        }
    }
    
    //MARK: - Show Password Button Pressed
    @IBAction func showPasswordPressed(_ sender: Any) {
        if (passwordShown) {
            passwordShown = false
            showPasswordButton.setBackgroundImage(UIImage(systemName: "circle"), for: UIControl.State.normal)
            //Hide Password
            passwordTextField.isSecureTextEntry = true
            
        } else {
            passwordShown = true
            showPasswordButton.setBackgroundImage(UIImage(systemName: "checkmark.circle.fill"), for: UIControl.State.normal)
            //Show Password
            passwordTextField.isSecureTextEntry = false
        }
    }
    
    @IBAction func ssidNextPressed(_ sender: Any) {
        self.passwordTextField.becomeFirstResponder()
    }
    
    @IBAction func passwordDonePressed(_ sender: Any) {
        self.dismissKeyboard()
    }
    
    //MARK: - Save Credentials Button Pressed
    @IBAction func saveCredentialsPressed(_ sender: Any) {
        if (ssidTextField.text?.trimString() != "" && passwordTextField.text?.trimString() != "") {
            self.utils.showLoadingHUD(view: self.view)
            sendCredentialsToCheckpoint(ssid: ssidTextField.text!.trimString(), password: passwordTextField.text!.trimString())
            
        } else {
            utils.showAlert(message: "Please enter the credentials and try again!", view: self)
        }
    }
    
    //MARK: - Send a http request to 192.168.4.1 (Esp Server) to get SSIDS.
    func sendRequestToCheckpoint() {
        var URL: String = "http://192.168.4.1"
        URL = URL.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed)!
        
        AF.request(URL, method: .get, encoding: JSONEncoding.default) .responseJSON {
            response in
            
            print(response.result)
            
            self.utils.hideLoadingHUD(view: self.view)

            switch response.result {
            case .success(let value):
                let result = value as? [String: Any]
                let numberOfNetworks = (result!["n"] as? Int)!     //n = number of Wi-Fi networks found
                
                if (numberOfNetworks == 0) {
                    self.availableNetworksButton.setTitle("No Networks Found", for: .normal)
                    
                } else {
                    var arr = [UIAction]()
                                        
                    for i in 1 ... numberOfNetworks {
                        let networkName = (result!["n" + String(i)] as? String)!
                        let network = UIAction(title: networkName) { _ in
                            self.ssidTextField.text = networkName.trimString()
                        }
                        arr.append(network)
                        
                        print(networkName)
                    }
                    
                    self.availableNetworksButton.setTitle(String(numberOfNetworks) + " Networks Found", for: .normal)
                    
                    let menu = UIMenu(title: "Menu", children: arr)
                    self.availableNetworksButton.menu = menu
                    self.availableNetworksButton.showsMenuAsPrimaryAction = true
                }
                
            case .failure(_):
                self.availableNetworksButton.setTitle("No Networks Found", for: .normal)
            }
        }
    }
    
    //MARK: - Send a http request to 192.168.4.1/settings to send credentials.
    func sendCredentialsToCheckpoint(ssid: String, password: String) {
        let webServerUrl: String = Utils.Configuration.value(defaultValue: "-1", forKey: "webserver_url")

        var URL: String = "http://192.168.4.1/setting?ssid=" + ssid + "&pass=" + password + "&web_url=" + webServerUrl
        URL = URL.addingPercentEncoding(withAllowedCharacters: .urlQueryAllowed)!
        
        AF.request(URL, method: .get, encoding: JSONEncoding.default) .responseJSON {
            response in
            
            self.utils.hideLoadingHUD(view: self.view)
            
            switch response.result {
            case .success(let value):
                let result = value as? [String: Any]
                let response = (result!["r"] as? String)!     //r = Response
                
                if (response == "cre_rec") {  //cre_rec = Credentials Received
                    self.utils.showAlert(message: "Credentials saved to checkpoint!", view: self)
                    
                } else {
                    self.utils.showAlert(message: "Failed to save credentials!", view: self)
                }
                
            case .failure(_):
                self.utils.showAlert(message: "Failed to save credentials!", view: self)
            }
            
            DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
                self.navigationController!.popToRootViewController(animated: true)
            }
        }
    }
}

extension String {
    func trimString() -> String {
        return self.trimmingCharacters(in: NSCharacterSet.whitespaces)
    }
}
