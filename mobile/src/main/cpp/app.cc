#include "app.h"

std::mutex interpreter_manager_mutex;

StatusType RecieveSchedulerSocket(SocketType socket_type, ThreadType thread_type, /*src*/ InterpreterManager &interpreter_manager) {
    if (socket_type == SocketType::INIT)
        return RecieveSchedulerSocket_impl(SocketType::INIT, ThreadType::UNI, interpreter_manager);
    else if (socket_type == SocketType::PREPARE) {
        if (thread_type == ThreadType::MAIN)
            return RecieveSchedulerSocket_impl(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager);
        else if (thread_type == ThreadType::SUB)
            return RecieveSchedulerSocket_impl(SocketType::PREPARE, ThreadType::SUB, interpreter_manager);
    }
    else if (socket_type == SocketType::RUN) {
        if (thread_type == ThreadType::MAIN)
            return RecieveSchedulerSocket_impl(SocketType::RUN, ThreadType::MAIN, interpreter_manager);
        else if (thread_type == ThreadType::SUB)
            return RecieveSchedulerSocket_impl(SocketType::RUN, ThreadType::SUB, interpreter_manager);
    }
    return StatusType::SUCCESS;
}

StatusType SendInterpreterManagerSocket(SocketType socket_type, ThreadType thread_type, /*dst*/ InterpreterManager &interpreter_manager) {
    if (socket_type == SocketType::INIT)
        return SendInterpreterManagerSocket_impl(SocketType::INIT, ThreadType::UNI, interpreter_manager);
    else if (socket_type == SocketType::PREPARE) {
        if (thread_type == ThreadType::MAIN)
            return SendInterpreterManagerSocket_impl(SocketType::PREPARE, ThreadType::MAIN, interpreter_manager);
        else if (thread_type == ThreadType::SUB)
            return SendInterpreterManagerSocket_impl(SocketType::PREPARE, ThreadType::SUB, interpreter_manager);
    }
    else if (socket_type == SocketType::RUN) {
        if (thread_type == ThreadType::MAIN)
            return SendInterpreterManagerSocket_impl(SocketType::RUN, ThreadType::MAIN, interpreter_manager);
        else if (thread_type == ThreadType::SUB)
            return SendInterpreterManagerSocket_impl(SocketType::RUN, ThreadType::SUB, interpreter_manager);
    }
    return StatusType::SUCCESS;
}

StatusType SendSchedulerSocket(SocketType socket_type, ThreadType thread_type, /*src*/ Scheduler &scheduler) {
    if (socket_type == SocketType::INIT)
        return SendSchedulerSocket_impl(SocketType::INIT, ThreadType::UNI, scheduler);
    else if (socket_type == SocketType::PREPARE) {
        if (thread_type == ThreadType::MAIN)
            return SendSchedulerSocket_impl(SocketType::PREPARE, ThreadType::MAIN, scheduler);
        else if (thread_type == ThreadType::SUB)
            return SendSchedulerSocket_impl(SocketType::PREPARE, ThreadType::SUB, scheduler);
    }
    else if (socket_type == SocketType::RUN) {
        if (thread_type == ThreadType::MAIN)
            return SendSchedulerSocket_impl(SocketType::RUN, ThreadType::MAIN, scheduler);
        else if (thread_type == ThreadType::SUB)
            return SendSchedulerSocket_impl(SocketType::RUN, ThreadType::SUB, scheduler);
    }
}

StatusType RecieveInterpreterManagerSocket(SocketType socket_type, ThreadType thread_type,
                                           Scheduler &scheduler) {
    if (socket_type == SocketType::INIT)
        return RecieveInterpreterManagerSocket_impl(SocketType::INIT, ThreadType::UNI, scheduler);
    else if (socket_type == SocketType::PREPARE) {
        if (thread_type == ThreadType::MAIN)
            return RecieveInterpreterManagerSocket_impl(SocketType::PREPARE, ThreadType::MAIN, scheduler);
        else if (thread_type == ThreadType::SUB)
            return RecieveInterpreterManagerSocket_impl(SocketType::PREPARE, ThreadType::SUB, scheduler);
    }
    else if (socket_type == SocketType::RUN) {
        if (thread_type == ThreadType::MAIN)
            return RecieveInterpreterManagerSocket_impl(SocketType::RUN, ThreadType::MAIN, scheduler);
        else if (thread_type == ThreadType::SUB)
            return RecieveInterpreterManagerSocket_impl(SocketType::RUN, ThreadType::SUB, scheduler);
    }
}

uint64_t millis__() {
    uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    return ms;
};