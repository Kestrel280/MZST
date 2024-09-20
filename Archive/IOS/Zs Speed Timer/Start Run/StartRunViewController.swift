//
//  StartRunViewController.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 13/02/2022.
//

import UIKit
import RealmSwift

class StartRunViewController: UIViewController {
    @IBOutlet weak var scoreView: UIView!
    @IBOutlet weak var scoreLabel: UILabel!
    @IBOutlet weak var athleteNameTextField: UITextField!
    @IBOutlet weak var statusLabel: UILabel!
    @IBOutlet weak var readyButton: UIButton!
    @IBOutlet weak var timerLabel: UILabel!
    
    var checkpointsOrderArray = [String]()
    
    var numberOfCheckpointsHit = 0
    var athleteName: String = "-1"
    let utils = Utils()
    var isReady: Bool = false
    
    var counter = 0.00
    var timer = Timer()
    var centiSecondCount: Int = 0
    
    var currentElapsedTimeString: String = "00:00.00"
    
    let realm = try! Realm()
    var run = Run()

    override func viewDidLoad() {
        super.viewDidLoad()
        
        scoreView.layer.cornerRadius = 8
        scoreView.layer.borderWidth = 1
        scoreView.layer.borderColor = UIColor.black.cgColor
        
        //Get the Checkpoints order
        for i in 1 ... 5 {
            let prefKey: String = "c_p_" + String(i)
            
            let checkpoint = Utils.Configuration.value(defaultValue: "-1", forKey: prefKey)
            if (checkpoint != "-1") {
                checkpointsOrderArray.append(checkpoint)
            }
        }
        
        //Keyboard Handling
        let tap = UITapGestureRecognizer(target: self, action: #selector(UIInputViewController.dismissKeyboard))
        view.addGestureRecognizer(tap)
        
        NotificationCenter.default.addObserver(self, selector: #selector(checkpointPushButtonPress(_:)), name: Notification.Name("checkpointPushButtonPressStartRun"), object: nil)
    }
    
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        //Update the state
        MainViewController.state = .RUN_WILL_START
    }
    
    override func viewWillDisappear(_ animated: Bool) {
        super.viewWillDisappear(animated)
        self.timer.invalidate()
    }
    
    @objc
    func dismissKeyboard() {
        view.endEditing(true)
    }
    
    @IBAction func donePressed(_ sender: Any) {
        dismissKeyboard()
    }
    
    @IBAction func readyPressed(_ sender: Any) {
        let athleteName = self.athleteNameTextField.text!
        
        if (athleteName.isEmpty) {
            self.utils.showAlert(message: "Enter the Athlete Name!", view: self)
            
            self.statusLabel.textColor = UIColor.systemRed
            self.statusLabel.text = "Not Ready!"
            self.isReady = false
            
        } else {
            self.counter = 0
            self.athleteNameTextField.isEnabled = false
            self.statusLabel.textColor = UIColor.systemGreen
            self.statusLabel.text = "Ready!"
            self.readyButton.isEnabled = false
            self.isReady = true
            
            self.scoreLabel.text = "CP1: 00:00.00\n\nCP2: 00:00.00\n\nCP3: 00:00.00\n\nCP4: 00:00.00\n\nCP5: 00:00.00"
            self.timerLabel.text = "00:00.00"
            self.currentElapsedTimeString = "00:00.00"
            
            //Clear the run object
            self.run = Run()
            
            self.athleteName = athleteName
            self.run.athleteName = athleteName
            self.run.courseName = Utils.Configuration.value(defaultValue: "-1", forKey: "c_n")
            
            //Set Current Date in the run object
            let date = Date()
            let formatter = DateFormatter()
            formatter.dateFormat = "MM/dd/yyyy"
            let dateString = formatter.string(from: date)
            self.run.date = dateString
        }
    }
    
    //MARK: - Notification Function called from Main View Controller
    @objc
    func checkpointPushButtonPress(_ notification: Notification) {
        let checkpointId: String = notification.userInfo?["id"] as! String
        
        //Run on main thread
        DispatchQueue.main.async {
            if (self.isReady) { //If the run is ready to go
                self.numberOfCheckpointsHit += 1
                
                //Update the state
                MainViewController.state = .RUN_STARTED
                
                if (self.numberOfCheckpointsHit == 1) { //First Checkpoint Hit
                    self.scoreLabel.text = ""
                    //Start the timer
                    self.timer = Timer.scheduledTimer(timeInterval: 0.01, target: self, selector: #selector(self.UpdateTimer), userInfo: nil, repeats: true)
                    
                    //Set Current Time in the run object
                    let date = Date()
                    let formatter = DateFormatter()
                    formatter.dateFormat = "hh:mm a"
                    let timeString = formatter.string(from: date)
                    
                    self.run.startTime = timeString
                }
                
                //Update the status to started
                self.statusLabel.textColor = UIColor.systemGreen
                self.statusLabel.text = "Started!"
                
                //Check if index doesn't go out of bounds
                if (self.numberOfCheckpointsHit <= self.checkpointsOrderArray.count) {
                    //Check the order using the order array
                    if (checkpointId == self.checkpointsOrderArray[self.numberOfCheckpointsHit - 1]) {
                        //Correct Order
                        
                        if (self.numberOfCheckpointsHit == 1) {
                            self.run.cp1 = self.currentElapsedTimeString
                            
                        } else if (self.numberOfCheckpointsHit == 2) {
                            self.run.cp2 = self.currentElapsedTimeString
                            
                        } else if (self.numberOfCheckpointsHit == 3) {
                            self.run.cp3 = self.currentElapsedTimeString
                            
                        } else if (self.numberOfCheckpointsHit == 4) {
                            self.run.cp4 = self.currentElapsedTimeString
                            
                        } else if (self.numberOfCheckpointsHit == 5) {
                            self.run.cp5 = self.currentElapsedTimeString
                        }
                        
                        self.scoreLabel.text! += "C" + String(self.numberOfCheckpointsHit) + ": " + self.currentElapsedTimeString
                        
                        if (self.numberOfCheckpointsHit < 5) {
                            self.scoreLabel.text! += "\n\n"
                        }
                        
                        if (self.numberOfCheckpointsHit == self.checkpointsOrderArray.count) {
                            //Check if the number of checkpoints in a course are less than 5, insert "-" (dash) in the remaining
                            if (self.checkpointsOrderArray.count < 5) {
                                for i in (self.numberOfCheckpointsHit + 1) ... 5 {
                                    if (i == 1) {
                                        self.run.cp1 = "-"
                                        
                                    } else if (i == 2) {
                                        self.run.cp2 = "-"
                                        
                                    } else if (i == 3) {
                                        self.run.cp3 = "-"
                                        
                                    } else if (i == 4) {
                                        self.run.cp4 = "-"
                                        
                                    } else if (i == 5) {
                                        self.run.cp5 = "-"
                                    }
                                }
                            }
                            
                            self.numberOfCheckpointsHit = 0
                            
                            self.utils.showAlert(message: "Run Complete Successfully", view: self)
                            
                            self.statusLabel.textColor = UIColor.systemRed
                            self.statusLabel.text = "Not Ready!"
                            self.athleteNameTextField.isEnabled = true
                            self.readyButton.isEnabled = true
                            self.isReady = false
                            
                            //Set Total Time in Label
                            let minutes = Int(self.counter / 60)
                            let seconds = Int(self.counter.truncatingRemainder(dividingBy: 60))
                            let timeString = String(format:"%02d:%02d.%02d", minutes, seconds, self.centiSecondCount)
                            self.timerLabel.text = "Total Time: " + timeString
                            
                            //Set total time to object
                            self.run.totalRunTime = timeString
                            
                            //Save the Run Object to Realm Database
                            self.realm.beginWrite()
                            self.realm.add(self.run)
                            try! self.realm.commitWrite()
                            
                            //Reset the Timer
                            self.timer.invalidate()
                            self.counter = 0
                            self.currentElapsedTimeString = "00:00.00"
                            
                            //Update the state
                            MainViewController.state = .RUN_WILL_START
                        }
                        
                    } else { //Incorrect Order
                        self.scoreLabel.text! += "C" + String(self.numberOfCheckpointsHit) + ": Incorrect Order!"
                        self.utils.showAlert(message: "Incorrect Order! Run Invalid", view: self)
                        
                        self.run.cp1 = "Invalid Run"
                        self.run.cp2 = "Invalid Run"
                        self.run.cp3 = "Invalid Run"
                        self.run.cp4 = "Invalid Run"
                        self.run.cp5 = "Invalid Run"
                        self.run.totalRunTime = "00:00.00"
                        
                        self.numberOfCheckpointsHit = 0
                        
                        self.statusLabel.textColor = UIColor.systemRed
                        self.statusLabel.text = "Not Ready!"
                        self.athleteNameTextField.isEnabled = true
                        self.readyButton.isEnabled = true
                        self.isReady = false
                        
                        //Reset the Timer
                        self.timer.invalidate()
                        self.counter = 0
                        self.timerLabel.text = "00:00.00"
                        self.currentElapsedTimeString = "00:00.00"
                        
                        //Save the Run Object to Realm Database
                        self.realm.beginWrite()
                        self.realm.add(self.run)
                        try! self.realm.commitWrite()
                        
                        //Update the state
                        MainViewController.state = .RUN_WILL_START
                    }
                }
            }
        }
    }
    
    @objc func UpdateTimer() {
        counter = counter + 0.01
        
        centiSecondCount += 1
        
        let minutes = Int(counter / 60)
        let seconds = Int(counter.truncatingRemainder(dividingBy: 60))

        let timeString = String(format:"%02d:%02d.%02d", minutes, seconds, centiSecondCount)
        
        self.timerLabel.text = timeString
        self.currentElapsedTimeString = timeString
        
        if centiSecondCount >= 99 {
            centiSecondCount = 0
        }
    }
}
