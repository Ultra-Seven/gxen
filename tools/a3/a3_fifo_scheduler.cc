/*
 * A3 FIFO scheduler
 *
 * Copyright (c) 2012-2013 Yusuke Suzuki
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <stdint.h>
#include "a3.h"
#include "a3_fifo_scheduler.h"
#include "a3_context.h"
#include "a3_registers.h"
#include "a3_device.h"
#include "a3_device_bar1.h"
#include "a3_ignore_unused_variable_warning.h"
namespace a3 {

fifo_scheduler_t::fifo_scheduler_t(const boost::posix_time::time_duration& wait, const boost::posix_time::time_duration& period, const boost::posix_time::time_duration& sample)
    : wait_(wait)
    , period_(period)
    , sample_(sample)
    , thread_()
    , mutex_()
    , cond_()
    , queue_()
{
}

fifo_scheduler_t::~fifo_scheduler_t() {
    stop();
}

void fifo_scheduler_t::register_context(context* ctx) {
    boost::unique_lock<boost::mutex> lock(mutex_);
    contexts_.push_back(*ctx);
}

void fifo_scheduler_t::unregister_context(context* ctx) {
    boost::unique_lock<boost::mutex> lock(mutex_);
    contexts_.remove_if([ctx](const context& target) {
        return &target == ctx;
    });
}

void fifo_scheduler_t::start() {
    if (thread_) {
        stop();
    }
    thread_.reset(new boost::thread(&fifo_scheduler_t::run, this));
    replenisher_.reset(new boost::thread(&fifo_scheduler_t::replenish, this));
    sampler_.reset(new boost::thread(&fifo_scheduler_t::sampling, this));
}

void fifo_scheduler_t::stop() {
    if (thread_) {
        thread_->interrupt();
        thread_.reset();
        replenisher_->interrupt();
        replenisher_.reset();
        sampler_->interrupt();
        sampler_.reset();
    }
}

void fifo_scheduler_t::replenish() {
    // uint64_t count = 0;
    while (true) {
        // replenish
        {
            boost::unique_lock<boost::mutex> lock(mutex_);
            if (contexts_.size()) {
                boost::posix_time::time_duration period = bandwidth_;
                // boost::posix_time::time_duration period = bandwidth_ + gpu_idle_;
                if (bandwidth_ != boost::posix_time::microseconds(0)) {
                    const auto budget = period / contexts_.size();
                    for (context& ctx: contexts_) {
                        ctx.replenish(budget, period_);
                    }
                    // ++count;
                }
                bandwidth_ = boost::posix_time::microseconds(0);
                gpu_idle_ = boost::posix_time::microseconds(0);
            }
        }
        boost::this_thread::sleep(period_);
        boost::this_thread::yield();
    }
}

void fifo_scheduler_t::enqueue(context* ctx, const command& cmd) {
    {
        boost::unique_lock<boost::mutex> lock(mutex_);
        queue_.push(fire_t(ctx, cmd));
    }
    cond_.notify_one();
}

void fifo_scheduler_t::run() {
    boost::condition_variable_any cond;
    boost::unique_lock<boost::mutex> lock(mutex_);
    while (true) {
        fire_t handle;
        while (queue_.empty()) {
            cond_.wait(lock);
        }
        handle = queue_.front();
        queue_.pop();

        lock.unlock();
        utilization_.start();

        A3_SYNCHRONIZED(device::instance()->mutex()) {
            device::instance()->bar1()->write(handle.first, handle.second);
        }

        lock.lock();

        while (device::instance()->is_active(handle.first)) {
            cond.timed_wait(lock, wait_);
        }

        const auto duration = utilization_.elapsed();
        bandwidth_ += duration;
        sampling_bandwidth_ += duration;
        handle.first->update_budget(duration);
    }
}

void fifo_scheduler_t::sampling() {
    uint64_t count = 0;
    while (true) {
        // sampling
        {
            boost::unique_lock<boost::mutex> lock(mutex_);
            if (!contexts_.empty()) {
                if (sampling_bandwidth_ != boost::posix_time::microseconds(0)) {
                    A3_FATAL(stdout, "UTIL: LOG %" PRIu64 "\n", count);
                    for (context& ctx : contexts_) {
                        A3_FATAL(stdout, "UTIL: %d => %f\n", ctx.id(), (static_cast<double>(ctx.sampling_bandwidth_used().total_microseconds()) / sampling_bandwidth_.total_microseconds()));
                        ctx.clear_sampling_bandwidth_used();
                    }
                    ++count;
                }
                sampling_bandwidth_ = boost::posix_time::microseconds(0);
            }
        }
        // boost::this_thread::sleep(boost::posix_time::microseconds(500));
        // boost::this_thread::sleep(boost::posix_time::milliseconds(50));
        // boost::this_thread::sleep(boost::posix_time::microseconds(1000));
        boost::this_thread::sleep(sample_);
        boost::this_thread::yield();
    }
}

}  // namespace a3
/* vim: set sw=4 ts=4 et tw=80 : */
