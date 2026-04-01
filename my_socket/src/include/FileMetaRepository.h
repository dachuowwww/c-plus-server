#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Macro.h"

namespace sql {
class ResultSet;
}

struct FileMeta {
  int64_t id = -1;
  int64_t user_id = -1;
  std::string filename;
  std::string filepath;
  int64_t filesize = 0;
  std::string filetype;
  std::string sha256;
  std::string created_at;
  std::string updated_at;
};

class FileMetaRepository {
 public:
  static void Init(std::string host, int port, std::string user, std::string password, std::string database,
                   int connect_timeout_sec = 1, int read_timeout_sec = 1, int write_timeout_sec = 1,
                   int query_timeout_sec = 1);
  static FileMetaRepository &Instance();

  bool CreateFileMeta(int64_t user_id, const std::string &filename, const std::string &filepath, int64_t filesize,
                      const std::string &filetype, const std::string &sha256 /*, int64_t *file_id*/);
  bool GetFileMetaById(int64_t file_id, FileMeta *meta);
  bool GetFileMetaByFilename(const std::string &filename, FileMeta *meta);
  std::vector<FileMeta> ListLatestFiles(int limit);
  bool DeleteFileMetaById(int64_t file_id);
  void InvalidateFileMetaCache(int64_t file_id);
  template <typename StatementType>
  void TrySetQueryTimeout(StatementType *stmt, int query_timeout_sec);

 private:
  FileMetaRepository(std::string host, int port, std::string user, std::string password, std::string database,
                     int connect_timeout_sec, int read_timeout_sec, int write_timeout_sec, int query_timeout_sec);

  bool QueryFileMetaById(int64_t file_id, FileMeta *meta);
  bool FillFileMetaFromResult(sql::ResultSet *result_set, FileMeta *meta) const;
  std::string BuildCacheKey(int64_t file_id) const;
  std::string EncodeCacheValue(const FileMeta &meta) const;
  bool DecodeCacheValue(const std::string &raw, FileMeta *meta) const;

  static FileMetaRepository *instance_;

  std::string host_;
  int port_;
  std::string user_;
  std::string password_;
  std::string database_;
  int connect_timeout_sec_;
  int read_timeout_sec_;
  int write_timeout_sec_;
  int query_timeout_sec_;

  DISALLOW_COPY_AND_ASSIGN(FileMetaRepository);
};

template <typename StatementType>
void FileMetaRepository::TrySetQueryTimeout(StatementType *stmt, int query_timeout_sec) {
  if (stmt == nullptr || query_timeout_sec <= 0) {
    return;
  }
  try {
    stmt->setQueryTimeout(static_cast<unsigned int>(query_timeout_sec));
  } catch (...) {
    // 某些 mysql cpp 驱动不支持 setQueryTimeout，退化为连接/读写超时。
  }
}
