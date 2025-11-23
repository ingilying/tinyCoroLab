#include "coro/engine.hpp"
#include "config.h"
#include "coro/io/io_info.hpp"
#include "coro/meta_info.hpp"
#include "coro/task.hpp"
#include "coro/uring_proxy.hpp"
#include <coroutine>
#include <utility>

#define wakeup_by_cqe(val) ((val & cqe_mask) >= 0)

namespace coro::detail
{

auto engine::init() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    linfo.egn = this;
    m_upxy.init(config::kEntryLength);
}

auto engine::deinit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    linfo.egn    = nullptr;
    m_waiting_io = 0;
    m_running_io = 0;
    m_upxy.deinit();
    mpmc_queue<coroutine_handle<>> empty;
    m_task_queue.swap(empty);
}

auto engine::ready() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return !m_task_queue.was_empty();
}

auto engine::get_free_urs() noexcept -> ursptr
{
    // TODO[lab2a]: Add you codes
    return get_uring().get_free_sqe();
}

auto engine::num_task_schedule() noexcept -> size_t
{
    // TODO[lab2a]: Add you codes
    return m_task_queue.was_size();
}

auto engine::schedule() noexcept -> coroutine_handle<>
{
    // TODO[lab2a]: Add you codes
    auto coro = m_task_queue.pop();
    return coro;
}

auto engine::submit_task(coroutine_handle<> handle) noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_task_queue.push(handle);
    wakeup(task_flag);
    return;
}

auto engine::exec_one_task() noexcept -> void
{
    auto coro = schedule();
    coro.resume();
    if (coro.done())
    {
        clean(coro);
    }
}

auto engine::handle_cqe_entry(urcptr cqe) noexcept -> void
{
    auto data = reinterpret_cast<io::detail::io_info*>(io_uring_cqe_get_data(cqe));
    data->cb(data, cqe->res);
}

auto engine::poll_submit() noexcept -> void
{
    // submit io
    if (m_waiting_io > 0)
    {
        m_upxy.submit();
        m_running_io += m_waiting_io;
        m_waiting_io = 0;
    }
    // TODO[lab2a]: Add you codes
    // waiting io and task
    auto cnt = m_upxy.wait_eventfd();
    if (!wakeup_by_cqe(cnt))
    {
        return;
    }
    // handle finished io
    auto num = m_upxy.peek_batch_cqe(m_urc.data(), m_running_io);
    if (num != 0)
    {
        for (size_t i = 0; i < num; ++i)
        {
            handle_cqe_entry(m_urc[i]);
        }
        m_upxy.cq_advance(num);
        m_running_io -= num;
    }
}

auto engine::add_io_submit() noexcept -> void
{
    // TODO[lab2a]: Add you codes
    m_waiting_io += 1;
}

auto engine::empty_io() noexcept -> bool
{
    // TODO[lab2a]: Add you codes
    return m_waiting_io == 0 && m_running_io == 0;
}

auto engine::wakeup(uint64_t state) noexcept -> void
{
    m_upxy.write_eventfd(state);
}
}; // namespace coro::detail