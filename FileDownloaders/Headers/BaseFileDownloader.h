//
// Created by Petr Flajsingr on 19/10/2018.
//

#ifndef FILEDOWNLOADERS_HEADERS_BASEFILEDOWNLOADER_H_
#define FILEDOWNLOADERS_HEADERS_BASEFILEDOWNLOADER_H_

#include <vector>
#include <string>
#include "Exceptions.h"

class BaseFileDownloader {
 private:

 protected:
  std::vector<std::string> availableFiles;

 public:
  virtual ~BaseFileDownloader() = default;

  virtual std::vector<std::string> getAvailableFiles() {
    return availableFiles;
  }

  virtual void downloadFile(const std::string &fileName) {
    auto result = std::find(availableFiles.begin(),
        availableFiles.end(),
        fileName);

    if (result == availableFiles.end()) {
      throw InvalidArgumentException(
          ("The file " + fileName + " is not available").c_str());
    }

    downloadFile(result - availableFiles.begin());
  }

  virtual void downloadFile(size_t fileIndex) = 0;
};

#endif  //FILEDOWNLOADERS_HEADERS_BASEFILEDOWNLOADER_H_