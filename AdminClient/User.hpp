#ifndef USER_H
#define USER_H
#include <string>
#include <vector>

struct User 
{
    User() { }

    User(const std::string& iuserName,
        const std::string& iname,
        const std::string& iemail,
        const std::string& iorganisation,
        const std::string& iphone,
        const std::string& ipassword,
        int iuserID,
        const std::vector<std::string>& ikeywords)
    {
        userName = iuserName;
        name = iname;
        email = iemail;
        organisation = iorganisation;
        phone = iphone;
        password = ipassword;
        userID = iuserID;
        keywords = ikeywords;
    }

    std::string userName;
    std::string password;
    std::string name;
    std::string email;
    std::string organisation;
    std::string phone;
    std::vector<std::string> keywords;
    int userID = 0;
};
#endif
