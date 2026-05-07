#ifndef MEM_CONSOLE_APP_LOOP_INTERNAL_H
#define MEM_CONSOLE_APP_LOOP_INTERNAL_H

#include "mem_console_app_internal.h"

typedef enum MemConsoleInputRouteTargetPolicy {
    MEM_CONSOLE_INPUT_ROUTE_TARGET_FALLBACK = 0,
    MEM_CONSOLE_INPUT_ROUTE_TARGET_GLOBAL = 1,
    MEM_CONSOLE_INPUT_ROUTE_TARGET_UI = 2
} MemConsoleInputRouteTargetPolicy;

typedef enum MemConsoleInputInvalidateReasonBits {
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_QUIT = 1u << 0,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_WINDOW = 1u << 1,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_KEYBOARD = 1u << 2,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_POINTER = 1u << 3,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_WHEEL = 1u << 4,
    MEM_CONSOLE_INPUT_INVALIDATE_REASON_TEXT = 1u << 5
} MemConsoleInputInvalidateReasonBits;

typedef struct MemConsoleInputEventRaw {
    uint32_t sdl_event_count;
    uint32_t quit_event_count;
    uint32_t window_event_count;
    uint32_t keyboard_event_count;
    uint32_t pointer_event_count;
    uint32_t wheel_event_count;
    uint32_t text_event_count;
    uint32_t other_event_count;
    uint8_t quit_requested;
} MemConsoleInputEventRaw;

typedef struct MemConsoleInputEventNormalized {
    uint8_t has_quit_action;
    uint8_t has_window_action;
    uint8_t has_keyboard_action;
    uint8_t has_pointer_action;
    uint8_t has_wheel_action;
    uint8_t has_text_action;
    uint32_t action_count;
} MemConsoleInputEventNormalized;

typedef struct MemConsoleInputRouteResult {
    uint8_t consumed;
    MemConsoleInputRouteTargetPolicy target_policy;
    uint32_t routed_global_count;
    uint32_t routed_ui_count;
    uint32_t routed_fallback_count;
} MemConsoleInputRouteResult;

typedef struct MemConsoleInputInvalidationResult {
    uint8_t full_invalidate;
    uint32_t invalidation_reason_bits;
    uint32_t target_invalidation_count;
    uint32_t full_invalidation_count;
} MemConsoleInputInvalidationResult;

typedef struct MemConsoleInputFrame {
    MemConsoleInputEventRaw raw;
    MemConsoleInputRouteResult route;
    MemConsoleInputInvalidationResult invalidation;
} MemConsoleInputFrame;

typedef struct MemConsoleInputDiagTotals {
    uint64_t frame_count;
    uint64_t event_count_total;
    uint64_t routed_global_total;
    uint64_t routed_ui_total;
    uint64_t routed_fallback_total;
    uint64_t invalidation_reason_bits_total;
} MemConsoleInputDiagTotals;

typedef struct MemConsoleRenderDiagTotals {
    uint64_t frame_count;
    uint64_t submit_ok_count;
    uint64_t submit_recoverable_count;
    uint64_t submit_fatal_count;
} MemConsoleRenderDiagTotals;

typedef enum MemConsoleLoopInputPhaseResult {
    MEM_CONSOLE_LOOP_INPUT_PHASE_OK = 0,
    MEM_CONSOLE_LOOP_INPUT_PHASE_EXIT = 1
} MemConsoleLoopInputPhaseResult;

typedef struct MemConsoleLoopRunState {
    bool running;
    int frame_width;
    int frame_height;
    int last_frame_width;
    int last_frame_height;
    KitUiInputState input;
    MemConsoleInputDiagTotals ir1_diag_totals;
    MemConsoleRenderDiagTotals rs1_diag_totals;
} MemConsoleLoopRunState;

typedef struct MemConsoleLoopFrameState {
    SDL_Event event;
    MemConsoleInputFrame input_frame;
    MemConsoleAction keyboard_action;
    MemConsoleAction ui_action;
    MemConsoleAction pending_action;
    int frame_result;
    uint32_t frame_reasons;
    uint32_t idle_wait_ms;
    uint64_t frame_begin_ms;
    int waited_event;
    int wait_performed;
    uint32_t wait_call_count;
    uint32_t wait_blocked_ms;
    int wheel_y;
} MemConsoleLoopFrameState;

int mem_console_app_loop_env_flag_enabled(const char *name);

void mem_console_input_frame_begin(MemConsoleInputFrame *frame);

MemConsoleLoopInputPhaseResult mem_console_loop_input_phase(const MemConsoleAppLoopContext *ctx,
                                                           MemConsoleLoopRunState *run_state,
                                                           MemConsoleLoopFrameState *frame);

#endif
