void PaperManager::fetchPaper(int paperID)
{
  *currentPaper = database->fetchPaper(paperID);
}

void PaperManager::sendPaper(int paperID)
{
  fetchPaper(paperID);
 //send *currentPaper to the client
}

void PaperManager::addPaper()
{
  database->createPaper(*currentPaper);
}

void PaperManager::modifyPaper()
{
  database->updatePaper(*currentPaper);
}

void PaperManager::sendPaperSummaryBatch(int confID)
{
  std::vector<int> paperIDsOfConf = database->getPaperIDsForConf(confID);
  //returns all paperSummaries using that confID
  std::vector<PaperSummary> papersOfConf;
  for(unsigned int i = 0; i < paperIDsOfConf.size(); ++i)
  {
    papersOfConf[i] = fetchPaperSummary(paperIDsOfConf[i]);
  }
  //send papersOfConf to the client
}

void PaperManager::sendPaperSummariesToReview(int userID, int confID)
{
 std::vector<int> paperIDsToReview = database->getPaperIDsForAllocatedReviewer(userID, confID); //or something like that
 std::vector<PaperSummary> papersToReview;
 for(unsigned int i = 0; i < paperIDsToReview.size(); ++i)
 {
   papersToReview[i] = fetchPaperSummary(paperIDsToReview[i]);
 }
  //send papersToReview to the client
}

void PaperManager::sendAuthoredPaperSummaries(int leadAuthorID, int confID)
{
 std::vector<int> paperIDsAuthored = database->fetchPapersFromLeadAuthorIDAndConfID(leadAuthorID, confID); 
 //or something like that
 std::vector<PaperSummary> papersAuthored;
 for(unsigned int i = 0; i < paperIDsAuthored.size(); ++i)
 {
   papersAuthored[i] = fetchPaperSummary(paperIDsAuthored[i]);
 }
  //send papersAuthored to the client
}



