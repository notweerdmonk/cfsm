#include <iostream>
#include <thread>
#include <chrono>
#include <cfsm.hpp>

using namespace cfsm;

class state_a final : public state {
  static
  const std::size_t type_id_;

public:
  static
  std::size_t type_id() {
    return state_a::type_id_;
  }

  void on_enter(void *dataptr) const override {
  }

  void on_exit(void *dataptr) const override {
  }
};

class state_b final : public state {
  static
  const std::size_t type_id_;

public:
  static
  std::size_t type_id() {
    return state_b::type_id_;
  }

  void on_enter(void *dataptr) const override {
  }

  void on_exit(void *dataptr) const override {
  }
};
#if __cplusplus >= 201402L
const std::size_t state_a::type_id_ =
  state_machine<
    state,
    alloc_type::INTERNAL,
    nullptr,
    state_a,
    state_b
  >::gen_type_id();
#else
const std::size_t state_a::type_id_ =
  state_machine<
    state,
    alloc_type::INTERNAL,
    nullptr,
    2
  >::gen_type_id();
#endif
#if __cplusplus >= 201402L
const std::size_t state_b::type_id_ =
  state_machine<
    state,
    alloc_type::INTERNAL,
    nullptr,
    state_a,
    state_b
  >::gen_type_id();
#else
const std::size_t state_b::type_id_ =
  state_machine<
    state,
    alloc_type::INTERNAL,
    nullptr,
    2
  >::gen_type_id();
#endif

CFSM_TRANSITION(state_a, state_b) {
}

CFSM_TRANSITION(state_b, state_a) {
}

/* Static state storage */
static struct _state_storage {
  state_a a;
  state_b b;
} state_storage = {
  state_a(),
  state_b()
};

static state* state_pool[2] = {
  &state_storage.a,
  &state_storage.b
};

void benchmark_state_machine_lazy(int num_transitions) {
  std::cout << "Lazy allocation\n";

#if __cplusplus >= 201402L
  state_machine<state, alloc_type::LAZY, nullptr, state_a, state_b> fsm;
#else
  state_machine<state, alloc_type::LAZY, nullptr, 2> fsm;
#endif

  fsm.start<state_a>(nullptr);

  /* Warmup */
  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  std::cout << num_transitions << " transitions took "
            << elapsed.count() << " seconds." << std::endl;

  std::cout << "Avg time per transition: "
    << 1000000 * elapsed.count() / num_transitions << " microseconds\n";
}

void benchmark_state_machine_internal(int num_transitions) {
  std::cout << "Internal preallocation\n";

#if __cplusplus >= 201402L
  state_machine<state, alloc_type::INTERNAL, nullptr, state_a, state_b> fsm;

  fsm.start<state_a>(nullptr);

  /* Warmup */
  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  std::cout << num_transitions << " transitions took "
    << elapsed.count() << " seconds\n";

  std::cout << "Avg time per transition: "
    << 1000000 * elapsed.count() / num_transitions << " microseconds\n";

#else

#warning Cannot benchmark internal preallocated storage for versions below C++14
  std::cerr << "Cannot benchmark internal preallocated storage for versions"
    "below C++14\n";
#endif

}

void benchmark_state_machine_internal_static(int num_transitions) {
  std::cout << "Internal preallocation with objects in static array\n";

#if __cplusplus >= 201402L
  state_machine<state, alloc_type::STATIC, nullptr, state_a, state_b> fsm;

  fsm.start<state_a>(nullptr);

  /* Warmup */
  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  std::cout << num_transitions << " transitions took "
    << elapsed.count() << " seconds\n";

  std::cout << "Avg time per transition: "
    << 1000000 * elapsed.count() / num_transitions << " microseconds\n";

#else

#warning Cannot test internal statically preallocated storage for versions below C++14
  std::cerr << "Cannot test internal statically preallocated storage for"
    "versions below C++14\n";

#endif

}

void benchmark_state_machine_external(int num_transitions) {
  std::cout << "External preallocation\n";

#if __cplusplus >= 201402L
  state_machine<state, alloc_type::PREALLOCED, state_pool, state_a, state_b> fsm;
#else
  state_machine<state, alloc_type::PREALLOCED, state_pool, 2> fsm;
#endif

  fsm.start<state_a>(nullptr);

  /* Warmup */
  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto start_time = std::chrono::high_resolution_clock::now();

  for (int i = 0; i < num_transitions; ++i) {
    if (i % 2 == 0) {
      fsm.transition<state_a, state_b>(nullptr);
    } else {
      fsm.transition<state_b, state_a>(nullptr);
    }
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  std::cout << num_transitions << " transitions took "
    << elapsed.count() << " seconds\n";

  std::cout << "Avg time per transition: "
    << 1000000 * elapsed.count() / num_transitions << " microseconds\n";
}

void benchmark_concurrent_state_machine_lazy(
    int num_transitions, int num_threads
) {
  std::cout << "Concurrent lazy allocation\n";

#if __cplusplus >= 201402L
  state_machine<state, alloc_type::LAZY, nullptr, state_a, state_b> fsm;
#else
  state_machine<state, alloc_type::LAZY, nullptr, 2> fsm;
#endif

  std::vector<std::thread> threads;
  fsm.start<state_a>(nullptr);

  /* Warmup */
  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(
        [&] {
          for (int i = 0; i < num_transitions / num_threads; ++i) {
            if (i % 2 == 0) {
              while (fsm.state<state_a>() == nullptr);
              fsm.transition<state_a, state_b>(nullptr);
            } else {
              while (fsm.state<state_b>() == nullptr);
              fsm.transition<state_b, state_a>(nullptr);
            }
          }
        }
    );
  }

  for (auto &t : threads) {
    t.join();
  }
  fsm.stop(nullptr);

  threads.clear();

  auto start_time = std::chrono::high_resolution_clock::now();

  fsm.start<state_a>(nullptr);

  for (int t = 0; t < num_threads; ++t) {
    threads.emplace_back(
        [&] {
          for (int i = 0; i < num_transitions / num_threads; ++i) {
            if (i % 2 == 0) {
              while (fsm.state<state_a>() == nullptr);
              fsm.transition<state_a, state_b>(nullptr);
            } else {
              while (fsm.state<state_b>() == nullptr);
              fsm.transition<state_b, state_a>(nullptr);
            }
          }
        }
    );
  }

  for (auto &t : threads) {
    t.join();
  }

  fsm.stop(nullptr);

  auto end_time = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end_time - start_time;

  std::cout << num_transitions << " transitions took "
            << elapsed.count() << " seconds." << std::endl;

  std::cout << "Avg time per transition: "
    << 1000000 * elapsed.count() / num_transitions << " microseconds\n";

  threads.clear();
}

static inline uint64_t rdtsc() {
    uint32_t lo, hi;
    asm volatile ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}

void calculate_cpu_clock_speed() {
    uint64_t start, end;
    std::chrono::duration<double> elapsed;

    // Get TSC and time at start
    start = rdtsc();
    auto start_time = std::chrono::high_resolution_clock::now();

    // Busy-wait loop (~1 ms)
    do {
      auto end_time = std::chrono::high_resolution_clock::now();
      elapsed = end_time - start_time;
    } while (elapsed.count() < 0.001);

    end = rdtsc();

    double fcpu_hz = (end - start) / elapsed.count();
    double fcpu_ghz = fcpu_hz / 1e9;

    std::cout << "CPU freq: " << fcpu_ghz << " GHz\n";
}

int main() {
  std::cout << "Compile-time state machine benchmark\n";
  calculate_cpu_clock_speed();
  benchmark_state_machine_lazy(8000000);
  benchmark_state_machine_external(8000000);
  benchmark_state_machine_internal(8000000);
  benchmark_state_machine_internal_static(8000000);
  benchmark_concurrent_state_machine_lazy(8000000, 8);

  return 0;
}
