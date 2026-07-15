// EPT torture test — host-compilable, no hardware.
//
// Drives the EPT core through virtual time: whenever every thread reports
// EPT_BLOCKED (the MCU would WFI), the test advances ept_tc by one tick.
// A thread stuck ACTIVE (livelock) or BLOCKED past its deadline fails fast.
//
// Build & run:
//   gcc -Wall -Wextra -Werror -Wno-implicit-fallthrough -o torture torture_test.c && ./torture
// (-Wno-implicit-fallthrough: the case-label fallthrough is inherent to switch-based LC)

#define EPT_TICK_FREQ_HZ 1000
#define EPT_IMPLEMENTATION
#include "../ept.h"

#include <stdio.h>

static int failures;
#define CHECK(cond)                                                  \
    do {                                                             \
        if (!(cond)) {                                               \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            failures++;                                              \
        }                                                            \
    } while (0)

typedef char (*thread_fn)(struct ept *);

// Run one thread the way the scheduler would: ACTIVE = call again immediately,
// BLOCKED = advance virtual time one tick. Returns 1 when DONE, 0 on livelock.
static int run_to_done(thread_fn fn, struct ept *e, uint32_t guard_iters)
{
    while (guard_iters--) {
        char s = fn(e);
        if (s == EPT_DONE) return 1;
        if (s == EPT_BLOCKED) ept_tc++;
    }
    return 0;
}

// ---- 1. EPT_SLEEP wakes on time, reports BLOCKED while waiting ---------------

static uint32_t sleeper_count;
EPT_THREAD(t_sleeper(struct ept *ept))
{
    EPT_BEGIN(ept);
    while (1) {
        EPT_SLEEP(ept, 100);
        sleeper_count++;
    }
    EPT_END(ept);
}

static void test_sleep(void)
{
    struct ept e;
    EPT_INIT(&e);
    ept_tc = 0;
    sleeper_count = 0;
    for (int i = 0; i < 1000; i++) {
        char s = t_sleeper(&e);
        CHECK(s == EPT_BLOCKED); // never ACTIVE: pure timed sleep
        ept_tc++;
    }
    CHECK(sleeper_count == 9); // calls at tc=0..999, wakes at tc=100,200,...,900
    if (sleeper_count != 9)
        printf("  sleeper_count = %u\n", (unsigned)sleeper_count);
}

// ---- 2. SPAWN propagates child status (the big fix) ---------------------------

static struct ept child_e;
static int parent_done;

EPT_THREAD(t_child(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_SLEEP(ept, 50);
    EPT_YIELD(ept);
    EPT_YIELD(ept);
    EPT_END(ept);
}

EPT_THREAD(t_parent(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_SPAWN(ept, &child_e, t_child(&child_e));
    parent_done = 1;
    EPT_END(ept);
}

static void test_spawn_propagation(void)
{
    struct ept e;
    EPT_INIT(&e);
    ept_tc = 0;
    parent_done = 0;

    CHECK(t_parent(&e) == EPT_BLOCKED); // child sleeping -> parent BLOCKED (MCU may sleep)
    ept_tc += 49;
    CHECK(t_parent(&e) == EPT_BLOCKED); // still sleeping
    ept_tc += 1;
    CHECK(t_parent(&e) == EPT_ACTIVE);  // child woke, yielded -> parent ACTIVE
    CHECK(t_parent(&e) == EPT_ACTIVE);  // second yield
    CHECK(t_parent(&e) == EPT_DONE);    // child done -> parent runs to completion
    CHECK(parent_done == 1);
}

// ---- 3. Nested spawn: status climbs all levels --------------------------------

static struct ept mid_e, leaf_e;

EPT_THREAD(t_leaf(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_SLEEP(ept, 30);
    EPT_END(ept);
}

EPT_THREAD(t_mid(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_SPAWN(ept, &leaf_e, t_leaf(&leaf_e));
    EPT_END(ept);
}

EPT_THREAD(t_top(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_SPAWN(ept, &mid_e, t_mid(&mid_e));
    EPT_END(ept);
}

static void test_nested_spawn(void)
{
    struct ept e;
    EPT_INIT(&e);
    ept_tc = 0;
    CHECK(t_top(&e) == EPT_BLOCKED); // leaf sleeps two levels down
    ept_tc += 30;
    CHECK(t_top(&e) == EPT_DONE);
}

// ---- 4. WAIT_UNTIL blocks, completes when condition set -----------------------

static int flag4;
EPT_THREAD(t_waiter(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_WAIT_UNTIL(ept, flag4);
    EPT_END(ept);
}

static void test_wait_until(void)
{
    struct ept e;
    EPT_INIT(&e);
    flag4 = 0;
    CHECK(t_waiter(&e) == EPT_BLOCKED);
    CHECK(t_waiter(&e) == EPT_BLOCKED);
    flag4 = 1;
    CHECK(t_waiter(&e) == EPT_DONE);
}

// ---- 5. WAIT_UNTIL_TIMEOUT: both exits ----------------------------------------

static int flag5;
static int got_cond5;
EPT_THREAD(t_timeout(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_WAIT_UNTIL_TIMEOUT(ept, flag5, 80);
    got_cond5 = flag5;
    EPT_END(ept);
}

static void test_wait_timeout(void)
{
    struct ept e;

    // a) timeout path
    EPT_INIT(&e);
    ept_tc = 0;
    flag5 = 0;
    CHECK(t_timeout(&e) == EPT_BLOCKED);
    ept_tc += 79;
    CHECK(t_timeout(&e) == EPT_BLOCKED);
    ept_tc += 1;
    CHECK(t_timeout(&e) == EPT_DONE);
    CHECK(got_cond5 == 0);
    CHECK(!ept_timer_armed(&e.t)); // timer disarmed after use

    // b) condition path (early)
    EPT_INIT(&e);
    ept_tc = 0;
    flag5 = 0;
    CHECK(t_timeout(&e) == EPT_BLOCKED);
    ept_tc += 10;
    flag5 = 1;
    CHECK(t_timeout(&e) == EPT_DONE);
    CHECK(got_cond5 == 1);
}

// ---- 6. YIELD returns ACTIVE exactly once per yield ----------------------------

EPT_THREAD(t_yielder(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_YIELD(ept);
    EPT_YIELD(ept);
    EPT_YIELD(ept);
    EPT_END(ept);
}

static void test_yield(void)
{
    struct ept e;
    EPT_INIT(&e);
    CHECK(t_yielder(&e) == EPT_ACTIVE);
    CHECK(t_yielder(&e) == EPT_ACTIVE);
    CHECK(t_yielder(&e) == EPT_ACTIVE);
    CHECK(t_yielder(&e) == EPT_DONE); // no time ever advanced
}

// ---- 7. YIELD_UNTIL: one forced yield, then BLOCKED until condition ------------

static int flag7;
EPT_THREAD(t_yield_until(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_YIELD_UNTIL(ept, flag7);
    EPT_END(ept);
}

static void test_yield_until(void)
{
    struct ept e;
    EPT_INIT(&e);
    flag7 = 0;
    CHECK(t_yield_until(&e) == EPT_ACTIVE);  // the forced yield
    CHECK(t_yield_until(&e) == EPT_BLOCKED); // now waiting: MCU may sleep
    CHECK(t_yield_until(&e) == EPT_BLOCKED);
    flag7 = 1;
    CHECK(t_yield_until(&e) == EPT_DONE);

    // condition already true: still yields exactly once
    EPT_INIT(&e);
    CHECK(t_yield_until(&e) == EPT_ACTIVE);
    CHECK(t_yield_until(&e) == EPT_DONE);
}

// ---- 8. RESTART re-runs the body from EPT_BEGIN --------------------------------

static int entries8;
static int flag8;
EPT_THREAD(t_restarter(struct ept *ept))
{
    EPT_BEGIN(ept);
    entries8++;
    EPT_WAIT_UNTIL(ept, flag8);
    EPT_RESTART(ept);
    EPT_END(ept);
}

static void test_restart(void)
{
    struct ept e;
    EPT_INIT(&e);
    entries8 = flag8 = 0;
    CHECK(t_restarter(&e) == EPT_BLOCKED);
    CHECK(entries8 == 1);
    flag8 = 1;
    CHECK(t_restarter(&e) == EPT_ACTIVE); // RESTART reports ACTIVE: run me again now
    flag8 = 0;
    CHECK(t_restarter(&e) == EPT_BLOCKED);
    CHECK(entries8 == 2); // body re-entered from the top
}

// ---- 9. Timer survives uint32 tick-counter wrap --------------------------------

static void test_wrap(void)
{
    struct ept e;
    EPT_INIT(&e);
    ept_tc = 0xFFFFFF00u; // 256 ms before wrap
    sleeper_count = 0;
    for (int i = 0; i < 1000; i++) {
        CHECK(t_sleeper(&e) == EPT_BLOCKED);
        ept_tc++;
    }
    CHECK(sleeper_count == 9); // 100 ms period straight across the wrap
}

// ---- 10. Child finishing on first call: parent continues same pass -------------

static struct ept instant_e;
EPT_THREAD(t_instant(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_END(ept);
}

EPT_THREAD(t_spawn_instant(struct ept *ept))
{
    EPT_BEGIN(ept);
    EPT_SPAWN(ept, &instant_e, t_instant(&instant_e));
    EPT_YIELD(ept);
    EPT_END(ept);
}

static void test_spawn_instant(void)
{
    struct ept e;
    EPT_INIT(&e);
    CHECK(t_spawn_instant(&e) == EPT_ACTIVE); // reached own YIELD in the same call
    CHECK(t_spawn_instant(&e) == EPT_DONE);
}

// ---- 11. Long randomized-ish run under the virtual scheduler -------------------

static void test_long_run(void)
{
    struct ept e;
    EPT_INIT(&e);
    ept_tc = 0;
    parent_done = 0;
    CHECK(run_to_done(t_parent, &e, 100000));

    EPT_INIT(&e);
    ept_tc = 0xFFFFFFF0u; // spawn chain across the wrap
    CHECK(run_to_done(t_top, &e, 100000));
}

// ---- main -----------------------------------------------------------------------

int main(void)
{
    test_sleep();
    test_spawn_propagation();
    test_nested_spawn();
    test_wait_until();
    test_wait_timeout();
    test_yield();
    test_yield_until();
    test_restart();
    test_wrap();
    test_spawn_instant();
    test_long_run();

    if (failures) {
        printf("torture test: %d FAILURE(S)\n", failures);
        return 1;
    }
    printf("torture test: all passed\n");
    return 0;
}
