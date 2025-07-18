# MZST

MZST is a hobby project which aims to bring structured competition in a gamified format to [Parkour](https://en.wikipedia.org/wiki/Parkour). MZST consists of a set of physical modules which can be installed in a parkour gym, along with a server which networks the modules and a user application. Players design "courses" with the user application, then race against others in a time-trial format to compete on leaderboards for the quickest times to complete these courses.

MZST is a collaboration between Alec Reduker (hardware, procurement, talent) and Sam Downey (software, firmware). Originally a [class project](https://www.youtube.com/watch?v=ushaRkm61o4), we are actively developing MZST with the goal of implementing it in a local parkour gym.

# Technical Overview
There are five major components to the MZST platform. Further technical details on each individual component can be found on the [MZST wiki](https://github.com/Kestrel280/MZST/wiki).

## Network Modules
These are the physical, user-interactable sensors which are placed throughout the gym. They are powered by an ESP32-S3 microcontroller integrated into a custom-designed PCB and structural housing, with firmware written in Espressif's ESP-IDF development framework in C/C++. The package is augmented with an RF-receiver submodule, for highly synchronous communication with a *Transmitter Module*, as well as LEDs and a speaker to provide player feedback. 

## Transmitter (Timing) Module(s)
The Transmitter Module (another ESP32-S3 custom integration) is responsible for tightly synchronizing the timings of all Network Modules in the system, which are incapable of precise timing on their own due to small variations in clock timings and unpredictable server latencies. When precise timing is needed, the Transmitter Module broadcasts a powerful RF signal which is received by submodules on each Network Module to reset/re-synchronize their internal timers.

## Server
The Server is the central brain of the system, currently implemented as an Android application written in Java and deployed on a Google Pixel. The server manages the state of the system, and is in a constant communication loop with all connected modules. The server informs the modules of changes in system state, while also receiving and processing event notifications from modules (e.g. a Network Module may send a message indicating it was triggered at a certain timestamp).

*Note: the current Server program runs as an Android application which provides an interactable debug interface. However, internally, the debug interface and the Server program itself are decoupled, allowing us to easily port the Server to other, more permanent platforms in the future.*

## Users
***The user application is currently in the design phase and has not yet been implemented.***

The user application allows players to connect to the MZST platform and participate by designing new courses, playing on existing courses, browsing leaderboards, etc.

## Database
***The user application is currently in the design phase and has not yet been implemented.***

The database stores all user information, course information, and leaderboard information. 