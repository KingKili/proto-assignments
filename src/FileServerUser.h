#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include "Filesystem.h"
#include "Logger.h"
#include "FileServerClient.h"

class FileServerUser
{
public:
  const std::string USER_NAME;
  const std::string PASSWORD;
  FilesystemServer *fS;

  FileServerUser(std::string userName, std::string password);
  FileServerUser();
  ~FileServerUser();

  FileServerClient *getClient(unsigned int clientId);
  void addClient(FileServerClient *client);

private:
  std::unordered_map<unsigned int, FileServerClient *> clients;
  std::mutex *clientsMutex;

  void deleteAllClients();
};