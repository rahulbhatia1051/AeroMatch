#include <benchmark/benchmark.h>

#include "OrderBook.h"

// These measure end-to-end processing throughput for deterministic synthetic
// streams. They do not establish p99/p99.9 or a sub-microsecond worst-case
// latency guarantee; that needs a dedicated latency harness and CPU pinning.
static void BM_AeroMatch_MatchThroughput(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        const uint64_t num_orders = state.range(0);
        OrderBook engine(num_orders);
        state.ResumeTiming();

        for (uint64_t i = 0; i < num_orders; ++i) {
            const bool is_buy = i % 2 == 0;
            const uint64_t price = 50000 + i % 100;
            engine.add_order(i, price, 10, is_buy);
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// A deep single-price FIFO queue stresses arbitrary intrusive-list removals.
static void BM_AeroMatch_CancelStorm(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        const uint64_t num_orders = state.range(0);
        OrderBook engine(num_orders);
        for (uint64_t i = 0; i < num_orders; ++i) {
            engine.add_order(i, 50000, 10, true);
        }
        state.ResumeTiming();

        for (uint64_t i = 0; i < num_orders; ++i) {
            engine.cancel_order(i);
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

// Seeds a 128-level ask book, then sweeps it with marketable buy limits.
static void BM_AeroMatch_DeepBookSweep(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        const uint64_t num_orders = state.range(0);
        OrderBook engine(num_orders + 1);
        for (uint64_t i = 0; i < num_orders; ++i) {
            engine.add_order(i, 50000 + i % 128, 1, false);
        }
        state.ResumeTiming();

        for (uint64_t i = 0; i < num_orders; ++i) {
            engine.add_order(num_orders + i, 60000, 1, true);
        }
    }
    state.SetItemsProcessed(state.iterations() * state.range(0));
}

#define AEROMATCH_BENCHMARKS(name) \
    BENCHMARK(name)->Arg(10000)->Arg(100000)->Arg(1000000)->UseRealTime()->Unit(benchmark::kMillisecond)

AEROMATCH_BENCHMARKS(BM_AeroMatch_MatchThroughput);
AEROMATCH_BENCHMARKS(BM_AeroMatch_CancelStorm);
AEROMATCH_BENCHMARKS(BM_AeroMatch_DeepBookSweep);

BENCHMARK_MAIN();
