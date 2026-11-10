// miniaudio's setup glue, and the one translation unit that compiles
// miniaudio's implementation.
// SPDX-License-Identifier: MIT-0
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio_setup.h"

#include <cstdlib>
#include <cstring>

#include "automap/lifecycle.h"

namespace {

ma_context context;
ma_engine engine;

} // namespace

ma_allocation_callbacks& am_miniaudio_allocation() {
    static ma_allocation_callbacks callbacks = {};
    return callbacks;
}

ma_log& am_miniaudio_log() {
    static ma_log log;
    return log;
}

ma_engine& am_miniaudio_engine() { return engine; }

bool am_miniaudio_setup_init(am::Context& ctx) {
    ma_context_config context_config = ma_context_config_init();
    context_config.pLog = &am_miniaudio_log();
    context_config.allocationCallbacks = am_miniaudio_allocation();
    const char* backend = std::getenv("AUTOMAP_MINIAUDIO_BACKEND");
    ma_backend null_backend[] = {ma_backend_null};
    bool headless = backend != nullptr && std::strcmp(backend, "null") == 0;
    if (ma_context_init(headless ? null_backend : nullptr, headless ? 1 : 0, &context_config, &context) != MA_SUCCESS) {
        return false;
    }
    ma_engine_config engine_config = ma_engine_config_init();
    engine_config.pContext = &context;
    engine_config.pLog = &am_miniaudio_log();
    engine_config.allocationCallbacks = am_miniaudio_allocation();
    if (ma_engine_init(&engine_config, &engine) != MA_SUCCESS) {
        ma_context_uninit(&context);
        return false;
    }
    ctx.miniaudio.engine = &engine;
    am::lifecycle::init_step("miniaudio:ma_context_init+ma_engine_init");
    return true;
}

void am_miniaudio_setup_shutdown(am::Context& ctx) {
    ma_engine_uninit(&engine);
    ma_context_uninit(&context);
    ctx.miniaudio.engine = nullptr;
    am::lifecycle::shutdown_step("miniaudio:ma_engine_uninit+ma_context_uninit");
}
