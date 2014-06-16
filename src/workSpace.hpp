#ifndef WORK_SPACE_HPP
#define WORK_SPACE_HPP

#include <string>

// set up temporary work space to store unique implementations
// and empricial testing needs
int setUpWorkSpace(std::string &out_file_name, std::string &fileName,
                   std::string &path, std::string &tmpPath, std::string &pathToTop,
                   bool deleteTmp);

// move best version to friendly location and report
void handleBestVersion(std::string &fileName, std::string &path,
                       std::string &tmpPath, int bestVersion);

#endif  // WORK_SPACE_HPP
