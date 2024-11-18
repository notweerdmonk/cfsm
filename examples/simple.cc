#include <iostream>
#include <cfsm.hpp>

using namespace cfsm;

class state_a final : public state {
public:
  void on_enter(void *dataptr) const override {
    std::cout << "Entering state A\n";
  }

  void on_exit(void *dataptr) const override {
    std::cout << "Exiting state A\n";
  }
};

class state_b final : public state {
public:
  void on_enter(void *dataptr) const override {
    std::cout << "Entering state B\n";
  }

  void on_exit(void *dataptr) const override {
    std::cout << "Exiting state B\n";
  }
};

class state_c final : public state {
public:
  void on_enter(void *dataptr) const override {
    std::cout << "Entering state C\n";
  }

  void on_exit(void *dataptr) const override {
    std::cout << "Exiting state C\n";
  }
};

/* Specialize transition for state_a -> state_b */
TRANSITION_BEGIN(state_a, state_b)
  void operator()(void *dataptr) {
    std::cout << "Transitioning from state A to state B\n";
  }
TRANSITION_END;

/* Specialize transition for state_b -> state_a */
TRANSITION_BEGIN(state_b, state_a)
  void operator()(void *dataptr) const {
    std::cout << "Transitioning from state B to state A\n";
  }
TRANSITION_END;

/* Specialize transition for state_a -> state_c */
TRANSITION_BEGIN(state_a, state_c)
  void operator()(void *dataptr) const {
    std::cout << "Transitioning from state A to state C\n";
  }
TRANSITION_END;

/* dummy state */
class state_foo {
  public:
    int n = 0xfade;
};

class state_1 final : public state {
  static
  const std::size_t type_id_;
public:
  static
  std::size_t type_id() {
    return state_1::type_id_;
  }

  void on_enter(void *dataptr) const override {
    std::cout << "Entering state 1\n";
  }

  void on_exit(void *dataptr) const override {
    std::cout << "Exiting state 1\n";
  }
};

class state_2 final : public state {
  static
  const std::size_t type_id_;
public:
  static
  std::size_t type_id() {
    return state_2::type_id_;
  }

  void on_enter(void *dataptr) const override {
    std::cout << "Entering state 2\n";
  }

  void on_exit(void *dataptr) const override {
    std::cout << "Exiting state 2\n";
  }
};

/* Static class members */
/*
 * The template specialization of state_machine::gen_type_id uses
 * alloc_type::LAZY for generating type identifiers for derived state classes.
 * This approach ensures that the same state classes are used regardless of the
 * allocation scheme. The key point is that type identifiers remain consistent
 * (or identical) across different allocation schemes, meaning that the indices
 * used for state objects are the same in both externally and internally managed
 * arrays.
 *
 * The differentiation between allocation schemes exists to allow for different
 * allocation behaviors, but it does not affect the generation of type
 * identifiers for the state classes themselves. Care should be taken to avoid
 * mixing allocation schemes when generating type identifiers, as this could
 * lead to inconsistencies in the mapping between state objects and their
 * identifiers.
 */

#if __cplusplus >= 201402L
const std::size_t state_1::type_id_ =
  state_machine<
    state,
    alloc_type::LAZY,
    nullptr,
    state_a,
    state_b,
    state_c
  >::gen_type_id();
#else
const std::size_t state_1::type_id_ =
  state_machine<
    state,
    alloc_type::LAZY,
    nullptr,
    2
  >::gen_type_id();
#endif
#if __cplusplus >= 201402L
const std::size_t state_2::type_id_ =
  state_machine<
    state,
    alloc_type::LAZY,
    nullptr,
    state_a,
    state_b,
    state_c
  >::gen_type_id();
#else
const std::size_t state_2::type_id_ =
  state_machine<
    state,
    alloc_type::LAZY,
    nullptr,
    2
  >::gen_type_id();
#endif

template<>
struct cfsm::transition<state_1, state_2> {
  TRANSITION_EXISTS;
  void operator()(void *dataptr) {
    std::cout << "Transitioning from state 1 to state 2\n";
  }
};

template<>
struct cfsm::transition<state_2, state_1> {
  TRANSITION_EXISTS;
  void operator()(void *dataptr) {
    std::cout << "Transitioning from state 2 to state 1\n";
  }
};

/* Static state storage */
struct _state_storage {
  state_1 s1;
  state_2 s2;
} state_storage = {
  state_1(),
  state_2()
};

state* state_pool[2] = {
  &state_storage.s1,
  &state_storage.s2
};

void test_lazy_allocator() {
#if __cplusplus >= 201402L
  //state_machine<
  //  state,
  //  alloc_type::LAZY,
  //  nullptr,
  //  state_a,
  //  state_b,
  //  state_c
  //> fsm;
  state_machine_lazy<
    state,
    nullptr,
    state_a,
    state_b,
    state_c
  > fsm;
#else
  state_machine<state> fsm;
#endif

  /* Start in state_a; on_entry of initial state will get called */
  fsm.start<state_a>(nullptr);

  /* Transition to state_b */
  std::cout << "* State A to state B\n";
  assert((fsm.transition<state_a, state_b>(nullptr)));

  /* Transition from state_a to state_c - will fail */
  std::cout << "* State A to state C - will fail\n";

  /* Check if state is state_a */
  if (fsm.state<state_a>() != nullptr) {
    assert((fsm.transition<state_a, state_c>(nullptr) == false));
  } else {
    std::cerr << "State machine is not in state A\n";
    return;
  }

  /* Transition back to state_a */
  std::cout << "* State B to state A\n";
  assert((fsm.transition<state_b, state_a>(nullptr)));

  /* Transition to state_c */
  std::cout << "* State A to state C\n";
  assert((fsm.transition<state_a, state_c>(nullptr)));

  /* Transition from state_c to state_a - compile error */
  //fsm.transition<state_c, state_a>();

  /* Stop the state machine; on_exit of current_state is callled */
  fsm.stop(nullptr);
}

void test_preallocated_static() {
  /* Provide preallocated array of state object pointers */
#if __cplusplus >= 201402L
  //state_machine<
  //  state,
  //  alloc_type::PREALLOCED,
  //  state_pool,
  //  state_1,
  //  state_2
  //> fsm;
  state_machine_ext<
    state,
    state_pool,
    state_1,
    state_2
  > fsm;
#else
  //state_machine<
  //  state,
  //  alloc_type::PREALLOCED,
  //  state_pool,
  //  sizeof(state_pool)
  //> fsm;
  state_machine_ext<
    state,
    state_pool,
    sizeof(state_pool)
  > fsm;
#endif

  fsm.start<state_1>(nullptr);

  /* Check if state is state_1 */
  if (fsm.state<state_1>() != nullptr) {
    fsm.transition<state_1, state_2>(nullptr);
  }

  fsm.transition<state_2, state_1>(nullptr);

  fsm.stop(nullptr);
}

void test_preallocated_dynamic() {
  /* Allocate state objects on heap */
  state_pool[0] = new state_1;
  state_pool[1] = new state_2;

  /* Provide array of state object pointers */
#if __cplusplus >= 201402L
  //state_machine<
  //  state,
  //  alloc_type::PREALLOCED,
  //  state_pool,
  //  state_1,
  //  state_2
  //> fsm;
  state_machine_ext<
    state,
    state_pool,
    state_1,
    state_2
  > fsm;
#else
  //state_machine<
  //  state,
  //  alloc_type::PREALLOCED,
  //  state_pool,
  //  sizeof(state_pool)
  //> fsm;
  state_machine_ext<
    state,
    state_pool,
    sizeof(state_pool)
  > fsm;
#endif

  fsm.start<state_1>(nullptr);

  /* Check if state is state_1 */
  if (fsm.state<state_1>() != nullptr) {
    fsm.transition<state_1, state_2>(nullptr);
  }

  fsm.transition<state_2, state_1>(nullptr);

  fsm.stop(nullptr);

  delete state_pool[0];
  delete state_pool[1];
}

void test_preallocated_internal() {
  /* Use internally allocated state objects */
#if __cplusplus >= 201402L
  //state_machine<
  //  state,
  //  alloc_type::INTERNAL,
  //  nullptr,
  //  state_1,
  //  state_2
  //> fsm;
  state_machine_int<
    state,
    nullptr,
    state_1,
    state_2
  > fsm;

  fsm.start<state_1>(nullptr);

  /* Check if state is state_1 */
  if (fsm.state<state_1>() != nullptr) {
    fsm.transition<state_1, state_2>(nullptr);
  }

  fsm.transition<state_2, state_1>(nullptr);

  fsm.stop(nullptr);

#else
#warning Cannot test internal preallocated storage for versions below C++14
  std::cerr << "Cannot test internal preallocated storage for versions below"
    "C++14\n";
#endif
}

void test_preallocated_internal_static() {
  /* Use internally allocated state objects in static array */
#if __cplusplus >= 201402L
  state_machine<
    state,
    alloc_type::STATIC,
    nullptr,
    state_1,
    state_2
  > fsm;
  //state_machine_static<
  //  state,
  //  nullptr,
  //  state_1,
  //  state_2
  //> fsm;

  fsm.start<state_1>(nullptr);

  /* Check if state is state_1 */
  if (fsm.state<state_1>() != nullptr) {
    fsm.transition<state_1, state_2>(nullptr);
  }

  fsm.transition<state_2, state_1>(nullptr);

  fsm.stop(nullptr);

#else
#warning Cannot test internal statically preallocated storage for versions below C++14
  std::cerr << "Cannot test internal statically preallocated storage for"
    "versions below C++14\n";
#endif
}

void test_serialization() {
#if __cplusplus >= 201402L
  //state_machine<
  //  state,
  //  alloc_type::INTERNAL,
  //  nullptr,
  //  state_1,
  //  state_2
  //> fsm, fsm_copy;
  state_machine_int<
    state,
    nullptr,
    state_1,
    state_2
  > fsm, fsm_copy;
#else
  //state_machine<
  //  state,
  //  alloc_type::LAZY,
  //  nullptr,
  //  state_1,
  //  state_2
  //> fsm, fsm_copy;
  state_machine_lazy<
    state,
    nullptr,
    2
  > fsm, fsm_copy;
#endif

  fsm.start<state_1>(nullptr);

  /* Check if state is state_1 */
  if (fsm.state<state_1>() != nullptr) {
    fsm.transition<state_1, state_2>(nullptr);
  }

  std::cout << "Saving state machine\n";
  long serialized_data;
  /* original state machine is unusable until load is called */
  assert(fsm.save(reinterpret_cast<char*>(&serialized_data),
      sizeof(serialized_data)) == sizeof(serialized_data));

  std::cout << "Loading state machine\n";
  assert(fsm_copy.load(reinterpret_cast<char*>(&serialized_data),
      sizeof(serialized_data)) == sizeof(serialized_data));

  fsm_copy.transition<state_2, state_1>(nullptr);

  fsm_copy.stop(nullptr);
}

int main() {
  std::cout << "Lazy allocator test\n\n";
  test_lazy_allocator();

  std::cout << "\nPreallocated test 1: static storage\n\n";
  test_preallocated_static();

  std::cout << "\nPreallocated test 1: dynamic storage\n\n";
  test_preallocated_dynamic();

  std::cout << "\nInternally allocated state objects test\n\n";
  test_preallocated_internal();

  std::cout << "\nInternally allocated state objects in static array test\n\n";
  test_preallocated_internal_static();

  std::cout << "\nSerialization test\n\n";
  test_serialization();

  return 0;
}
