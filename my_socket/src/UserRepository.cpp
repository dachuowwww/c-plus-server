#include "UserRepository.h"
#include <cppconn/exception.h>           // SQL 异常类
#include <cppconn/prepared_statement.h>  // 预处理语句类,写sql语句
#include <cppconn/resultset.h>           // 结果集类
#include <mysql_connection.h>            // 数据库连接
#include <mysql_driver.h>                // MySQL 驱动对象
#include <memory>
#include <utility>

UserRepository::UserRepository(std::string host, int port, std::string user, std::string password, std::string database)
    : url_("tcp://" + std::move(host) + ":" + std::to_string(port)),
      user_(std::move(user)),
      password_(std::move(password)),
      database_(std::move(database)) {}

bool UserRepository::VerifyPlainPassword(const std::string &username, const std::string &password) const {
  sql::mysql::MySQL_Driver *driver = sql::mysql::get_driver_instance();
  std::unique_ptr<sql::Connection> conn(driver->connect(url_, user_, password_));
  conn->setSchema(database_);  // 选择数据库

  std::unique_ptr<sql::PreparedStatement> stmt(
      conn->prepareStatement("SELECT password FROM users WHERE username = ? AND is_deleted = 0 LIMIT 1"));
  stmt->setString(1, username);                              // 绑定参数
  std::unique_ptr<sql::ResultSet> rs(stmt->executeQuery());  // 执行并返回结果
  if (!rs->next()) {
    return false;
  }

  const std::string db_password = rs->getString("password");
  return db_password == password;
}
