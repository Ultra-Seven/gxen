/*
 * NVIDIA NVC0 VM(Virtual Memory) model
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
#include "nvc0.h"
#include "nvc0_inttypes.h"
#include "nvc0_vm.h"
#include "nvc0_mmio.h"
#include "nvc0_context.h"
#include "nvc0_remapping.h"
namespace nvc0 {

uint32_t vm_bar1_read(nvc0_state_t* state, target_phys_addr_t offset) {
    context* ctx = context::extract(state);
    const cross::command cmd = {
        cross::command::TYPE_READ,
        0,
        offset,
        cross::command::BAR1
    };
    return ctx->send(cmd).value;
}

void vm_bar1_write(nvc0_state_t* state, target_phys_addr_t offset, uint32_t value) {
    context* ctx = context::extract(state);
    const cross::command cmd = {
        cross::command::TYPE_WRITE,
        value,
        offset,
        cross::command::BAR1
    };
    ctx->send(cmd);
}

uint32_t vm_bar3_read(nvc0_state_t* state, target_phys_addr_t offset) {
    context* ctx = context::extract(state);
    const cross::command cmd = {
        cross::command::TYPE_READ,
        0,
        offset,
        cross::command::BAR3
    };
    return ctx->send(cmd).value;
}

void vm_bar3_write(nvc0_state_t* state, target_phys_addr_t offset, uint32_t value) {
    context* ctx = context::extract(state);
    const cross::command cmd = {
        cross::command::TYPE_WRITE,
        value,
        offset,
        cross::command::BAR3
    };
    ctx->send(cmd);
}

}  // namespace nvc0
/* vim: set sw=4 ts=4 et tw=80 : */
