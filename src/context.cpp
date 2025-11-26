#include "coro/context.hpp"
#include "coro/engine.hpp"
#include <atomic>
#include <cstddef>

namespace coro
{
context::context() noexcept : m_engine(), m_register_count(0)
{
    m_id = ginfo.context_id.fetch_add(1, std::memory_order_relaxed);
}

auto context::init() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = this;
    m_engine.init();
}

auto context::deinit() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    linfo.ctx = nullptr;
    m_engine.deinit();
}

auto context::start() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job = make_unique<jthread>(
        [this](stop_token token)
        {
            this->init();
            if (!(this->m_stop_callback))
            {
                this->m_stop_callback = [&]() { m_job->request_stop(); };
            }
            this->run(token);
            this->deinit();
        });
}

auto context::notify_stop() noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_job->request_stop();
    m_engine.wakeup();
}

auto context::submit_task(std::coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_engine.submit_task(handle);
}

auto context::register_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_register_count.fetch_add(register_cnt, std::memory_order_acq_rel);
}

auto context::unregister_wait(int register_cnt) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    m_register_count.fetch_sub(register_cnt, std::memory_order_acq_rel);
}

auto context::run(stop_token token) noexcept -> void
{
    // TODO[lab2b]: Add you codes
    while (!token.stop_requested())
    {
        // do task work
        auto num = m_engine.num_task_schedule();
        for (size_t i = 0; i < num; ++i)
        {
            m_engine.exec_one_task();
        }
        // check if stop
        if (m_engine.empty_io() && m_register_count.load(std::memory_order_acquire) == 0)
        {
            if (!m_engine.ready())
            {
                // have no task to run then do io
                m_stop_callback();
            }
            else
            {
                // still have tasks to run
                continue;
            }
        }
        // do io
        m_engine.poll_submit();
    }
}

}; // namespace coro