//
//  RunTableViewCell.swift
//  Zs Speed Timer
//
//  Created by Muhammad Hammad on 14/02/2022.
//

import UIKit

class RunTableViewCell: UITableViewCell {
    @IBOutlet weak var backView: UIView!
    @IBOutlet weak var topView: UIView!
    @IBOutlet weak var scoreLabel: UILabel!
    @IBOutlet weak var athleteNameLabel: UILabel!
    @IBOutlet weak var courseNameLabel: UILabel!
    @IBOutlet weak var dateTimeLabel: UILabel!
    @IBOutlet weak var totalTimeLabel: UILabel!
    
    static let identifier = "RunTableViewCell"

    override func awakeFromNib() {
        super.awakeFromNib()
        
        //Views stroke and corner
        self.backView.layer.cornerRadius = 8
        self.backView.layer.borderWidth = 1
        self.backView.layer.borderColor = UIColor.systemIndigo.cgColor
        
        self.topView.layer.cornerRadius = 8
        self.topView.layer.borderWidth = 1
        self.topView.layer.borderColor = UIColor.white.cgColor
    }
    
    public func configure(with run: Run) {
        self.athleteNameLabel.text = run.athleteName
        self.courseNameLabel.text = run.courseName
        
        let score: String = "CP1: " + run.cp1 + "          " + "CP2: " + run.cp2 + "\n" +
        "CP3: " + run.cp3 + "          " + "CP4: " + run.cp4 + "\n" + "CP5: " + run.cp5
        self.scoreLabel.text = score
        
        let dateTime = run.startTime + " " + run.date
        self.dateTimeLabel.text = dateTime
        
        self.totalTimeLabel.text = "Total Time: " + run.totalRunTime
    }
    
    static func nib() -> UINib {
        return UINib(nibName: "RunTableViewCell", bundle: nil)
    }
    
    override func setSelected(_ selected: Bool, animated: Bool) {
        super.setSelected(selected, animated: animated)
    }
}
