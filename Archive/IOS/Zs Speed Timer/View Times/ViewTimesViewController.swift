//
//  ViewTimesViewController.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 14/02/2022.
//

import UIKit
import RealmSwift

class ViewTimesViewController: UIViewController, UITableViewDelegate, UITableViewDataSource {
    @IBOutlet weak var tableView: UITableView!
    @IBOutlet weak var noRunsFoundImageView: UIImageView!

    var realm = try! Realm()
    var count = -1
    
    let utils = Utils()
    
    override func viewDidLoad() {
        super.viewDidLoad()
        
        self.tableView.register(RunTableViewCell.nib(), forCellReuseIdentifier: RunTableViewCell.identifier)
        self.tableView.delegate = self
        self.tableView.dataSource = self
    }
    
    //MARK: - View will appear
    override func viewWillAppear(_ animated: Bool) {
        super.viewWillAppear(animated)
        
        //Get all runs from database
        let runs = realm.objects(Run.self)
        count = runs.count
        
        if (runs.isEmpty) {
            noRunsFoundImageView.isHidden = false
            
        } else {
            self.tableView.reloadData()
        }
    }
    
    // MARK: - Table View Data Source
    func tableView(_ tableView: UITableView, heightForHeaderInSection section: Int) -> CGFloat {
        return CGFloat.leastNormalMagnitude
    }

    func tableView(_ tableView: UITableView, heightForFooterInSection section: Int) -> CGFloat {
        return CGFloat.leastNormalMagnitude
    }
    
    func tableView(_ tableView: UITableView, numberOfRowsInSection section: Int) -> Int {
        return count
    }
    
    func tableView(_ tableView: UITableView, cellForRowAt indexPath: IndexPath) -> UITableViewCell {
        let cell = tableView.dequeueReusableCell(withIdentifier: RunTableViewCell.identifier, for: indexPath) as! RunTableViewCell
        
        cell.configure(with: realm.objects(Run.self)[indexPath.row])
        return cell
    }
    
    func tableView(_ tableView: UITableView, didSelectRowAt indexPath: IndexPath) {
        self.tableView.deselectRow(at: indexPath, animated: false)
    }
    
    //MARK: - Export CSV Pressed
    @IBAction func exportCSVButtonPressed(_ sender: Any) {
        self.createCSVFile()
    }
    
    //MARK: - Create CSV File
    func createCSVFile() {
        var csvString = "\("Athlete Name"), \("Course Name"), \("Date"), \("Start Time"), \("Total Time"), \("CP1"), \("CP2"), \("CP3"), \("CP4"), \("CP5")\n\n"
        
        //Get all runs from database
        let runs = realm.objects(Run.self)
        
        if (!runs.isEmpty) {
            for run in runs {
                csvString = csvString.appending("\(run.athleteName), \(run.courseName), \(run.date), \(run.startTime), \(run.totalRunTime), \(run.cp1), \(run.cp2), \(run.cp3), \(run.cp4), \(run.cp5)\n")
            }
            
            do {
                //Set Current Time in the run object
                let date = Date()
                let formatter = DateFormatter()
                formatter.dateFormat = "hh_mm a MM-dd-yyyy"
                let timeString = formatter.string(from: date)
                let fileName = "Runs Record " + timeString + ".csv"
                
                let path = NSURL(fileURLWithPath: NSTemporaryDirectory()).appendingPathComponent(fileName)
                try csvString.write(to: path!, atomically: true, encoding: .utf8)
                let exportSheet = UIActivityViewController(activityItems: [path as Any], applicationActivities: nil)
                self.present(exportSheet, animated: true, completion: nil)
            } catch {
                self.utils.showAlert(message: "Failed! Please try again!", view: self)
            }
            
        } else {
            self.utils.showAlert(message: "No runs found!", view: self)
        }
    }
}
