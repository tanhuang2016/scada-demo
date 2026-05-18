#pragma once

/**
 * @file   MasterApplication.hpp
 * @brief  主站应用：生命周期与后台服务编排（迭代 1 起加载 MySQL，迭代 2 起 104）
 */

namespace master {

/**
 * @brief 主站进程入口类，负责 run 循环与优雅退出
 */
class MasterApplication {
public:
    MasterApplication();
    ~MasterApplication();

    /**
     * @brief 启动主站逻辑，阻塞直到 requestStop
     * @return 进程退出码，0 表示正常
     */
    int run();

    /** @brief 请求结束主循环（信号处理或外部调用） */
    void requestStop();

private:
    MasterApplication(const MasterApplication&);
    MasterApplication& operator=(const MasterApplication&);

    bool running_;
};

}  // namespace master
