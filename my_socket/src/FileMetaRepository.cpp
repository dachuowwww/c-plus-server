#include "FileMetaRepository.h"

#include <cppconn/driver.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "RedisClient.h"

namespace {
constexpr int kFileMetaCacheTtlSeconds = 120;

std::vector<std::string> SplitLines(const std::string &raw) {
  std::vector<std::string> parts;
  size_t begin = 0;
  while (begin <= raw.size()) {
    const size_t end = raw.find('\n', begin);
    if (end == std::string::npos) {
      parts.push_back(raw.substr(begin));
      break;
    }
    parts.push_back(raw.substr(begin, end - begin));
    begin = end + 1;
  }
  return parts;
}
}  // namespace

FileMetaRepository *FileMetaRepository::instance_ = nullptr;

void FileMetaRepository::Init(std::string host, int port, std::string user, std::string password, std::string database,
                              int connect_timeout_sec, int read_timeout_sec, int write_timeout_sec,
                              int query_timeout_sec) {
  if (instance_ == nullptr) {
    instance_ = new FileMetaRepository(std::move(host), port, std::move(user), std::move(password), std::move(database),
                                       connect_timeout_sec, read_timeout_sec, write_timeout_sec, query_timeout_sec);
  }
}

FileMetaRepository &FileMetaRepository::Instance() {
  if (instance_ == nullptr) {
    throw std::runtime_error("FileMetaRepository 未初始化");
  }
  return *instance_;
}

FileMetaRepository::FileMetaRepository(std::string host, int port, std::string user, std::string password,
                                       std::string database, int connect_timeout_sec, int read_timeout_sec,
                                       int write_timeout_sec, int query_timeout_sec)
    : host_(std::move(host)),
      port_(port),
      user_(std::move(user)),
      password_(std::move(password)),
      database_(std::move(database)),
      connect_timeout_sec_(connect_timeout_sec),
      read_timeout_sec_(read_timeout_sec),
      write_timeout_sec_(write_timeout_sec),
      query_timeout_sec_(query_timeout_sec) {}

bool FileMetaRepository::CreateFileMeta(int64_t user_id, const std::string &filename, const std::string &filepath,
                                        int64_t filesize, const std::string &filetype, const std::string &sha256/*,
                                        int64_t *file_id*/) {
  if (user_id <= 0 || filename.empty() || filepath.empty()) {
    return false;
  }

  sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
  sql::ConnectOptionsMap options;
  options["hostName"] = host_;
  options["port"] = port_;
  options["userName"] = user_;
  options["password"] = password_;
  options["OPT_CONNECT_TIMEOUT"] = connect_timeout_sec_;
  options["OPT_READ_TIMEOUT"] = read_timeout_sec_;
  options["OPT_WRITE_TIMEOUT"] = write_timeout_sec_;

  std::unique_ptr<sql::Connection> conn(driver->connect(options));
  conn->setSchema(database_);

  std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement(
      "INSERT INTO files (user_id, filename, filepath, filesize, filetype, sha256) VALUES (?, ?, ?, ?, ?, ?)"));
  TrySetQueryTimeout(stmt.get(), query_timeout_sec_);
  stmt->setInt64(1, user_id);
  stmt->setString(2, filename);
  stmt->setString(3, filepath);
  stmt->setInt64(4, filesize);
  stmt->setString(5, filetype);
  stmt->setString(6, sha256);
  if (stmt->executeUpdate() <= 0) {
    return false;
  }

  // std::unique_ptr<sql::Statement> id_stmt(conn->createStatement());
  // TrySetQueryTimeout(id_stmt.get(), query_timeout_sec_);
  // std::unique_ptr<sql::ResultSet> rs(id_stmt->executeQuery("SELECT LAST_INSERT_ID() AS id"));
  // if (!rs->next()) {
  //   return false;
  // }
  // const int64_t new_id = rs->getInt64("id");
  // if (file_id != nullptr) {
  //   *file_id = new_id;
  // }
  return true;
}

bool FileMetaRepository::GetFileMetaById(int64_t file_id, FileMeta *meta) {  // 主键查询
  if (file_id <= 0 || meta == nullptr) {
    return false;
  }

  const std::string cache_key = "file:meta:" + std::to_string(file_id);
  std::string cache_value;
  const RedisClient::GetResult redis_status = RedisClient::Instance().GetWithStatus(cache_key, &cache_value);
  if (redis_status == RedisClient::GetResult::kHit && DecodeCacheValue(cache_value, meta)) {
    return true;
  }
  // 缓存未命中或解析失败，查询数据库
  FileMeta db_meta;
  if (!QueryFileMetaById(file_id, &db_meta)) {
    return false;
  }
  *meta = db_meta;
  // 将元数据结果写入缓存
  if (redis_status != RedisClient::GetResult::kError) {
    RedisClient::Instance().SetEx(cache_key, kFileMetaCacheTtlSeconds, EncodeCacheValue(db_meta));
  }
  return true;
}

bool FileMetaRepository::GetFileMetaByFilename(const std::string &filename, FileMeta *meta) {  // 二级查询
  if (filename.empty() || meta == nullptr) {
    return false;
  }

  sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
  sql::ConnectOptionsMap options;
  options["hostName"] = host_;
  options["port"] = port_;
  options["userName"] = user_;
  options["password"] = password_;
  options["OPT_CONNECT_TIMEOUT"] = connect_timeout_sec_;
  options["OPT_READ_TIMEOUT"] = read_timeout_sec_;
  options["OPT_WRITE_TIMEOUT"] = write_timeout_sec_;

  std::unique_ptr<sql::Connection> conn(driver->connect(options));
  conn->setSchema(database_);

  std::unique_ptr<sql::PreparedStatement> stmt(
      conn->prepareStatement("SELECT id, user_id, filename, filepath, filesize, filetype, sha256, "
                             "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at, "
                             "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') AS updated_at "
                             "FROM files WHERE filename = ? ORDER BY id DESC LIMIT 1"));
  TrySetQueryTimeout(stmt.get(), query_timeout_sec_);
  stmt->setString(1, filename);

  std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
  if (!rs->next()) {
    return false;
  }
  return FillFileMetaFromResult(rs.get(), meta);
}

std::vector<FileMeta> FileMetaRepository::ListLatestFiles(int limit) {
  if (limit <= 0) {
    return {};
  }

  sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
  sql::ConnectOptionsMap options;
  options["hostName"] = host_;
  options["port"] = port_;
  options["userName"] = user_;
  options["password"] = password_;
  options["OPT_CONNECT_TIMEOUT"] = connect_timeout_sec_;
  options["OPT_READ_TIMEOUT"] = read_timeout_sec_;
  options["OPT_WRITE_TIMEOUT"] = write_timeout_sec_;

  std::unique_ptr<sql::Connection> conn(driver->connect(options));
  conn->setSchema(database_);

  std::unique_ptr<sql::PreparedStatement> stmt(
      conn->prepareStatement("SELECT id, user_id, filename, filepath, filesize, filetype, sha256, "
                             "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at, "
                             "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') AS updated_at "
                             "FROM files ORDER BY id DESC LIMIT ?"));
  TrySetQueryTimeout(stmt.get(), query_timeout_sec_);
  stmt->setInt(1, limit);

  std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
  std::vector<FileMeta> files;
  while (rs->next()) {
    FileMeta meta;
    if (FillFileMetaFromResult(rs.get(), &meta)) {
      files.push_back(meta);
    }
  }
  return files;
}

bool FileMetaRepository::DeleteFileMetaById(int64_t file_id) {
  if (file_id <= 0) {
    return false;
  }

  sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
  sql::ConnectOptionsMap options;
  options["hostName"] = host_;
  options["port"] = port_;
  options["userName"] = user_;
  options["password"] = password_;
  options["OPT_CONNECT_TIMEOUT"] = connect_timeout_sec_;
  options["OPT_READ_TIMEOUT"] = read_timeout_sec_;
  options["OPT_WRITE_TIMEOUT"] = write_timeout_sec_;

  std::unique_ptr<sql::Connection> conn(driver->connect(options));
  conn->setSchema(database_);

  std::unique_ptr<sql::PreparedStatement> stmt(conn->prepareStatement("DELETE FROM files WHERE id = ?"));
  TrySetQueryTimeout(stmt.get(), query_timeout_sec_);
  stmt->setInt64(1, file_id);
  const bool deleted = stmt->executeUpdate() > 0;
  if (deleted) {
    InvalidateFileMetaCache(file_id);
  }
  return deleted;
}

void FileMetaRepository::InvalidateFileMetaCache(int64_t file_id) {
  if (file_id <= 0) {
    return;
  }
  RedisClient::Instance().Del("file:meta:" + std::to_string(file_id));
}

bool FileMetaRepository::QueryFileMetaById(int64_t file_id, FileMeta *meta) {
  if (file_id <= 0 || meta == nullptr) {
    return false;
  }

  sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
  sql::ConnectOptionsMap options;
  options["hostName"] = host_;
  options["port"] = port_;
  options["userName"] = user_;
  options["password"] = password_;
  options["OPT_CONNECT_TIMEOUT"] = connect_timeout_sec_;
  options["OPT_READ_TIMEOUT"] = read_timeout_sec_;
  options["OPT_WRITE_TIMEOUT"] = write_timeout_sec_;

  std::unique_ptr<sql::Connection> conn(driver->connect(options));
  conn->setSchema(database_);

  std::unique_ptr<sql::PreparedStatement> stmt(
      conn->prepareStatement("SELECT id, user_id, filename, filepath, filesize, filetype, sha256, "
                             "DATE_FORMAT(created_at, '%Y-%m-%d %H:%i:%s') AS created_at, "
                             "DATE_FORMAT(updated_at, '%Y-%m-%d %H:%i:%s') AS updated_at "
                             "FROM files WHERE id = ? LIMIT 1"));
  TrySetQueryTimeout(stmt.get(), query_timeout_sec_);
  stmt->setInt64(1, file_id);

  std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());
  if (!rs->next()) {
    return false;
  }
  return FillFileMetaFromResult(rs.get(), meta);
}

bool FileMetaRepository::FillFileMetaFromResult(sql::ResultSet *result_set, FileMeta *meta) const {
  if (result_set == nullptr || meta == nullptr) {
    return false;
  }
  meta->id = result_set->getInt64("id");
  meta->user_id = result_set->getInt64("user_id");
  meta->filename = result_set->getString("filename");
  meta->filepath = result_set->getString("filepath");
  meta->filesize = result_set->getInt64("filesize");
  meta->filetype = result_set->getString("filetype");
  meta->sha256 = result_set->getString("sha256");
  meta->created_at = result_set->getString("created_at");
  meta->updated_at = result_set->getString("updated_at");
  return meta->id > 0;
}

std::string FileMetaRepository::EncodeCacheValue(const FileMeta &meta) const {
  return std::to_string(meta.id) + "\n" + std::to_string(meta.user_id) + "\n" + meta.filename + "\n" + meta.filepath +
         "\n" + std::to_string(meta.filesize) + "\n" + meta.filetype + "\n" + meta.sha256 + "\n" + meta.created_at +
         "\n" + meta.updated_at;
}

bool FileMetaRepository::DecodeCacheValue(const std::string &raw, FileMeta *meta) const {
  if (meta == nullptr) {
    return false;
  }
  const std::vector<std::string> parts = SplitLines(raw);
  if (parts.size() < 9) {
    return false;
  }

  try {
    meta->id = std::stoll(parts[0]);
    meta->user_id = std::stoll(parts[1]);
    meta->filesize = std::stoll(parts[4]);
  } catch (...) {
    return false;
  }
  meta->filename = parts[2];
  meta->filepath = parts[3];
  meta->filetype = parts[5];
  meta->sha256 = parts[6];
  meta->created_at = parts[7];
  meta->updated_at = parts[8];
  return meta->id > 0;
}
