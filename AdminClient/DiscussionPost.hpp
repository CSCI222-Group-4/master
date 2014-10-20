#ifndef DISCUSSION_POST_H
#define DISCUSSION_POST_H
#include <string>

struct DiscussionPost {
  DiscussionPost(const std::string& icomment,
                 const std::string& ireviewerUName,
                 int ipostID):
                 comment(icomment),
                 reviewerUName(ireviewerUName),
                 postID(ipostID)
                 { }
  std::string comment{""};
  std::string reviewerUName{""};
  int postID{0};
};
#endif
