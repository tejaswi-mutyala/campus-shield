# CampusShield – Smart Campus Safety & Emergency Response System

## Problem
Campus emergencies and safety complaints can be difficult to report, assign, track and close using informal communication.

## Solution
CampusShield is a C++ + SQLite incident-management prototype. Students report incidents; security/admin users assign and update them through a controlled workflow.

## Core workflow
Reported → Assigned → In Progress → Resolved

## Features
- Student/security/admin user records
- Incident reporting with category, location and priority
- Incident list and assignment
- Status tracking
- Incident update/history log
- SQLite relational database
- Foreign-key relationships and validation

## Tech
C++, SQLite, SQL, DBMS

## Build
Linux/macOS:
`g++ main.cpp -lsqlite3 -o campusshield`
`./campusshield`

Windows MinGW (if SQLite development files are installed):
`g++ main.cpp -lsqlite3 -o campusshield.exe`
`campusshield.exe`

The program automatically creates `campusshield.db` and inserts sample users on first run.

## Demo IDs
1 = Aarav Student
2 = Security Ravi
3 = Admin Priya



