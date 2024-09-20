//
//  SetupCheckpointsConnectToWifiViewController.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 11/02/2022.
//

import UIKit

class SetupCheckpointsConnectToWifiViewController: UIViewController {
    @IBOutlet weak var continueButton: UIButton!
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        //Set Next Button Stroke
        self.continueButton.layer.cornerRadius = 8
        self.continueButton.layer.borderWidth = 1
        self.continueButton.layer.borderColor = UIColor.black.cgColor
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        //Update the state
        MainViewController.state = .SETUP_CHECKPOINT
    }
    
    //MARK: - Open Settings
    @IBAction func settingsPressed(_ sender: Any) {
        let shared = UIApplication.shared
        let url = URL(string: UIApplication.openSettingsURLString)!
        
        if #available(iOS 10.0, *) {
            shared.open(url)
        } else {
            shared.openURL(url)
        }
    }
    
    //MARK: - Continue Button Pressed
    @IBAction func continueButtonPressed(_ sender: Any) {
        self.performSegue(withIdentifier: "showSetupCheckpointsCredentials", sender: self)
    }
}
