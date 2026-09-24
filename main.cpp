#include <iostream>
#include <string>
#include <sqlite3.h>
#include <limits>
using namespace std;

sqlite3* db = nullptr;

void execSQL(const string& sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        cerr << "Database error: " << (err ? err : "unknown") << "\n";
        sqlite3_free(err);
    }
}

void initDB() {
    execSQL(R"SQL(
    PRAGMA foreign_keys = ON;
    CREATE TABLE IF NOT EXISTS users(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        name TEXT NOT NULL,
        phone TEXT,
        role TEXT NOT NULL CHECK(role IN ('STUDENT','SECURITY','ADMIN'))
    );
    CREATE TABLE IF NOT EXISTS incidents(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        reporter_id INTEGER NOT NULL,
        category TEXT NOT NULL,
        description TEXT NOT NULL,
        location TEXT NOT NULL,
        priority TEXT NOT NULL CHECK(priority IN ('LOW','MEDIUM','HIGH','CRITICAL')),
        status TEXT NOT NULL DEFAULT 'REPORTED'
            CHECK(status IN ('REPORTED','ASSIGNED','IN_PROGRESS','RESOLVED')),
        assigned_to INTEGER,
        created_at TEXT DEFAULT CURRENT_TIMESTAMP,
        resolved_at TEXT,
        FOREIGN KEY(reporter_id) REFERENCES users(id),
        FOREIGN KEY(assigned_to) REFERENCES users(id)
    );
    CREATE TABLE IF NOT EXISTS incident_updates(
        id INTEGER PRIMARY KEY AUTOINCREMENT,
        incident_id INTEGER NOT NULL,
        updated_by INTEGER NOT NULL,
        old_status TEXT,
        new_status TEXT NOT NULL,
        note TEXT,
        updated_at TEXT DEFAULT CURRENT_TIMESTAMP,
        FOREIGN KEY(incident_id) REFERENCES incidents(id),
        FOREIGN KEY(updated_by) REFERENCES users(id)
    );
    )SQL");
}

void seed() {
    execSQL("INSERT INTO users(name,phone,role) SELECT 'Aarav Student','9000000001','STUDENT' WHERE NOT EXISTS(SELECT 1 FROM users WHERE name='Aarav Student');");
    execSQL("INSERT INTO users(name,phone,role) SELECT 'Security Ravi','9000000002','SECURITY' WHERE NOT EXISTS(SELECT 1 FROM users WHERE name='Security Ravi');");
    execSQL("INSERT INTO users(name,phone,role) SELECT 'Admin Priya','9000000003','ADMIN' WHERE NOT EXISTS(SELECT 1 FROM users WHERE name='Admin Priya');");
}

void pause() { cout << "\nPress Enter to continue..."; cin.ignore(numeric_limits<streamsize>::max(), '\n'); }

int askInt(const string& msg) {
    int x;
    cout << msg;
    while (!(cin >> x)) {
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Enter a number: ";
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return x;
}

string ask(const string& msg) {
    string s; cout << msg; getline(cin, s); return s;
}

bool userExists(int id, const string& role="") {
    sqlite3_stmt* st=nullptr;
    string sql="SELECT 1 FROM users WHERE id=?";
    if(!role.empty()) sql += " AND role=?";
    sqlite3_prepare_v2(db, sql.c_str(), -1, &st, nullptr);
    sqlite3_bind_int(st,1,id);
    if(!role.empty()) sqlite3_bind_text(st,2,role.c_str(),-1,SQLITE_TRANSIENT);
    bool ok = sqlite3_step(st)==SQLITE_ROW;
    sqlite3_finalize(st);
    return ok;
}

void listUsers() {
    cout << "\nID   Name                 Role       Phone\n";
    cout << "-----------------------------------------------\n";
    sqlite3_stmt* st=nullptr;
    sqlite3_prepare_v2(db,"SELECT id,name,role,COALESCE(phone,'') FROM users ORDER BY id",-1,&st,nullptr);
    while(sqlite3_step(st)==SQLITE_ROW)
        cout << sqlite3_column_int(st,0) << "    "
             << sqlite3_column_text(st,1) << "    "
             << sqlite3_column_text(st,2) << "    "
             << sqlite3_column_text(st,3) << "\n";
    sqlite3_finalize(st);
}

void reportIncident() {
    int reporter=askInt("Reporter student ID: ");
    if(!userExists(reporter,"STUDENT")) { cout<<"Invalid student ID.\n"; return; }
    string category=ask("Category (Medical/Security/Fire/Harassment/Other): ");
    string desc=ask("Description: ");
    string loc=ask("Location/building: ");
    string priority=ask("Priority (LOW/MEDIUM/HIGH/CRITICAL): ");
    if(priority!="LOW"&&priority!="MEDIUM"&&priority!="HIGH"&&priority!="CRITICAL") priority="MEDIUM";

    sqlite3_stmt* st=nullptr;
    sqlite3_prepare_v2(db,"INSERT INTO incidents(reporter_id,category,description,location,priority) VALUES(?,?,?,?,?)",-1,&st,nullptr);
    sqlite3_bind_int(st,1,reporter);
    sqlite3_bind_text(st,2,category.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(st,3,desc.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(st,4,loc.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(st,5,priority.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_step(st); sqlite3_finalize(st);
    cout<<"Incident reported successfully. Incident ID: "<<sqlite3_last_insert_rowid(db)<<"\n";
}

void viewIncidents() {
    sqlite3_stmt* st=nullptr;
    sqlite3_prepare_v2(db,R"SQL(
      SELECT i.id,u.name,i.category,i.location,i.priority,i.status,
             COALESCE(a.name,'Unassigned'),i.created_at
      FROM incidents i JOIN users u ON i.reporter_id=u.id
      LEFT JOIN users a ON i.assigned_to=a.id ORDER BY i.id DESC
    )SQL",-1,&st,nullptr);
    cout << "\nID | Reporter | Category | Location | Priority | Status | Assigned | Created\n";
    cout << "-------------------------------------------------------------------------------\n";
    while(sqlite3_step(st)==SQLITE_ROW) {
        for(int c=0;c<8;c++) cout << (c?" | ":"") << sqlite3_column_text(st,c);
        cout<<"\n";
    }
    sqlite3_finalize(st);
}

void assignIncident() {
    int id=askInt("Incident ID: ");
    int sec=askInt("Security/Admin user ID: ");
    if(!userExists(sec) ) { cout<<"Invalid user.\n"; return; }
    sqlite3_stmt* st=nullptr;
    sqlite3_prepare_v2(db,"SELECT status FROM incidents WHERE id=?",-1,&st,nullptr);
    sqlite3_bind_int(st,1,id);
    if(sqlite3_step(st)!=SQLITE_ROW){ cout<<"Incident not found.\n"; sqlite3_finalize(st); return; }
    string old=(const char*)sqlite3_column_text(st,0); sqlite3_finalize(st);

    sqlite3_prepare_v2(db,"UPDATE incidents SET assigned_to=?,status='ASSIGNED' WHERE id=?",-1,&st,nullptr);
    sqlite3_bind_int(st,1,sec); sqlite3_bind_int(st,2,id);
    sqlite3_step(st); sqlite3_finalize(st);

    sqlite3_prepare_v2(db,"INSERT INTO incident_updates(incident_id,updated_by,old_status,new_status,note) VALUES(?,?,?,'ASSIGNED','Incident assigned')",-1,&st,nullptr);
    sqlite3_bind_int(st,1,id); sqlite3_bind_int(st,2,sec); sqlite3_bind_text(st,3,old.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_step(st); sqlite3_finalize(st);
    cout<<"Incident assigned.\n";
}

void updateStatus() {
    int id=askInt("Incident ID: ");
    int by=askInt("Updater user ID: ");
    string ns=ask("New status (IN_PROGRESS/RESOLVED): ");
    if(ns!="IN_PROGRESS"&&ns!="RESOLVED"){cout<<"Invalid status.\n";return;}

    sqlite3_stmt* st=nullptr;
    sqlite3_prepare_v2(db,"SELECT status FROM incidents WHERE id=?",-1,&st,nullptr);
    sqlite3_bind_int(st,1,id);
    if(sqlite3_step(st)!=SQLITE_ROW){cout<<"Incident not found.\n";sqlite3_finalize(st);return;}
    string old=(const char*)sqlite3_column_text(st,0); sqlite3_finalize(st);

    sqlite3_prepare_v2(db,"UPDATE incidents SET status=?,resolved_at=CASE WHEN ?='RESOLVED' THEN CURRENT_TIMESTAMP ELSE resolved_at END WHERE id=?",-1,&st,nullptr);
    sqlite3_bind_text(st,1,ns.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,2,ns.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_int(st,3,id);
    sqlite3_step(st); sqlite3_finalize(st);

    string note=ask("Update note: ");
    sqlite3_prepare_v2(db,"INSERT INTO incident_updates(incident_id,updated_by,old_status,new_status,note) VALUES(?,?,?,?,?)",-1,&st,nullptr);
    sqlite3_bind_int(st,1,id); sqlite3_bind_int(st,2,by); sqlite3_bind_text(st,3,old.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_bind_text(st,4,ns.c_str(),-1,SQLITE_TRANSIENT); sqlite3_bind_text(st,5,note.c_str(),-1,SQLITE_TRANSIENT);
    sqlite3_step(st); sqlite3_finalize(st);
    cout<<"Status updated.\n";
}

void history() {
    int id=askInt("Incident ID: ");
    sqlite3_stmt* st=nullptr;
    sqlite3_prepare_v2(db,R"SQL(
      SELECT u.name, x.old_status, x.new_status, x.note, x.updated_at
      FROM incident_updates x JOIN users u ON x.updated_by=u.id
      WHERE x.incident_id=? ORDER BY x.id
    )SQL",-1,&st,nullptr);
    sqlite3_bind_int(st,1,id);
    cout<<"\nHistory:\n";
    while(sqlite3_step(st)==SQLITE_ROW)
        cout<<sqlite3_column_text(st,4)<<" | "<<sqlite3_column_text(st,0)
            <<" | "<<(sqlite3_column_text(st,1)?(const char*)sqlite3_column_text(st,1):"-")
            <<" -> "<<sqlite3_column_text(st,2)<<" | "<<sqlite3_column_text(st,3)<<"\n";
    sqlite3_finalize(st);
}

int main(){
    if(sqlite3_open("campusshield.db",&db)!=SQLITE_OK){cerr<<"Cannot open database\n";return 1;}
    initDB(); seed();
    int ch;
    do{
        cout<<"\n========== CAMPUSSHIELD ==========\n"
            <<"1. List users\n2. Report incident\n3. View incidents\n4. Assign incident\n"
            <<"5. Update incident status\n6. View incident history\n0. Exit\n";
        ch=askInt("Choose: ");
        switch(ch){case 1:listUsers();break;case 2:reportIncident();break;case 3:viewIncidents();break;
        case 4:assignIncident();break;case 5:updateStatus();break;case 6:history();break;case 0:break;
        default:cout<<"Invalid option.\n";}
    }while(ch!=0);
    sqlite3_close(db);
    return 0;
}
