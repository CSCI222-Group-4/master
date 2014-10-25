#include "Database.hpp"

#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"
#include "cppconn/prepared_statement.h"

#include <cstring>
#include <stdlib.h>
#include <iostream>

#include <vector>
#include <map>
#include <string>

using namespace sql;

const char* databaseStr = "localhost";
const char* databaseUserName = "user";
const char* databaseUserPassword = "user";
const char* databaseSchema = "csci222";

const char* noDB = "Sorry, but was unable to open database so operation failed";

const char* prepareFailed = "Failure when preparing statement";
const char* bindFailed = "Failure when binding arguments";
const char* writeFailed = "Failure when writing to database";

Database::Database()
{
        // Constructors should always succeed. But suppose it failed and
        // the database couldn't be created?
        invalid = true;
        driver = get_driver_instance();
        try {
                dbcon = driver->connect(databaseStr, databaseUserName, databaseUserPassword);
        } catch (sql::SQLException &e) {
                // Failed
                return;
        }
        dbcon->setSchema(databaseSchema);
        // It's ok
        invalid = false;
}

Database::~Database()
{
        if (!invalid) {
                this->close();
        }
}

void Database::close()
{
        if (!invalid) {
                dbcon->close();
                delete dbcon;
        }
        invalid = true;
}

User *Database::fetchUser(std::string key) throw (const char*)
{
        if (invalid)
                throw (noDB);

	// Add information on user from database to User object
	const char* getUser = "SELECT * FROM UserAccount where username=?";
	const char* getPersonalInfo = "SELECT * FROM PersonalInfo where userID=?";
	const char* getExpertise = "SELECT keyword FROM Keywords WHERE keywordID in (SELECT expertiseID FROM ExpertiseArea where userID=?)";

        // =======================================
        // user account
	sql::PreparedStatement *pstmt = NULL;
	sql::ResultSet *rs = NULL;
	
	pstmt = dbcon->prepareStatement(getUser);
	pstmt->setString(1, key);

	rs = pstmt->executeQuery();
	bool haveRecord = rs->next();
	if (!haveRecord)
	{
		delete rs;
		delete pstmt;
		return NULL;
	}

        int userID = rs->getInt(1);
        std::string username = rs->getString(2);
        std::string password = rs->getString(3);

        delete rs;
        delete pstmt;

        // =======================================
        // Personal Info
	pstmt = dbcon->prepareStatement(getPersonalInfo);
	pstmt->setInt(1, userID);
	rs = pstmt->executeQuery();
	haveRecord = rs->next();
	if (!haveRecord)
	{
		delete rs;
		delete pstmt;
		return NULL;
	}

        std::string name = rs->getString(3);
	std::string email = rs->getString(4);
	std::string organisation = rs->getString(5);
	std::string phone = rs->getString(6);

        delete rs;
        delete pstmt;

        // =======================================
        // Expertise Area
        std::vector<std::string> vec;
	
	pstmt = dbcon->prepareStatement(getExpertise);
	pstmt->setInt(1, userID);

	rs = pstmt->executeQuery();

        while (rs->next()) {
                std::string expertise = rs->getString(1);
                vec.push_back(expertise);
        }

        User *user = new User(
		username,
		name,
		email,
		organisation,
		phone,
		password,
		userID,
		vec);

        delete rs;
        delete pstmt;
	
	return user;
}

bool Database::existsUserName(std::string key) throw (const char*)
{
        if (invalid)
                throw (noDB);

	const char* countUser = "SELECT COUNT(*) FROM UserAccount WHERE username=?";

        sql::PreparedStatement *pstmt = NULL;
        sql::ResultSet * rs = NULL;

        pstmt = dbcon->prepareStatement(countUser);

        int count = 0;
        pstmt->setString(1, key);
        rs = pstmt->executeQuery();

        if (rs->next()) {
                count = rs->getInt(1);
        }
        delete rs;
        delete pstmt;

        return count == 1;
}

void Database::putUser(std::string key, const User *user) throw (const char*)
{
        if (invalid)
                throw (noDB);

        if (this->existsUserName(key))
	{
                this->updateUser(user);
        }
	else
	{
        	this->createUser(user);
	}
}

void Database::createUser(const User* user)
{
        if (invalid)
                throw (noDB);

	const char* insertPersonalInfo = "INSERT INTO PersonalInfo(userID, name, email, organisation, phone) VALUES(NULL, ?, ?, ?, ?)";
	const char* insertUser = "INSERT INTO UserAccount(username, password, infoID) VALUES(?, ?, LAST_INSERT_ID())";
	const char* updatePIuserID = "UPDATE PersonalInfo SET userID = LAST_INSERT_ID() where infoID=(SELECT infoID FROM UserAccount where userID=LAST_INSERT_ID())";
	const char* getUserID = "SELECT LAST_INSERT_ID()";
	const char* insertExpertise = "INSERT IGNORE INTO ExpertiseArea VALUES(?, (SELECT keywordID FROM Keywords WHERE keyword=?))";

        // =======================================
        // Personal Info
        sql::PreparedStatement *pstmt = NULL;
        pstmt = dbcon->prepareStatement(insertPersonalInfo);
        pstmt->setString(1, user->name);
        pstmt->setString(2, user->email);
        pstmt->setString(3, user->organisation);
        pstmt->setString(4, user->phone);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // user account
        pstmt = dbcon->prepareStatement(insertUser);
        pstmt->setString(1, user->userName);
        pstmt->setString(2, user->password);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Personal Info Update
        pstmt = dbcon->prepareStatement(updatePIuserID);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Store userID
	sql::ResultSet *rs = NULL;
	
	pstmt = dbcon->prepareStatement(getUserID);

	rs = pstmt->executeQuery();
	bool haveRecord = rs->next();

        int userID = rs->getInt(1);

        delete rs;
        delete pstmt;

        // =======================================
        // Expertise
      	if (user->keywords.size() > 0)
      	{
      		pstmt = dbcon->prepareStatement(insertExpertise);
		std::vector<std::string>::const_iterator it;
      		for (it = user->keywords.begin(); it != user->keywords.end(); it ++)
		{
			std::string word = *it;	

        		if (!this->existsKeyword(word))
			{
				this->addKeyword(word);
			}
			
			pstmt->setInt(1, userID);
			pstmt->setString(2, word);

      		        pstmt->executeUpdate();
		}
      		delete pstmt;

      	}
}

void Database::updateUser(const User* user)
{
        if (invalid)
                throw (noDB);

	const char* getUserID = "SELECT userID FROM UserAccount where username = ?";
	const char* updateUser = "UPDATE UserAccount SET username=?, password=? WHERE userID=?";
	const char* updatePersonalInfo = "UPDATE PersonalInfo SET name=?, email=?, organisation=?, phone=? WHERE infoID=?";

        const char* deleteExpertise = "DELETE FROM ExpertiseArea WHERE userID=?";
	const char* insertExpertise = "INSERT IGNORE INTO ExpertiseArea VALUES(?, (SELECT keywordID FROM Keywords WHERE keyword=?))";

        // =======================================
        // Store userID
        sql::PreparedStatement *pstmt = NULL;
	sql::ResultSet *rs = NULL;
	
	pstmt = dbcon->prepareStatement(getUserID);
        pstmt->setString(1, user->userName);

	rs = pstmt->executeQuery();
	bool haveRecord = rs->next();

        int userID = rs->getInt(1);

        delete rs;
        delete pstmt;

        // =======================================
        // user account
        pstmt = dbcon->prepareStatement(updateUser);
        pstmt->setString(1, user->userName);
        pstmt->setString(2, user->password);
        pstmt->setInt(3, userID);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Personal Info
        pstmt = dbcon->prepareStatement(updatePersonalInfo);
        pstmt->setString(1, user->name);
        pstmt->setString(2, user->email);
        pstmt->setString(3, user->organisation);
        pstmt->setString(4, user->phone);
        pstmt->setInt(5, userID);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Expertise Delete
        pstmt = dbcon->prepareStatement(deleteExpertise);
        pstmt->setInt(1, userID);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Expertise Insert
      	if (user->keywords.size() > 0)
      	{
      		pstmt = dbcon->prepareStatement(insertExpertise);
		std::vector<std::string>::const_iterator it;
      		for (it = user->keywords.begin(); it != user->keywords.end(); it ++)
		{
			std::string word = *it;	
        		if (!this->existsKeyword(word))
			{
				this->addKeyword(word);
			}
			pstmt->setInt(1, userID);
			pstmt->setString(2, word);

      		        pstmt->executeUpdate();
		}
      		delete pstmt;

      	}
}

std::vector<int> *Database::allUserIDs()
{
        if (invalid)
                throw (noDB);
        std::vector<int> *vptr = new std::vector<int>();

        const char* selectall = "select userID from UserAccount";

        sql::PreparedStatement *pstmt = NULL;
        sql::ResultSet * rs = NULL;

        pstmt = dbcon->prepareStatement(selectall);

        rs = pstmt->executeQuery();
        while (rs->next()) {
                int anid = rs->getInt(1);
                vptr->push_back(anid);
        }

        return vptr;
}

std::vector<std::string> *Database::allUserNames()
{
        if (invalid)
                throw (noDB);
        std::vector<std::string> *vptr = new std::vector<std::string>();

        const char* selectall = "select username from UserAccount";

        sql::PreparedStatement *pstmt = NULL;
        sql::ResultSet * rs = NULL;

        pstmt = dbcon->prepareStatement(selectall);

        rs = pstmt->executeQuery();
        while (rs->next()) {
                std::string aname = rs->getString(1);
                vptr->push_back(aname);
        }

        return vptr;
}

bool Database::existsKeyword(std::string key) throw (const char*)
{
        if (invalid)
                throw (noDB);

	const char* countKeyword = "SELECT COUNT(*) FROM Keywords WHERE keyword=?";

        sql::PreparedStatement *pstmt = NULL;
        sql::ResultSet * rs = NULL;

        pstmt = dbcon->prepareStatement(countKeyword);

        int count = 0;
        pstmt->setString(1, key);
        rs = pstmt->executeQuery();

        if (rs->next()) {
                count = rs->getInt(1);
        }
        delete rs;
        delete pstmt;

        return count == 1;
}

void Database::addKeyword(std::string key) throw (const char*)
{
	if (invalid)
		throw (noDB);

	const char* insertKeyword = "INSERT INTO Keywords VALUES(NULL, ?)";

        sql::PreparedStatement *pstmt = NULL;
        pstmt = dbcon->prepareStatement(insertKeyword);
        pstmt->setString(1, key);

        pstmt->executeUpdate();

        delete pstmt;
}

Conference *Database::fetchConference(int key) throw (const char*)
{
        if (invalid)
                throw (noDB);

	// Add information on user from database to User object
	const char* getConference = "SELECT * FROM Conference WHERE confID=?
	const char* getConfKeywords = "SELECT keyword FROM Keywords WHERE keywordID in (SELECT keywordID FROM ConferenceKeywords where confID=?)";

        // =======================================
        // Conference 
	sql::PreparedStatement *pstmt = NULL;
	sql::ResultSet *rs = NULL;
	
	pstmt = dbcon->prepareStatement(getConference);
	pstmt->setInt(1, key);

	rs = pstmt->executeQuery();
	bool haveRecord = rs->next();
	if (!haveRecord)
	{
		delete rs;
		delete pstmt;
		return NULL;
	}

        int confID = rs->getInt(1);
        std::string title = rs->getString(2);
        std::string topic = rs->getString(3);
        std::string description = rs->getString(4);
        std::string location = rs->getString(5);
        bool isActive = rs->getBoolean(6);
        Date paperDeadline  = rs->getDate(7);
        bool isBeforePaperDeadline = rs->getBoolean(8);
        Date allocationDate  = rs->getDate(9);
        bool isBeforeAllocationDate = rs->getBoolean(10);
        Date reviewDeadlineSoft  = rs->getDate(11);
        bool isBeforeSoftReviewDeadline = rs->getBoolean(12);
        Date reviewDeadlineHard  = rs->getDate(13);
        bool isBeforeHardReviewDeadline = rs->getBoolean(14);
        Date discussDeadline  = rs->getDate(15);
        bool isBeforeDiscussDeadline = rs->getBoolean(16);
	int reviewersPerPaper = rs->getInt(17);
	int postWordLimit = rs->getInt(18);

        delete rs;
        delete pstmt;

        // =======================================
        // Expertise Area
        std::vector<std::string> vec;
	
	pstmt = dbcon->prepareStatement(getConfKeywords);
	pstmt->setInt(1, key);
	rs = pstmt->executeQuery();

        while (rs->next()) {
                std::string keyword = rs->getString(2);
                vec.push_back(keyword);
        }

        delete rs;
        delete pstmt;

        Conference *conf = new Conference(
		isActive,
		title,
		confID,
		topic,
		description,
		location,
		vec,
		isBeforePaperDeadline,
		paperDeadline,
		isBeforeAllocationDate,
		allocationDate,
		isBeforeSoftReviewDeadline,
		reviewDeadlineSoft,
		isBeforeHardReviewDeadline,
		reviewDeadlineHard,
		isBeforeDiscussDeadline,
		discussDeadline,
		reviewersPerPaper,
		postWordLimit
		);
	
	return Conference;
}

bool Database::existsConfName(std::string key) throw (const char*)
{
        if (invalid)
                throw (noDB);

	const char* countConf = "SELECT COUNT(*) FROM Conference WHERE name=?";

        sql::PreparedStatement *pstmt = NULL;
        sql::ResultSet * rs = NULL;

        pstmt = dbcon->prepareStatement(countConf);

        int count = 0;
        pstmt->setString(1, key);
        rs = pstmt->executeQuery();

        if (rs->next()) {
                count = rs->getInt(1);
        }
        delete rs;
        delete pstmt;

        return count == 1;
}

void Database::putConf(std::string key, const Conference *conf) throw (const char*)
{
        if (invalid)
                throw (noDB);

        if (this->existsConfName(key))
	{
                this->updateConf(conf);
        }
	else
	{
        	this->createConf(conf;
	}
}

void Database::createConf(const Conference* conf)
{
        if (invalid)
                throw (noDB);

	const char* insertConference = "INSERT INTO Conference(confID, name, topic, description, location, active, paperDeadline, paperDeadlineBool, allocationDate, allocationDateBool, reviewDeadlineSoft, reviewDeadlineSoftBool, reviewDeadlineHard, reviewDeadlineHardBool, discussDeadline, discussDeadlineBool, reviewersPerPaper, postWordlimit) VALUES(NULL, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ? ,?, ?)";

	const char* getconfID = "SELECT LAST_INSERT_ID()";
	const char* insertConfKeyword = "INSERT IGNORE INTO ConferenceKeywords VALUES(?, (SELECT keywordID FROM Keywords WHERE keyword=?))";

        // =======================================
        // Personal Info
        sql::PreparedStatement *pstmt = NULL;
        pstmt = dbcon->prepareStatement(insertConference);

        pstmt->setString(1, conf->title);
        pstmt->setString(2, conf->topic);
        pstmt->setString(3, conf->description);
        pstmt->setString(4, conf->location);
        pstmt->setBoolean(5, conf->isActive);
        pstmt->setDate(6, conf->paperDeadline);
        pstmt->setBoolean(7, conf->isBeforePaperDeadline);
        pstmt->setDate(8, conf->allocationDate);
        pstmt->setBoolean(9, conf->isBeforeAllocationDate);
        pstmt->setDate(10, conf->reviewDeadlineSoft);
        pstmt->setBoolean(11, conf->isBeforeSoftReviewDeadline);
        pstmt->setDate(12, conf->reviewDeadlineHard);
        pstmt->setBoolean(13, conf->isBeforeHardReviewDeadline);
        pstmt->setDate(14, conf->discussDeadline);
        pstmt->setBoolean(15, conf->isBeforeDiscussDeadline);
	pstmt->setInt(16, conf->reviersPerPaper);
	pstmt->setInt(17, conf->postWordLimit);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Store confID
	sql::ResultSet *rs = NULL;
	
	pstmt = dbcon->prepareStatement(getconfID);

	rs = pstmt->executeQuery();
	bool haveRecord = rs->next();

        int confID = rs->getInt(1);

        delete rs;
        delete pstmt;

        // =======================================
        // Expertise
      	if (conf->keywords.size() > 0)
      	{
      		pstmt = dbcon->prepareStatement(insertConfKeyword);
		std::vector<std::string>::const_iterator it;
      		for (it = conf->keywords.begin(); it != conf->keywords.end(); it ++)
		{
			std::string word = *it;	

        		if (!this->existsKeyword(word))
			{
				this->addKeyword(word);
			}
			
			pstmt->setInt(1, confID);
			pstmt->setString(2, word);

      		        pstmt->executeUpdate();
		}
      		delete pstmt;

      	}
}

void Database::updateConf(const Conference* conf)
{
        if (invalid)
                throw (noDB);

	const char* getConfID = "SELECT confID FROM Conference where name = ?";

	const char* updateConference = "UPDATE Conference SET confID=?, name=?, topic=?, description=?, location=?, active=?, paperDeadline=?, paperDeadlineBool=?, allocationDate=?, allocationDateBool=?, reviewDeadlineSoft=?, reviewDeadlineSoftBool=?, reviewDeadlineHard=?, reviewDeadlineHardBool=?, discussDeadline=?, discussDeadlineBool=?, reviewersPerPaper=?, postWordlimit=? WHERE confID=?";

        const char* deleteKeywords = "DELETE FROM ConferenceKeywords WHERE confID=?";
	const char* insertKeywords = "INSERT IGNORE INTO ConferenceKeywords VALUES(?, (SELECT keywordID FROM Keywords WHERE keyword=?))";

        // =======================================
        // Store confID
        sql::PreparedStatement *pstmt = NULL;
	sql::ResultSet *rs = NULL;
	
	pstmt = dbcon->prepareStatement(getConfID);
        pstmt->setString(1, conf->name);

	rs = pstmt->executeQuery();
	bool haveRecord = rs->next();

        int confID = rs->getInt(1);

        delete rs;
        delete pstmt;

        // =======================================
        // Conference
        pstmt = dbcon->prepareStatement(updateConference);

        pstmt->setString(1, conf->title);
        pstmt->setString(2, conf->topic);
        pstmt->setString(3, conf->description);
        pstmt->setString(4, conf->location);
        pstmt->setBoolean(5, conf->isActive);
        pstmt->setDate(6, conf->paperDeadline);
        pstmt->setBoolean(7, conf->isBeforePaperDeadline);
        pstmt->setDate(8, conf->allocationDate);
        pstmt->setBoolean(9, conf->isBeforeAllocationDate);
        pstmt->setDate(10, conf->reviewDeadlineSoft);
        pstmt->setBoolean(11, conf->isBeforeSoftReviewDeadline);
        pstmt->setDate(12, conf->reviewDeadlineHard);
        pstmt->setBoolean(13, conf->isBeforeHardReviewDeadline);
        pstmt->setDate(14, conf->discussDeadline);
        pstmt->setBoolean(15, conf->isBeforeDiscussDeadline);
	pstmt->setInt(16, conf->reviersPerPaper);
	pstmt->setInt(17, conf->postWordLimit);
	pstmt->setInt(18, confID);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Keywords Delete
        pstmt = dbcon->prepareStatement(deleteKeywords);
        pstmt->setInt(1, confID);

        pstmt->executeUpdate();

        delete pstmt;

        // =======================================
        // Keywords Insert
      	if (conf->keywords.size() > 0)
      	{
      		pstmt = dbcon->prepareStatement(insertKeywords);
		std::vector<std::string>::const_iterator it;
      		for (it = conf->keywords.begin(); it != conf->keywords.end(); it ++)
		{
			std::string word = *it;	
        		if (!this->existsKeyword(word))
			{
				this->addKeyword(word);
			}
			pstmt->setInt(1, confID);
			pstmt->setString(2, word);

      		        pstmt->executeUpdate();
		}
      		delete pstmt;

      	}
}

std::vector<int> *Database::allConfIDs()
{
        if (invalid)
                throw (noDB);
        std::vector<int> *vptr = new std::vector<int>();

        const char* selectall = "select confID from Conference";

        sql::PreparedStatement *pstmt = NULL;
        sql::ResultSet * rs = NULL;

        pstmt = dbcon->prepareStatement(selectall);

        rs = pstmt->executeQuery();
        while (rs->next()) {
                int anid = rs->getInt(1);
                vptr->push_back(anid);
        }

        return vptr;
}

std::vector<std::string> *Database::allConfNames()
{
        if (invalid)
                throw (noDB);
        std::vector<std::string> *vptr = new std::vector<std::string>();

        const char* selectall = "select name from Conference";

        sql::PreparedStatement *pstmt = NULL;
        sql::ResultSet * rs = NULL;

        pstmt = dbcon->prepareStatement(selectall);

        rs = pstmt->executeQuery();
        while (rs->next()) {
                std::string aname = rs->getString(1);
                vptr->push_back(aname);
        }

        return vptr;
}

//
//std::vector<MyRecord*> *Database::getInRole(const char* role) throw (const char*)
//{
//        if (invalid)
//                throw (noDB);
//        // Laboured
//        // First get a list of the ids with the role, then one by one get the
//        // complete records.
//        const char* getrolepersons = "selects personid from roles where _role=?";
//
//        sql::PreparedStatement *pstmt = NULL;
//        sql::ResultSet * rs = NULL;
//
//        pstmt = dbcon->prepareStatement(getrolepersons);
//        pstmt->setString(1, role);
//        rs = pstmt->executeQuery();
//
//        bool haveRecord = rs->next();
//        if (!haveRecord)
//        {
//                delete rs;
//                delete pstmt;
//                return NULL;
//        }
//
//        std::vector<std::string> people;
//
//        // rc = sqlite3_step(stmt);
//
//        while (rs->next()) {
//                std::string apersonid = std::string(rs->getString(1));
////              const char* apersonid = reinterpret_cast<const char*> (rs->getString(2));
//                people.push_back(apersonid);
//        }
//
//        // Maybe there weren't any
//        if (people.size() == 0)
//                return NULL;
//
//        // Build collection by getting each record
//        std::vector<MyRecord*> *roleholders = new std::vector<MyRecord*>();
//
//        std::vector<std::string>::const_iterator it;
//        for (it = people.begin(); it != people.end(); it++) {
//                std::string aperson = *it;
//                MyRecord* rec = this->get(aperson.c_str());
//                roleholders->push_back(rec);
//        }
//
//        return roleholders;
//}
//

//bool Database::deleteUser(int* key) throw (const char*)
//{
//        if (invalid)
//                throw (noDB);
//        // explicitly delete subordinate records in Phones, Address, others and Roles
//        // then delete the myrecord entry
//        // recreating the prepared statements for each operation - costly
//        const char* deletephones = "delete from phones where personid=?";
//        const char* deleteaddress = "delete from addresses where personid=?";
//        const char* deleteother = "delete from other where personid=?";
//        const char* deleteroles = "delete from roles where personid=?";
//        const char* deletemyrecord = "delete from myrecord where _id=?";
//        const char* unused; // Pointer to unused part of sql string (?)
//        sql::PreparedStatement *pstmt = NULL;
//        // Phones
//        pstmt = dbcon->prepareStatement(deletephones);
//        pstmt->setString(1, key);
//        pstmt->executeUpdate();
//        delete pstmt;
//
//        // Addresses
//        pstmt = dbcon->prepareStatement(deleteaddress);
//        pstmt->setString(1, key);
//        pstmt->executeUpdate();
//        delete pstmt;
//
//        //Other
//        pstmt = dbcon->prepareStatement(deleteother);
//        pstmt->setString(1, key);
//        pstmt->executeUpdate();
//        delete pstmt;
//
//        // Roles
//        pstmt = dbcon->prepareStatement(deleteroles);
//        pstmt->setString(1, key);
//        pstmt->executeUpdate();
//        delete pstmt;
//
//        // Finally
//        pstmt = dbcon->prepareStatement(deletemyrecord);
//        pstmt->setString(1, key);
//        pstmt->executeUpdate();
//        delete pstmt;
//
//        return true;
//}
//
//
//MyRecord *Database::get(const char* key) throw (const char*)
//{
//        if (invalid)
//                throw (noDB);
//        // Again laborious
//        // Load the MyRecord from its table, then add data from Other tables.
//        const char* getrecord = "select * from myrecord where _id=?";
//        const char* getroles = "select role from roles where personid=?";
//        const char* getphones = "select type, number from phones where personid=?";
//        const char* getaddress = "select type, address from addresses where personid=?";
//        const char* getother = "select key, valyue from other where personid=?";
//
//        sql::PreparedStatement *pstmt = NULL;
//        sql::ResultSet *rs = NULL;
//
//        pstmt = dbcon->prepareStatement(getrecord);
//        pstmt->setString(1, key);
//        rs = pstmt->executeQuery();
//        bool haveRecord = rs->next();
//        if (!haveRecord)
//        {
//                delete rs;
//                delete pstmt;
//                return NULL;
//        }
//
//        std::string id = rs->getString(1);
//        std::string name = rs->getString(2);
//        std::string email = rs->getString(3);
//        std::string info = rs->getString(4);
//        std::string image = rs->getString(5);
//
//        MyRecord *arec = new MyRecord(id);
//        arec->setName(name);
//        arec->setEmail(email);
//        arec->setInfo(info);
//        arec->setImage(image);
//
//        delete rs;
//        delete pstmt;
//        // =======================================
//        // Now ask about roles
//        pstmt = dbcon->prepareStatement(getroles);
//        pstmt->setString(1, key);
//        rs = pstmt->executeQuery();
//        while (rs->next())
//        {
//                std::string arole = rs->getString(1);
//                arec->addRole(arole);
//        }
//        delete rs;
//        delete pstmt;
//}
//
//void Database::recordToTables(const MyRecord* data)
//{
//        if (invalid)
//                throw (noDB);
//
//        // Insert the myrecord data first, then deal with Other data tables
//
//        const char* putmyrecord = "insert into myrecord values( ?, ?, ?, ?, ?)";
//        const char* putrole = "insert into roles values(?, ?)";
//        const char* putphone = "insert into phones values (?, ?, ?)";
//        const char* putaddress = "insert into addresses values (?, ?, ?)";
//        const char* putother = "insert into other values (?, ?, ?)";
//
//        sql::PreparedStatement *pstmt = NULL;
//        pstmt = dbcon->prepareStatement(putmyrecord);
//        pstmt->setString(1, data->getID());
//        pstmt->setString(2, data->getName());
//        pstmt->setString(3, data->getEmail());
//        pstmt->setString(4, data->getInfo());
//        pstmt->setString(5, data->getImage());
//
//        pstmt->executeUpdate();
//
//        delete pstmt;
//
//        // Role 
////      if (data->getOtherKV().size() > 0)
////      {
////              pstmt = dbcon->prepareStatement(putother);
////              std::map<std::string, std::string>::const_iterator it;
////              for (it = data->getRoles().begin(); it != data->getRoles().end(); it ++)
////              {
////                      std::string akey = it->first;
////                      std::string avalue = it->second;
////                      pstmt->setString(1, data->getID());
////                      pstmt->setString(2, akey);
////                      pstmt->setString(3, avalue);
////                      pstmt->executeUpdate();
////              }
////              delete pstmt;
////
////      }
//
////      // Other
////      if (data->getOtherKV().size() > 0)
////      {
////              pstmt = dbcon->prepareStatement(putother);
////              std::map<std::string, std::string>::const_iterator it;
////              for (it = data->getOtherKV.begin(); it != data->getOtherKV().end(); it ++)
////              {
////                      std::string akey = it->first;
////                      std::string avalue = it->second;
////                      pstmt->setString(1, data->getID());
////                      pstmt->setString(2, akey);
////                      pstmt->setString(3, avalue);
////                      pstmt->executeUpdate();
////              }
////              delete pstmt;
////
////      }
//}
