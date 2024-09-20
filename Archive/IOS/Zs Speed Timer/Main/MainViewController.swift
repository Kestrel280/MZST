//
//  ViewController.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 03/02/2022.
//

import UIKit
import Network
import GCDWebServer

class MainViewController: UIViewController {
    let utils = Utils()
    var webServer = GCDWebServer()
    public static var isWifiConnected: Bool = false
    @IBOutlet weak var webserverStatusLabel: UILabel!
    var courseSet: Bool = false

    enum State {
        case IDLE_STATE
        case SETUP_CHECKPOINT
        case SET_COURSE
        case RUN_STARTED
        case RUN_WILL_START
    }
    public static var state: State = .IDLE_STATE
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        //Start the Network Monitor
        let monitor = NWPathMonitor(requiredInterfaceType: .wifi)
        let queue = DispatchQueue.global(qos: .background)
        monitor.start(queue: queue)
        
        monitor.pathUpdateHandler = { path in
            if path.status == .satisfied { //Wi-Fi was connected!
                DispatchQueue.main.async {
                    //Stop the server if already running
                    if (self.webServer.isRunning) {
                        self.webServer.stop()
                    }
                    MainViewController.isWifiConnected = true
                    //Start the web-server if the state is IDLE
                    
                    if MainViewController.state == .IDLE_STATE {
                        self.startWebServer()
                    }
                }
            } else {
                DispatchQueue.main.async {
                    MainViewController.isWifiConnected = false
                    self.webserverStatusLabel.textColor = UIColor.systemRed
                    self.webserverStatusLabel.text = "Server Not Started!"
                    
                    //Stop the server if already running
                    if (self.webServer.isRunning) {
                        self.webServer.stop()
                    }
                }
            }
        }
    }
    
    //MARK: - View will Disappear
    ///If wifi is connected: start it
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        //Update the state
        MainViewController.state = .IDLE_STATE
        
        if (MainViewController.isWifiConnected && !self.webServer.isRunning) {
            self.startWebServer()
        }
        
        //Check if course is set
        for i in 1 ... 5 {
            let prefKey: String = "c_p_" + String(i)
            if (Utils.Configuration.value(defaultValue: "-1", forKey: prefKey) != "-1") {
                courseSet = true
            }
        }
        
    }
    
    @IBAction func setupCheckPointsPressed(_ sender: Any) {
        let webServerURL = Utils.Configuration.value(defaultValue: "-1", forKey: "webserver_url")
        
        if (webServerURL != "-1") {
            self.performSegue(withIdentifier: "showSetupWifi", sender: self)
            
        } else {
            self.utils.showAlert(message: "Please connect to the Local Router first!", view: self)
        }
        
    }
    
    @IBAction func setCoursePressed(_ sender: Any) {
        if (self.webServer.isRunning) {
            self.performSegue(withIdentifier: "showSetCourse", sender: self)
            
        } else {
            self.utils.showAlert(message: "Please connect to the Local Router first to start the Web-Server.", view: self)
        }
    }
    
    @IBAction func startRunPressed(_ sender: Any) {
        if (!self.webServer.isRunning) {
            self.utils.showAlert(message: "Please connect to the Local Router first to start the Web-Server.", view: self)
            
        } else if (!courseSet) {
            self.utils.showAlert(message: "Please set the course first!", view: self)
            
        } else {
            self.performSegue(withIdentifier: "showRunCourse", sender: self)
        }
    }
    
    @IBAction func viewTimesPressed(_ sender: Any) {
        self.performSegue(withIdentifier: "showViewTimes", sender: self)
    }
    
    //MARK: - Web Server
    func startWebServer() {
        self.webServer.addDefaultHandler(forMethod: "GET", request: GCDWebServerRequest.self) { request in
            //Button Press Request
            
            var response = GCDWebServerDataResponse(text: "Hello World")
            
            if (request.path.contains("button_press")) {
                let checkpointId = request.query?["id"] ?? "-1"
                
                if (checkpointId != "-1") {
                    let dict: [String: String] = ["id": checkpointId]

                    if (MainViewController.state == .SET_COURSE) {
                        NotificationCenter.default.post(name: Notification.Name("checkpointPushButtonPress"), object: nil, userInfo: dict)
                        
                        response = GCDWebServerDataResponse(text: "order_saved")
                        
                    } else if (MainViewController.state == .RUN_STARTED || MainViewController.state == .RUN_WILL_START) {
                        NotificationCenter.default.post(name: Notification.Name("checkpointPushButtonPressStartRun"), object: nil, userInfo: dict)
                        
                        response = GCDWebServerDataResponse(text: "checkpoint_hit")
                        
                    }
                }
            } else if (request.path.contains("status_check")) {
                var res = ""
                if (MainViewController.state == .IDLE_STATE) {
                    res = "IDLE_STATE"
                } else if (MainViewController.state == .SETUP_CHECKPOINT) {
                    res = "SETUP_CHECKPOINT"
                } else if (MainViewController.state == .SET_COURSE) {
                    res = "SET_COURSE"
                } else if (MainViewController.state == .RUN_STARTED) {
                    res = "RUN_STARTED"
                } else if (MainViewController.state == .RUN_WILL_START) {
                    res = "RUN_WILL_START"
                }
                response = GCDWebServerDataResponse(text: res)
            }
            
            return response
        }
        
        self.webServer.start(withPort: 80, bonjourName: nil)
        let webServerURL: String = self.webServer.serverURL?.host ?? "-1"
        
        if (webServerURL != "-1" && !webServerURL.contains("4.")) {
            Utils.Configuration.value(value: webServerURL, forKey: "webserver_url")
            self.webserverStatusLabel.textColor = UIColor.systemGreen
            self.webserverStatusLabel.text = "Server Started!"
            
            print("Web Server Started")
            print(webServerURL)
        }
    }
}

