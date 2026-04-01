#include "Config.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <stdexcept>

AppConfig LoadConfigFromYaml(const std::string &path) {
  YAML::Node root = YAML::LoadFile(path);

  AppConfig cfg;

  if (root["server"]) {
    auto server = root["server"];
    if (server["host"]) cfg.server.host = server["host"].as<std::string>();
    if (server["port"]) cfg.server.port = server["port"].as<int>();
    if (server["instance_name"]) {
      cfg.server.instance_name = server["instance_name"].as<std::string>();
    }
  }

  if (root["log"]) {
    auto log = root["log"];
    if (log["dir"]) cfg.log.dir = log["dir"].as<std::string>();
    if (log["level"]) cfg.log.level = log["level"].as<std::string>();
  }

  if (root["redis"]) {
    auto redis = root["redis"];
    if (redis["host"]) cfg.redis.host = redis["host"].as<std::string>();
    if (redis["port"]) cfg.redis.port = redis["port"].as<int>();
    if (redis["password"]) cfg.redis.password = redis["password"].as<std::string>();
    if (redis["db"]) cfg.redis.db = redis["db"].as<int>();
    if (redis["connect_timeout_ms"]) cfg.redis.connect_timeout_ms = redis["connect_timeout_ms"].as<int>();
    if (redis["command_timeout_ms"]) cfg.redis.command_timeout_ms = redis["command_timeout_ms"].as<int>();
  }

  if (root["mysql"]) {
    auto mysql = root["mysql"];
    if (mysql["host"]) cfg.mysql.host = mysql["host"].as<std::string>();
    if (mysql["port"]) cfg.mysql.port = mysql["port"].as<int>();
    if (mysql["user"]) cfg.mysql.user = mysql["user"].as<std::string>();
    if (mysql["password"]) cfg.mysql.password = mysql["password"].as<std::string>();
    if (mysql["database"]) cfg.mysql.database = mysql["database"].as<std::string>();
    if (mysql["connect_timeout_sec"]) cfg.mysql.connect_timeout_sec = mysql["connect_timeout_sec"].as<int>();
    if (mysql["read_timeout_sec"]) cfg.mysql.read_timeout_sec = mysql["read_timeout_sec"].as<int>();
    if (mysql["write_timeout_sec"]) cfg.mysql.write_timeout_sec = mysql["write_timeout_sec"].as<int>();
    if (mysql["query_timeout_sec"]) cfg.mysql.query_timeout_sec = mysql["query_timeout_sec"].as<int>();
  }

  if (root["feature"]) {
    auto feature = root["feature"];
    if (feature["enable_hot_cache"]) {
      cfg.feature.enable_hot_cache = feature["enable_hot_cache"].as<bool>();
    }
    if (feature["enable_gray"]) {
      cfg.feature.enable_gray = feature["enable_gray"].as<bool>();
    }
    if (feature["rpc_timeout_ms"]) {
      cfg.feature.rpc_timeout_ms = feature["rpc_timeout_ms"].as<int>();
    }
    if (feature["guest_browse_rate_limit_per_min"]) {
      cfg.feature.guest_browse_rate_limit_per_min = feature["guest_browse_rate_limit_per_min"].as<int>();
    }
    if (feature["api_list_rate_limit_per_min"]) {
      cfg.feature.api_list_rate_limit_per_min = feature["api_list_rate_limit_per_min"].as<int>();
      if (!feature["guest_browse_rate_limit_per_min"]) {
        cfg.feature.guest_browse_rate_limit_per_min = cfg.feature.api_list_rate_limit_per_min;
      }
    }
  }

  return cfg;
}
