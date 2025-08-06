#include "ipc_socket.h"

// 아래 함수들은 오직 여기(하나의 .cc)에만 구현해야 합니다.

StatusType SendSchedulerSocket_impl(SocketType type, ThreadType thread_type, Scheduler &scheduler) {
    char * socket_path;
    if(type == SocketType::INIT){
        socket_path = SCHEDULER_SOCK_INIT;
    }
    else if(type == SocketType::PREPARE){
        if(thread_type == ThreadType::MAIN)
            socket_path = SCHEDULER_SOCK_PREPARE_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = SCHEDULER_SOCK_PREPARE_SUB;
    }
    else if(type == SocketType::RUN){
        if(thread_type == ThreadType::MAIN)
            socket_path = SCHEDULER_SOCK_RUN_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = SCHEDULER_SOCK_RUN_SUB;
    }
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "[SendSocketData] Socket creation failed: ";
        perror("socket error");
        return StatusType::FAILURE;
    }
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    const int MAX_RETRIES = 20;
    const int INITIAL_DELAY_MS = 100;
    int attempt = 0;

    while (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == ECONNREFUSED || errno == ENOENT) {
            if (++attempt >= MAX_RETRIES) {
                std::cerr << "[SendSocketData] Connection failed after retries: ";
                perror("connect error");
                close(sockfd);
                return StatusType::FAILURE;
            }
            int delay = INITIAL_DELAY_MS * (1 << attempt);
            std::cerr << "[SendSocketData] Retrying to connect in "
                      << delay << " ms...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        } else {
            std::cerr << "[SendSocketData] Connection failed: ";
            perror("connect error");
            close(sockfd);
            return StatusType::FAILURE;
        }
    }
    if(type == SocketType::INIT){
        if (send(sockfd, &scheduler.buffer_init, sizeof(scheduler.buffer_init), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully: loop num = "
                  << scheduler.buffer_init.loop_num << ", status = "
                  << static_cast<int>(scheduler.buffer_init.status) << std::endl;
    }
    else if(type == SocketType::PREPARE && thread_type == ThreadType::MAIN){
        if (send(sockfd, &scheduler.buffer_prepare_main, sizeof(scheduler.buffer_prepare_main), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully: seg info : "
                  << scheduler.buffer_prepare_main.segs[0].resource << "\n";
        // std::cout << "[SendSocketData] Sent successfully: flag info : "
        //                                        << scheduler.buffer_prepare_main.seq_opt << "\n";
    }
    else if(type == SocketType::PREPARE && thread_type == ThreadType::SUB){
        if (send(sockfd, &scheduler.buffer_prepare_sub, sizeof(scheduler.buffer_prepare_sub), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully: seg info : "
                  << scheduler.buffer_prepare_sub.segs[0].resource << "\n";
        //    std::cout << "[SendSocketData] Sent successfully: flag info : "
        //                                   << scheduler.buffer_prepare_sub.seq_opt << "\n";
    }
    else if(type == SocketType::RUN && thread_type == ThreadType::MAIN){
        if (send(sockfd, &scheduler.buffer_run_main, sizeof(scheduler.buffer_run_main), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully \n";

    }
    else if(type == SocketType::RUN && thread_type == ThreadType::SUB){
        if (send(sockfd, &scheduler.buffer_run_sub, sizeof(scheduler.buffer_run_sub), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully \n";

    }
    return StatusType::SUCCESS;
}

StatusType RecieveSchedulerSocket_impl(SocketType type, ThreadType thread_type, InterpreterManager &interpreter_manager) {
    char * socket_path;
    if(type == SocketType::INIT){
        socket_path = SCHEDULER_SOCK_INIT;
    }
    else if(type == SocketType::PREPARE){
        if(thread_type == ThreadType::MAIN)
            socket_path = SCHEDULER_SOCK_PREPARE_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = SCHEDULER_SOCK_PREPARE_SUB;
    }
    else if(type == SocketType::RUN){
        if(thread_type == ThreadType::MAIN)
            socket_path = SCHEDULER_SOCK_RUN_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = SCHEDULER_SOCK_RUN_SUB;
    }
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "[ReceiveSocketData] Socket creation failed: ";
        perror("socket error");
        return StatusType::FAILURE;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "[ReceiveSocketData] Bind failed: ";
        perror("bind error");
        close(sockfd);
        return StatusType::FAILURE;
    }

    if (listen(sockfd, 1) == -1) {
        std::cerr << "[ReceiveSocketData] Listen failed: ";
        perror("listen error");
        close(sockfd);
        return StatusType::FAILURE;
    }

    int client_fd = accept(sockfd, nullptr, nullptr);
    if (client_fd == -1) {
        std::cerr << "[ReceiveSocketData] Accept failed: ";
        perror("accept error");
        close(sockfd);
        return StatusType::FAILURE;
    }
    if (type == SocketType::INIT){
        if (recv(client_fd, &interpreter_manager.buffer_init, sizeof(interpreter_manager.buffer_init), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
        std::cout << "[ReceiveSocketData] Received successfully: loop num = "
                  << interpreter_manager.buffer_init.loop_num << ", status = "
                  << static_cast<int>(interpreter_manager.buffer_init.status) << std::endl;
    }
    else if (type == SocketType::PREPARE && thread_type == ThreadType::MAIN){
        // while (true) {
        ssize_t bytes_received = recv(client_fd, &interpreter_manager.buffer_prepare_main,
                                      sizeof(interpreter_manager.buffer_prepare_main), 0);
        if (bytes_received == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }

        // if (interpreter_manager.buffer_prepare_main.core_flag_IPC.IS_VALID) {
        // break;
        // }

        // std::cerr << "[ReceiveSocketData] Received invalid data (IS_VALID=0), retrying recv...\n";
        // }

        std::cout << "[ReceiveSocketData] Received successfully: seg info : "
                  << interpreter_manager.buffer_prepare_main.segs[0].resource << "\n";
        // std::cout << "[ReceiveSocketData] Received successfully: flag info : "
        //       << interpreter_manager.buffer_prepare_main.seq_opt << "\n";
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
    }
    else if (type == SocketType::PREPARE && thread_type == ThreadType::SUB){
        // while (true) {
        ssize_t bytes_received = recv(client_fd, &interpreter_manager.buffer_prepare_sub,
                                      sizeof(interpreter_manager.buffer_prepare_sub), 0);
        if (bytes_received == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        // if (interpreter_manager.buffer_prepare_sub.core_flag_IPC.IS_VALID) {
        // break;
        // }

        // std::cerr << "[ReceiveSocketData] Received invalid data (IS_VALID=0), retrying recv...\n";
        // }
        std::cout << "[ReceiveSocketData] Received successfully: seg info : "
                  << interpreter_manager.buffer_prepare_sub.segs[0].resource << "\n";
        //    std::cout << "[ReceiveSocketData] Received successfully: flag info : "
        //                       << interpreter_manager.buffer_prepare_sub.seq_opt << "\n";
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
    }
    else if (type == SocketType::RUN && thread_type == ThreadType::MAIN){
        if (recv(client_fd, &interpreter_manager.buffer_run_main, sizeof(interpreter_manager.buffer_run_main), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        std::cout << "[ReceiveSocketData] Received successfully \n";

        close(client_fd);
        close(sockfd);
        unlink(socket_path);
    }
    else if (type == SocketType::RUN && thread_type == ThreadType::SUB){
        if (recv(client_fd, &interpreter_manager.buffer_run_sub, sizeof(interpreter_manager.buffer_run_sub), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        std::cout << "[ReceiveSocketData] Received successfully \n";

        close(client_fd);
        close(sockfd);
        unlink(socket_path);
    }
    return StatusType::SUCCESS;
}

StatusType SendInterpreterManagerSocket_impl(SocketType type, ThreadType thread_type, InterpreterManager &interpreter_manager) {
    char * socket_path;
    if(type == SocketType::INIT){
        socket_path = INTERPRETER_MANAGER_SOCK_INIT;
    }
    else if(type == SocketType::PREPARE){
        if(thread_type == ThreadType::MAIN)
            socket_path = INTERPRETER_MANAGER_SOCK_PREPARE_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = INTERPRETER_MANAGER_SOCK_PREPARE_SUB;
    }
    else if(type == SocketType::RUN){
        if(thread_type == ThreadType::MAIN)
            socket_path = INTERPRETER_MANAGER_SOCK_RUN_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = INTERPRETER_MANAGER_SOCK_RUN_SUB;
    }
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "[SendSocketData] Socket creation failed: ";
        perror("socket error");
        return StatusType::FAILURE;
    }
    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    const int MAX_RETRIES = 20;
    const int INITIAL_DELAY_MS = 100;
    int attempt = 0;
    //////////////////////////////////////////////////////////////////////
    /* Add connect retry logic to "SendInterpreterManagerSocket_impl" */
    while (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        if (errno == ECONNREFUSED || errno == ENOENT) {
            if (++attempt >= MAX_RETRIES) {
                std::cerr << "[SendSocketData] Connection failed after retries: ";
                perror("connect error");
                close(sockfd);
                return StatusType::FAILURE;
            }
            int delay = INITIAL_DELAY_MS * (1 << attempt);
            std::cerr << "[SendSocketData] Retrying to connect in "
                      << delay << " ms...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        } else {
            std::cerr << "[SendSocketData] Connection failed: ";
            perror("connect error");
            close(sockfd);
            return StatusType::FAILURE;
        }
    }
    //////////////////////////////////////////////////////////////////////

    if(type == SocketType::INIT){
        if (send(sockfd, &interpreter_manager.buffer_init, sizeof(interpreter_manager.buffer_init), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully: loop num = "
                  << interpreter_manager.buffer_init.loop_num << ", status = "
                  << static_cast<int>(interpreter_manager.buffer_init.status) << std::endl;
    }
    else if(type == SocketType::PREPARE && thread_type == ThreadType::MAIN){
        if (send(sockfd, &interpreter_manager.buffer_prepare_main, sizeof(interpreter_manager.buffer_prepare_main), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully\n";
    }
    else if(type == SocketType::PREPARE && thread_type == ThreadType::SUB){
        if (send(sockfd, &interpreter_manager.buffer_prepare_sub, sizeof(interpreter_manager.buffer_prepare_sub), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully\n";
    }
    else if(type == SocketType::RUN && thread_type == ThreadType::MAIN){
        if (send(sockfd, &interpreter_manager.buffer_run_main, sizeof(interpreter_manager.buffer_run_main), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully \n";
//        LOGI("[SendSocketData] Sent successfully (Main)");

    }
    else if(type == SocketType::RUN && thread_type == ThreadType::SUB){
        if (send(sockfd, &interpreter_manager.buffer_run_sub, sizeof(interpreter_manager.buffer_run_sub), 0) == -1) {
            std::cerr << "[SendSocketData] Data sending failed: ";
            perror("send error");
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(sockfd);
        std::cout << "[SendSocketData] Sent successfully \n";
//        LOGI("[SendSocketData] Sent successfully (Sub)");
    }
    return StatusType::SUCCESS;
}

StatusType RecieveInterpreterManagerSocket_impl(SocketType type, ThreadType thread_type, Scheduler &scheduler) {
    char * socket_path;
    if(type == SocketType::INIT){
        socket_path = INTERPRETER_MANAGER_SOCK_INIT;
    }
    else if(type == SocketType::PREPARE){
        if(thread_type == ThreadType::MAIN)
            socket_path = INTERPRETER_MANAGER_SOCK_PREPARE_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = INTERPRETER_MANAGER_SOCK_PREPARE_SUB;
    }
    else if(type == SocketType::RUN){
        if(thread_type == ThreadType::MAIN)
            socket_path = INTERPRETER_MANAGER_SOCK_RUN_MAIN;
        if(thread_type == ThreadType::SUB)
            socket_path = INTERPRETER_MANAGER_SOCK_RUN_SUB;
    }
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        std::cerr << "[ReceiveSocketData] Socket creation failed: ";
        perror("socket error");
        return StatusType::FAILURE;
    }

    struct sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);
    unlink(socket_path);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "[ReceiveSocketData] Bind failed: ";
        perror("bind error");
        close(sockfd);
        return StatusType::FAILURE;
    }

    if (listen(sockfd, 1) == -1) {
        std::cerr << "[ReceiveSocketData] Listen failed: ";
        perror("listen error");
        close(sockfd);
        return StatusType::FAILURE;
    }

    const int MAX_RETRIES = 20;
    const int INITIAL_DELAY_MS = 100;
    int attempt = 0;
    int client_fd = -1;

    while (true) {
        client_fd = accept(sockfd, nullptr, nullptr);
        if (client_fd != -1) {
            break;
        }
        if (errno == EINTR || errno == EAGAIN) {
            if (++attempt >= MAX_RETRIES) {
                std::cerr << "[ReceiveSocketData] Accept failed after retries: ";
                perror("accept error");
                close(sockfd);
                return StatusType::FAILURE;
            }
            int delay = INITIAL_DELAY_MS * (1 << attempt);
            std::cerr << "[ReceiveSocketData] Retrying accept in "
                      << delay << " ms...\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(delay));
        } else {
            std::cerr << "[ReceiveSocketData] Accept failed: ";
            perror("accept error");
            close(sockfd);
            return StatusType::FAILURE;
        }
    }
    //

    if (client_fd == -1) {
        std::cerr << "[ReceiveSocketData] Accept failed: ";
        perror("accept error");
        close(sockfd);
        return StatusType::FAILURE;
    }
    if (type == SocketType::INIT){
        if (recv(client_fd, &scheduler.buffer_init, sizeof(scheduler.buffer_init), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
        std::cout << "[ReceiveSocketData] Received successfully: loop num = "
                  << scheduler.buffer_init.loop_num << ", status = "
                  << static_cast<int>(scheduler.buffer_init.status) << std::endl;
    }
    else if (type == SocketType::PREPARE && thread_type == ThreadType::MAIN){
        if (recv(client_fd, &scheduler.buffer_prepare_main, sizeof(scheduler.buffer_prepare_main), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
        std::cout << "[ReceiveSocketData] Received successfully\n";
    }
    else if (type == SocketType::PREPARE && thread_type == ThreadType::SUB){
        if (recv(client_fd, &scheduler.buffer_prepare_sub, sizeof(scheduler.buffer_prepare_sub), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
        std::cout << "[ReceiveSocketData] Received successfully\n";
    }
    else if (type == SocketType::RUN && thread_type == ThreadType::MAIN){
        if (recv(client_fd, &scheduler.buffer_run_main, sizeof(scheduler.buffer_run_main), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        std::cout << "[ReceiveSocketData] Received successfully \n";
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
    }
    else if (type == SocketType::RUN && thread_type == ThreadType::SUB){
        if (recv(client_fd, &scheduler.buffer_run_sub, sizeof(scheduler.buffer_run_sub), 0) == -1) {
            std::cerr << "[ReceiveSocketData] Data receiving failed: ";
            perror("recv error");
            close(client_fd);
            close(sockfd);
            return StatusType::FAILURE;
        }
        std::cout << "[ReceiveSocketData] Received successfully \n";
        close(client_fd);
        close(sockfd);
        unlink(socket_path);
    }
    return StatusType::SUCCESS;
}

void CleanSocketFiles() {
    const char* sockets[] = {
            INTERPRETER_MANAGER_SOCK_INIT,
            INTERPRETER_MANAGER_SOCK_PREPARE_MAIN,
            INTERPRETER_MANAGER_SOCK_PREPARE_SUB,
            INTERPRETER_MANAGER_SOCK_RUN_MAIN,
            INTERPRETER_MANAGER_SOCK_RUN_SUB,
            INTERPRETER_MANAGER_SOCK_CLEAN_MAIN,
            INTERPRETER_MANAGER_SOCK_CLEAN_SUB,
            SCHEDULER_SOCK_INIT,
            SCHEDULER_SOCK_PREPARE_MAIN,
            SCHEDULER_SOCK_PREPARE_SUB,
            SCHEDULER_SOCK_RUN_MAIN,
            SCHEDULER_SOCK_RUN_SUB,
            SCHEDULER_SOCK_CLEAN_MAIN,
            SCHEDULER_SOCK_CLEAN_SUB
    };

    for (const char* sock_path : sockets) {
        if (unlink(sock_path) == 0) {
            std::cout << "[Socket Cleanup] Removed: " << sock_path << std::endl;
        } else {
            std::perror(("[Socket Cleanup] Failed to remove: " + std::string(sock_path)).c_str());
        }
    }
}