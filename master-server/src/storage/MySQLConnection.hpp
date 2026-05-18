/**
 * @file   MySQLConnection.hpp
 * @brief  MySQL 业务库连接管理（单例，自动重连）
 * @module master-server
 */

#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include <mysql.h>

namespace master {
namespace storage {

/**
 * @brief MySQL 连接配置
 */
struct MySQLConfig {
    std::string host;
    int port;
    std::string username;
    std::string password;
    std::string database;
    int connect_timeout;
    int reconnect_interval;

    MySQLConfig()
        : host("127.0.0.1")
        , port(3306)
        , username("root")
        , password("123456")
        , database("scada_demo")
        , connect_timeout(10)
        , reconnect_interval(5)
    {
    }
};

/**
 * @brief MySQL 连接管理器（单例模式，线程安全）
 *
 * 功能：
 * - 自动连接与重连
 * - 连接状态检查
 * - 简单的执行接口
 */
class MySQLConnection {
public:
    /** 获取单例实例 */
    static MySQLConnection& instance();

    /** 禁止拷贝 */
    MySQLConnection(const MySQLConnection&) = delete;
    MySQLConnection& operator=(const MySQLConnection&) = delete;

    /** 初始化连接（需在使用前调用） */
    bool initialize(const MySQLConfig& config);

    /** 获取原始 MYSQL 指针（只读，非线程安全，需要外部加锁） */
    MYSQL* getRaw() const;

    /** 检查连接是否有效 */
    bool isValid() const;

    /** 执行 SQL 查询（返回受影响行数，或 -1 表示失败） */
    int execute(const std::string& sql);

    /** 查询并返回单行数据（仅用于简单查询） */
    bool queryOne(const std::string& sql, std::function<bool(MYSQL*)> row_handler);

    /** 查询多行数据 */
    bool queryMany(const std::string& sql, std::function<bool(MYSQL*)> row_handler);

private:
    MySQLConnection();
    ~MySQLConnection();

    bool connect();
    void disconnect();

    mutable std::mutex mutex_;
    MySQLConfig config_;
    MYSQL* mysql_;
    bool initialized_;
};

}  // namespace storage
}  // namespace master
