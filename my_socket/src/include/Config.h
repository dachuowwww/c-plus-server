#pragma once
#include <string>

struct ServerConfig {
  std::string host = "0.0.0.0";
  int port = 8080;
  std::string instance_name = "default";
};

struct LogConfig {
  std::string dir = "./logs";
  std::string level = "info";  // 保留
};

struct RedisConfig {
  std::string host = "127.0.0.1";
  int port = 6379;
  std::string password;
  int db = 0;
  int connect_timeout_ms = 100;
  int command_timeout_ms = 100;
};

struct MySQLConfig {
  std::string host = "127.0.0.1";
  int port = 3306;
  std::string user = "root";
  std::string password;
  std::string database;
  int connect_timeout_sec = 1;
  int read_timeout_sec = 1;
  int write_timeout_sec = 1;
  int query_timeout_sec = 1;
};

struct FeatureConfig {
  bool enable_hot_cache = true;
  bool enable_gray = false;
  int rpc_timeout_ms = 2000;
  int guest_browse_rate_limit_per_min = 120;
  int api_list_rate_limit_per_min = 120;
};

struct AppConfig {
  ServerConfig server;
  LogConfig log;
  RedisConfig redis;
  MySQLConfig mysql;
  FeatureConfig feature;
};

AppConfig LoadConfigFromYaml(const std::string &path);
