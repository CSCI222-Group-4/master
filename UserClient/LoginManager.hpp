//incomplete
#include "Conference.hpp"

Class LoginManager {
  public:
  LoginManager(bool iloggedIn,
               const std::vector<Conference>& iconferences,
               Conference* iactiveConference) :
               loggedIn(iloggedIn),
               conferences(iconferences),
               activeConference(iactiveConference) // potentially useful for logging back into the conference they were last in
               { }
  ~LoginManager() {
  for (unsigned i=0; i < conferences.size(); i++) {
    delete conferences[i];
  }
}
// no other constructors deemed necessary at this point
  bool login(std::string, std::string)
  logout()
  createAccount()
  listConferences()
  viewActiveConfDetails()
  Conference* getActiveConference() {return activeConference;}
  std::vector<Conference> getConferences() {return conferences;}
  void setActiveConference(Conference* iactiveConference) {activeConference =  iactiveConference;}
  void setConferences(const std::vector<Conference>& iconferences) {conferences =  iconferences;}

  bool loggedIn;
  private:
  std::vector<Conference> conferences;
  Conference* activeConference;
};