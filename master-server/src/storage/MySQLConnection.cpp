/**
 * @file   MySQLConnection.cpp
 * @brief  MySQL 业务库连接管理实现
 * @module master-server
 */

#include "storage/MySQLConnection.hpp"

#include <iostream>
#include <cstring>

#include <mysql.h>

namespace master {
namespace storage {

MySQLConnection& MySQLConnection::instance()
{
    static MySQLConnection conn;
    return conn;
}

MySQLConnection::MySQLConnection()
    : mysql_(nullptr)
    , initialized_(false)
{
}

MySQLConnection::~MySQLConnection()
{
    std::lock_guard<std::mutex> lock(mutex_);
    disconnect();
}

bool MySQLConnection::initialize(const MySQLConfig& config)
{
    std::lock_guard<std::mutex> lock(mutex_);

    config_ = config;
    initialized_ = true;

    // 尝试连接
    if (!connect()) {
        std::cerr << "[master-server] MySQL 初始化连接失败\n";
        // 不返回 false，允许之后自动重连
    }

    return true;
}

bool MySQLConnection::isValid() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !mysql_) {
        return false;
    }

    // 使用 ping 检查连接
    if (mysql_ping(mysql_) == 0) {
        return true;
    } else {
        std::cerr << "[master-server] MySQL ping 失败: "
                  << mysql_error(mysql_) << '\n';
        return false;
    }
}

MYSQL* MySQLConnection::getRaw() const
{
    return mysql_;
}

bool MySQLConnection::connect()
{
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }

    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        std::cerr << "[master-server] MySQL 初始化失败\n";
        return false;
    }

    // 设置连接超时
    unsigned int timeout = config_.connect_timeout;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    mysql_options(mysql_, MYSQL_OPT_READ_TIMEOUT, &timeout);
    mysql_options(mysql_, MYSQL_OPT_WRITE_TIMEOUT, &timeout);

    // 启用自动重连
    my_bool reconnect = 1;
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect);

    // 完全禁用 SSL（使用正确的 enum 类型）
    // SSL_MODE_DISABLED 值为 1，来自 mysql.h 中的 enum mysql_ssl_mode { SSL_MODE_DISABLED=1, ... }
    enum mysql_ssl_mode ssl_mode = SSL_MODE_DISABLED;
    mysql_options(mysql_, MYSQL_OPT_SSL_MODE, &ssl_mode);

    // 连接到服务器
    if (!mysql_real_connect(
            mysql_,
            config_.host.c_str(),
            config_.username.c_str(),
            config_.password.c_str(),
            config_.database.c_str(),
            config_.port,
            nullptr,
            CLIENT_NO_SCHEMA | CLIENT_FOUND_ROWS
        )) {
        std::cerr << "[master-server] MySQL 连接失败 ("
                  << config_.host << ":" << config_.port
                  << "/" << config_.database << "): "
                  << mysql_error(mysql_) << '\n';
        mysql_close(mysql_);
        mysql_ = nullptr;
        return false;
    }

    // 设置字符集为 utf8mb4
    if (mysql_set_character_set(mysql_, "utf8mb4") != 0) {
        std::cerr << "[master-server] MySQL 设置字符集失败: "
                  << mysql_error(mysql_) << '\n';
    }

    std::cout << "[master-server] MySQL 连接成功 ("
              << config_.host << ":" << config_.port
              << "/" << config_.database << ")\n";

    return true;
}

void MySQLConnection::disconnect()
{
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
        std::cout << "[master-server] MySQL 连接已关闭\n";
    }
}

int MySQLConnection::execute(const std::string& sql)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "[master-server] MySQL 未初始化\n";
        return -1;
    }

    // 确保连接可用
    if (!mysql_ || mysql_ping(mysql_) != 0) {
        std::cout << "[master-server] MySQL 尝试重连...\n";
        if (!connect()) {
            return -1;
        }
    }

    std::cout << "[master-server] MySQL execute SQL: " << sql << '\n';
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        std::cerr << "[master-server] MySQL 执行失败: "
                  << mysql_error(mysql_) << '\n';
        return -1;
    }

    int affectedRows = static_cast<int>(mysql_affected_rows(mysql_));
    std::cout << "[master-server] MySQL execute affected rows: "
              << affectedRows << '\n';
    return affectedRows;
}

bool MySQLConnection::queryOne(const std::string& sql, std::function<bool(MYSQL*)> row_handler)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "[master-server] MySQL 未初始化\n";
        return false;
    }

    // 确保连接可用
    if (!mysql_ || mysql_ping(mysql_) != 0) {
        std::cout << "[master-server] MySQL 尝试重连...\n";
        if (!connect()) {
            return false;
        }
    }

    std::cout << "[master-server] MySQL queryOne SQL: " << sql << '\n';
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        std::cerr << "[master-server] MySQL 查询失败: "
                  << mysql_error(mysql_) << '\n';
        return false;
    }

    bool ok = row_handler(mysql_);
    std::cout << "[master-server] MySQL queryOne result: "
              << (ok ? "OK" : "FAILED") << '\n';
    return ok;
}

bool MySQLConnection::queryMany(const std::string& sql, std::function<bool(MYSQL*)> row_handler)
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        std::cerr << "[master-server] MySQL 未初始化\n";
        return false;
    }

    // 确保连接可用
    if (!mysql_ || mysql_ping(mysql_) != 0) {
        std::cout << "[master-server] MySQL 尝试重连...\n";
        if (!connect()) {
            return false;
        }
    }

    std::cout << "[master-server] MySQL queryMany SQL: " << sql << '\n';
    if (mysql_query(mysql_, sql.c_str()) != 0) {
        std::cerr << "[master-server] MySQL 查询失败: "
                  << mysql_error(mysql_) << '\n';
        return false;
    }
    bool ok = row_handler(mysql_);
    std::cout << "[master-server] MySQL queryMany result: "
              << (ok ? "OK" : "FAILED") << '\n';
    return ok;
}

}  // namespace storage
}  // namespace master
