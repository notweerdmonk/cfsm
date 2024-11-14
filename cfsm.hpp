/*
  MIT License
  
  Copyright (c) 2024 notweerdmonk
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
  
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

#ifndef __SMBUILDER_HPP__
#define __SMBUILDER_HPP__

/**
 * @file cfsm.hpp
 * @brief Complie-time finite state machine library.
 * @author notweerdmonk
 */

/**
 * @mainpage Compile-time finite state machine library
 *
 * Browse @link cfsm.hpp @endlink
 *
 * @section header Static disposition
 * The states and transitions between them must be known at compile-time. The
 * library uses templates to implement state transitions.
 *
 * @section states States
 * All user defined states which are represented by classes should inherit
 * from the base "state" class provided in "cfsm.hpp". The base class
 * provides on_enter and on_exit member functions which are called on entry
 * to state and exit from the state respectively. These pure virtual member
 * functions shall be overriden in the user defined classes.
 *
 * The "state_machine" class provides a "state" member function template which
 * will return a pointer to the current state object which is dynamic_cast
 * into the state class provided as the template parameter. When current state
 * is not of the requested type, nullptr is returned. This can be used as a
 * test for the current state of a state machine.
 *
 * @section transitions State transitions
 * Transitions between user defined states are represented by a struct
 * template named "transition". The template parameters are the types of
 * source state class and target state class respectively.
 *
 * User defined template specializations should provide two members. First,
 * a static constexpr const bool member variable named "exists" which should be
 * set as "true"; an helper macro "TRANSITION_EXISTS" is provided for this
 * purpose. Second, an implementation of the "operator()" function, the call
 * operator overload, which returns void and takes a void pointer as its
 * argument. This function shall be called on a valid transition (source and
 * target states are valid classes and the template specialization of
 * "transition" struct for source and target state exists), triggered by
 * calling the "transition" member function of the "state_machine" class.
 * The function signature shall be:
 * 
 *   void operator()(void *dataptr);
 *
 *
 * @section state_machine_types State machine types
 * State machines are classified based on their state object allocation scheme.
 * These are lazily allocated, externally preallocated and internally
 * preallocated. The enum class "alloc_type" passed as a template parameter, 
 * specifies the allocation scheme for a state machine.
 *
 * @section lazy Lazily allocated states
 * If the "state_pool" template parameter is set to nullptr (default) the
 * memory for state objects is dynamically allocated with new operator when
 * state transition occurs (by calling "transition" member function).
 *
 * @section external_prealloc Externally managed preallocated states storage
 * An external array of state objects is required. These state objects will be
 * used by the state machine. The "state_pool" template parameter shall be an
 * array of pointers of the base "state" class, having size equal to the number
 * of states. The array is indexed into by the type identifiers of respective
 * state classes enabling constant time access. Base class pointers should point
 * to objects of derived state classes. Memory for these derived state class
 * objects shall be managed by user.
 *
 * @section internal_prealloc Internally managed preallocated state storage
 * The array of preallocated state objects can be managed by the state machine
 * internally. Type identifiers of the derivedstate classes will be used to
 * index into this array. The state objects' lifetimes span the lifetime of the
 * state machine object. This scheme offers conveniece at the cost of reduced
 * flexibility.
 *
 * @section type_id Type identifier for derived state classes
 * The type identifier of a derived state class is an unsigned integer of type
 * as std::size_t. Provide a static member function named "type_id" to each
 * derived state class which returns the type identifier.
 * The function signature shall be:
 *
 *   static std::size_t type_id();
 *
 *
 * See example programs for type identifier schemes. A static helper function
 * named "gen_type_id" which returns a std::size_t value based on an
 * incremented static variable is provided for the "state_machine" class.
 *
 * @see cfsm.hpp
 */

#include <type_traits>
#include <sstream>
#include <cassert>

namespace cfsm {

#if __cplusplus < 201703L
  
#if __cplusplus >= 201402L

  /* C++11 alternatives for std::disjunction and std::conjunction */
  template <typename...>
  struct disjunction : std::false_type {};
  
  template <typename B1>
  struct disjunction<B1> : B1 {};
  
  template <typename B1, typename... Bn>
  struct disjunction<B1, Bn...>
      : std::conditional<B1::value, B1, disjunction<Bn...>>::type {};
  
  template <typename... B>
  constexpr bool disjunction_v = disjunction<B...>::value;
  
  template <typename...>
  struct conjunction : std::true_type {};
  
  template <typename B1>
  struct conjunction<B1> : B1 {};
  
  template <typename B1, typename... Bn>
  struct conjunction<B1, Bn...>
      : std::conditional<B1::value, conjunction<Bn...>, B1>::type {};
  
  template <typename... B>
  constexpr bool conjunction_v = conjunction<B...>::value;

#endif /* __cplusplus >= 201402L */
  
#else /* __cplusplus < 201703L */

  /* For C++17 and above */
  using std::disjunction_v;
  using std::conjunction_v;

#endif /* __cplusplus < 201703L */

  /* Base class for states, all states should inherit from this */
  /**
   * @brief Abstract base class representing a state in the state machine.
   * 
   * Any state in the state machine should inherit from this class and implement
   * the `on_enter` and `on_exit` member functions to define behavior when the
   * state machine enters or exits that state respectively.
   */
  class state {

  public:
    /**
     * @brief Called when the state machine enters the state.
     * 
     * This member function should be overridden in derived state classes to
     * define specific behavior that should happen when the state machine enters
     * the state.
     *
     * @param dataptr Opaque pointer to user data.
     */
    virtual void on_enter(void *dataptr) const = 0;

    /**
     * @brief Called when the state machine exits the state.
     * 
     * This member function should be overridden in derived state classes to
     * define specific behavior that should happen when the state machine exits
     * the state.
     *
     * @param dataptr Opaque pointer to user data.
     */
    virtual void on_exit(void *dataptr) const = 0;

    /// Virtual destructor for safe polymorphic deletion.
    virtual ~state() = default;
  };
  
  /* Transition functor (generic fallback) */
  /**
   * @brief Functor template to handle state transitions.
   * 
   * This is a generic fallback for transitions that are not explicitly
   * specialized. It asserts a false condition indicating that the transition
   * is invalid.
   * 
   * @tparam from_state The state type representing the current state.
   * @tparam to_state The state type representing the target state.
   */
  template <typename from_state, typename to_state>
  struct transition {
    /**
     * @brief Static constant bool member variable which indicates a valid
     * transition between states, if its value is `true`.
     */
    static constexpr const bool exists = false;

    /**
     * @brief Helper macro to define "exists" static member variable as true.
     */
    #define TRANSITION_EXISTS static constexpr const bool exists = true

    /**
     * @brief Functor call operator for the transition.
     * 
     * This operator is invoked during a state transition. The default behavior
     * is to print an invalid transition message, unless specialized for
     * specific state transitions.
     *
     * @param dataptr Opaque pointer to user data.
     */
    void operator()(void *dataptr) {
      assert((0, "Generic transition between states"));
    }
  };

  /**
   * @brief Helper macro to begin definition of transition between `from` and
   * `to` states.
   *
   * User needs to implement the `operator()` member function.
   *
   * TRANSITION_BEGIN(from, to)
   *   void operator()() {
   *
   *   }
   * TRANSITION_END
   *
   * @see TRANSITION_END
   */
  #define TRANSITION_BEGIN(from, to) \
    template <> \
    struct cfsm::transition<from, to> { \
      TRANSITION_EXISTS;

  /**
   * @brief Helper macro to end definition of transition between `from` and
   * `to` states.
   *
   * @see TRANSITION_BEGIN
   */
  #define TRANSITION_END \
    };

  
  /**
   * @brief Enum which specifies state objects allocation scheme for a state
   * machine.
   */
  enum class alloc_type {
    LAZY,         ///< Lazily allocated state objects
    PREALLOCED,   ///< User managed preallocated state objects array
    INTERNAL      ///< Self managed preallocated state objects array
  };

  /* Finite state machine class */

#if __cplusplus >= 201402L

  /**
   * @brief Template class representing a finite state machine.
   * 
   * This state machine class manages transitions between states and calls
   * appropriate `on_enter` and `on_exit` member functions for the states.
   * States must derive from a base state class.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam type Enum specifying the state object allocation scheme.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam states List of state classes present in the state machine.
   */
  template <
    typename base_state,
    enum alloc_type type = alloc_type::LAZY,
    base_state* state_pool[] = nullptr,
    typename... states
  >

#else /* __cplusplus >= 201402L */

  /**
   * @brief Template class representing a finite state machine.
   * 
   * This state machine class manages transitions between states and calls
   * appropriate * `on_enter` and `on_exit` member functions for the states.
   * States must derive from a base state class.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam type Enum specifying the state object allocation scheme.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam state_count Number of states present in the state machine.
   */
  template <
    typename base_state,
    enum alloc_type type = alloc_type::LAZY,
    base_state* state_pool[] = nullptr,
    std::size_t state_count = 0
  >

#endif /* __cplusplus >= 201402L */

  class state_machine {

    base_state* p_current_state = nullptr; ///< Pointer to the current state object.
  
    /* Ensure the given state is valid */

#if __cplusplus >= 201402L

    /**
     * @brief Checks if a type is derived from a base class.
     * 
     * This struct evaluates whether a given type `derived` is derived from the
     * specified base class `base`.
     * 
     * @tparam base The base class to check against.
     * @tparam derived The type to check if it's derived from `base`.
     */
    template <typename base, typename derived>
    struct is_base_of_v {
      /**
       * @brief The boolean result indicating whether `derived` is derived from
       * `base`.
       */
      static constexpr bool value = std::is_base_of<base, derived>::value;
    };
  
    /**
     * @brief Checks if all provided types are derived from the base class.
     * 
     * This structure checks if a variadic list of `derived` types are all
     * derived from the specified base class `base` using conjunction.
     * 
     * @tparam base The base class to check against.
     * @tparam derived The variadic template list of types to check.
     */
    template <typename base, typename... derived>
    struct are_valid_states {
      /**
       * @brief The boolean result indicating whether all `derived` types are
       * derived from `base`.
       */
      static constexpr bool value =
        conjunction_v<is_base_of_v<base, derived>...>;
    };
  
    /**
     * @brief Checks if a given type matches any type in the list of valid
     * states.
     * 
     * This static function evaluates whether the given type `T` matches
     * any of the valid states in a predefined list of types.
     * 
     * @tparam T The type to be checked against valid states.
     * @return constexpr bool `true` if `T` matches any of the valid states,
     * `false` otherwise.
     */
    template <typename T>
    static
    constexpr bool is_valid_state() {
      return disjunction_v<std::is_same<T, states>...>;
    }

#else /* __cplusplus >= 201402L */

    /**
     * @brief Checks if a given type is derived from a base state.
     * 
     * This static function evaluates whether the given type `T` is derived
     * from the predefined `base_state` type.
     * 
     * @tparam T The type to be checked.
     * @return constexpr bool `true` if `T` is derived from `base_state`,
     * `false` otherwise.
     */
    template <typename T>
    static
    constexpr bool is_valid_state() {
      return std::is_base_of<base_state, T>::value;
    }

#endif /* __cplusplus >= 201402L */
  
    /**
     * @brief Provides a pointer to an object of requested state class.
     * 
     * This struct contains a static method named `state` which returns a
     * pointer of `alloc_base_state` class.
     *
     * @tparam alloc_base_state The base state class.
     * @tparam size The size of the state object pointer array.
     */
    template <typename alloc_base_state, std::size_t size = 0>
    struct state_allocator {
      private:

#if __cplusplus >= 201402L

      /**
       * @brief Manages a pool of state objects.
       *
       * The `internal_state_pool` struct is used to manage a collection (pool)
       * of state objects, offering a pointer to this pool and ensuring proper
       * cleanup of dynamically allocated objects.
       */
      struct internal_state_pool {
        private:
        alloc_base_state *pool_ptr[sizeof...(states)];

        public:
        /**
         * @brief Constructs an internal state pool and allocates state objects.
         *
         * Initializes the pool with dynamically allocated objects of
         * each state type provided in the `states` parameter pack.
         */
        internal_state_pool() {
          alloc_base_state *pool_[sizeof...(states)] = { new states... };
          for (std::size_t i = 0; i < sizeof...(states); ++i) {
            pool_ptr[i] = pool_[i];
          }
        }

        /**
         * @brief Destroys the internal state pool and releases memory.
         *
         * Frees each dynamically allocated state object.
         */
        ~internal_state_pool() {
          for (std::size_t i = 0; i < sizeof...(states); ++i) {
            delete pool_ptr[i];
          }
        }

        /**
         * Provides access to the internal pool of state objects.
         *
         * @return A pointer to the array of `alloc_base_state` pointers to
         * derived state objects.
         */
        alloc_base_state** pool() {
          return pool_ptr;
        }
      };

#endif /* __cplusplus >= 201402L */

      public:
      /**
       * @brief Provides a pointer to an object of requested state class from
       * given array of pointers.
       *
       * Takes an array of base state class pointers as argument. The pointers
       * in this array shall point to state objects of the classes enlisted with
       * the state machine. This array is indexed into using `type_id` argument
       * to get the address of an object of the requested state class.
       *
       * @param pool The state object pointers array.
       * @param type_id The type identifier of the derived state class which
       * the address of an object of, is returned in the base state class
       * pointer.
       * @return A pointer to derived state class object of the base state
       * class.
       */
      static
      alloc_base_state* state(alloc_base_state *pool[size],
          const std::size_t type_id) {
        return type_id < size ? pool[type_id] : nullptr;
      }

#if __cplusplus >= 201402L

    private:
      static
      internal_state_pool*& get_pool() {
        static internal_state_pool *pool_ = nullptr;
        return !pool_ ? (pool_ = new internal_state_pool) : pool_;
      }

    public:
      /**
       * @brief Provides a pointer to an object of requested state class from
       * internally managed array of pointers.
       *
       * Allocates objects of enlisted state classes and stores their addresses
       * in an array of base state class pointers. This array is indexed into
       * using `type_id` argument to get the address of an object of the
       * requested state class.
       *
       * @param type_id The type identifier of the derived state class which
       * the address of an object of, is returned in the base state class
       * pointer.
       * @return A pointer to derived state class object of the base state
       * class.
       */
      static
      alloc_base_state* state(const std::size_t type_id) {
        internal_state_pool *pool_ = get_pool();
        return pool_ && type_id < size ? pool_->pool()[type_id] : nullptr;
      }

      template <
        enum alloc_type type_ = type,
        typename std::enable_if<type_ == alloc_type::INTERNAL, int>::type = 0
      >
      static
      void delete_pool() {
        internal_state_pool *&pool_ = get_pool();
        pool_ ? delete pool_ : (void)0;
        pool_ = nullptr;
      }

      template <
        enum alloc_type type_ = type,
        typename std::enable_if<type_ != alloc_type::INTERNAL, int>::type = 0
      >
      static
      void delete_pool() {
      }

#endif /* __cplusplus >= 201402L */

    };
  
    /**
     * @brief Functon template to fetch a pointer to an object of requested
     * state class, from an array of base state class pointers.
     *
     * This function is a wrapper over the actual implementation.
     * @see state_allocator::state(allocate_state**, const std::size_t)
     *
     * @tparam new_state The requested state class.
     * @tparam state_pool_ The state object pointer array.
     * @return A pointer to derived state class object of the base state
     * class.
     */
    template <
      typename new_state,
      enum alloc_type type_ = type,
      typename std::enable_if<type_ == alloc_type::PREALLOCED, int>::type = 0
    >
    base_state* allocate_state() {

#if __cplusplus >= 201402L

      return state_allocator<cfsm::state, sizeof...(states)>::state(state_pool,
          new_state::type_id());

#else

      return state_allocator<cfsm::state, state_count>
        ::state(state_pool, new_state::type_id());

#endif

    }
  
    /* Internal allocator is unavailable below C++14 */

#if __cplusplus >= 201402L

    /**
     * @brief Functon template to fetch a pointer to an object of requested
     * state class, from an internally managed array of base state class
     * pointers.
     *
     * This function is a wrapper over the actual implementation.
     * @see state_allocator::state(const std::size_t)
     *
     * @tparam new_state The requested state class.
     * @tparam state_pool_ The state object pointer array.
     * @return A pointer to derived state class object of the base state
     * class.
     */
    template <
      typename new_state,
      enum alloc_type type_ = type,
      typename std::enable_if<type_ == alloc_type::INTERNAL, int>::type = 0
    >
    base_state* allocate_state() {
      return state_allocator<cfsm::state, sizeof...(states)>
        ::state(new_state::type_id());
    }

#endif /* __cplusplus >= 201402L */

    /**
     * @brief Functon template to allocate a new state object and return its
     * address as a pointer of base state class.
     *
     * Uses new operator on the requested state class.
     *
     * @tparam new_state The requested state class.
     * @tparam state_pool_ The state object pointer array.
     * @return A pointer to derived state class object of the base state
     * class.
     */
    template <
      typename new_state,
      enum alloc_type type_ = type,
      typename std::enable_if<type_ == alloc_type::LAZY, int>::type = 0
    >
    base_state* allocate_state() {
      return new new_state();
    }

    /**
     * @brief Free the current state object.
     *
     * Sets the current state pointer as nullptr;
     */
    void delete_current_state() {
      if (type == alloc_type::LAZY) {
        delete p_current_state;
      }
      p_current_state = nullptr;
    }

  public:
    /**
     * @brief Returns a non-negative integer of `std::size_t` type.
     *
     * Returns incremented value of a static variable every time it gets called
     * starting with zero. This function is static member of the "state_machine"
     * class, hence can be used to assign type identifiers to derived state
     * classes.
     * 
     * @return A non-negative integer.
     */
    static
    std::size_t gen_type_id() {
      static std::size_t type_id_gen = 0;
      return type_id_gen++;
    }
  
#if __cplusplus >= 201402L

    /**
     * @brief Constructor for the state machine.
     *
     * Checks if the states provided as template parameters are derived from
     * the base "state" class.
     */
    state_machine() {
      static_assert(are_valid_states<base_state, states...>::value,
          "Invalid states in ctor");
    }

#else /* __cplusplus >= 201402L */

    /**
     * @brief Constructor for the state machine.
     */
    state_machine() = default;

#endif /* __cplusplus >= 201402L */
  
    /**
     * @brief Starts the state machine in an initial state.
     * 
     * The state machine transitions into the initial state and calls the
     * state's `on_enter` member function.
     * 
     * @tparam initial_state The type of the initial state, which must inherit
     * from `base_state`.
     * @param dataptr Opaque pointer to user data.
     */
    template <typename initial_state>
    void start(void *dataptr) {
      static_assert(is_valid_state<initial_state>(), "Invalid initial state");
  
      p_current_state = state_machine::allocate_state<initial_state>();
      if (!p_current_state) {
        throw std::runtime_error("State pointer is null");
      }
  
      p_current_state->on_enter(dataptr);
    }
  
    /**
     * @brief Transitions the state machine to a new state.
     * 
     * The state machine transitions from the current state to the new state
     * and calls the appropriate `on_exit` member function for the current
     * state and `on_enter` member function for the new state. It also invokes
     * the transition functor for the specific transition between the two
     * states.
     * 
     * @tparam new_state The type of the target state, which must inherit from
     * `base_state`.
     * @param dataptr Opaque pointer to user data.
     * @return true on successfull state transition, false on error.
     */
    template <
      typename from_state, typename to_state,
      typename std::enable_if<transition<from_state, to_state>::exists,
        bool>::type = false
    >
    bool transition(void *dataptr) {
      static_assert(is_valid_state<from_state>(), "Invalid source state");
      static_assert(is_valid_state<to_state>(), "Invalid target state");
      if (dynamic_cast<from_state*>(p_current_state) == nullptr) {
        return false;
      }
  
      if (!p_current_state) {
        throw std::runtime_error("State pointer is null");
      }
  
      base_state* p_new_state = state_machine::allocate_state<to_state>();
      if (!p_new_state) {
        std::ostringstream oss;
        oss << "Failed to allocate new state, state_pool: " << state_pool;
        throw std::runtime_error(oss.str());
      }
  
      p_current_state->on_exit(dataptr);
      if (type == alloc_type::LAZY) {
        delete p_current_state;
      }
  
      /* Call transition functor for "from_state" to "to_state" transition */
      cfsm::transition<from_state, to_state>()(dataptr);
  
      p_current_state = p_new_state;
      p_current_state->on_enter(dataptr);
  
      return true;
    }
  
    /**
     * @brief Stops the state machine.
     *
     * This function calls the `on_exit` member function and frees the current
     * state object. The `start` member function is required to be called for
     * the state machine to start operating.
     *
     * @param dataptr Opaque pointer to user data.
     */
    void stop(void *dataptr) {
      if (!p_current_state) {
        return;
      }

      p_current_state->on_exit(dataptr);

      delete_current_state();

#if __cplusplus >= 201402L

      state_allocator<cfsm::state, sizeof...(states)>::delete_pool();

#endif

    }

    /**
     * @brief Returns pointer to constant current state object.
     *
     * This function template returns a pointer of the state type provided as
     * template argument which points to constant object of the current state 
     * class. If the current state object does not belong the the state type
     * requested, nullptr is returned.
     *
     * @tparam state_type The expected state class of the current state.
     * @return Pointer of the state class provided as template parameter.
     */
    template<typename state_type = base_state>
    const state_type* state() const {
      return dynamic_cast<state_type*>(p_current_state);
    }

    /**
     * @brief Save the state machine's state to memory.
     *
     * Writes the address of the current state object to the provided char
     * array. The array whose size is also provided should be large enough
     * to store an address. If the array is smaller the function does nothing.
     *
     * @param pdata Pointer to char array.
     * @param datalen Size of the char array.
     */
    std::size_t save(char *pdata, std::size_t datalen) {
      if (!pdata) {
        return 0;
      }
      if (datalen < sizeof(p_current_state)) {
        return 0;
      }

      *reinterpret_cast<base_state**>(pdata) = p_current_state;

      /* To avoid incorrect free in dtor set p_current_state as nullptr */
      p_current_state = nullptr;

      return sizeof(p_current_state);
    }

    /**
     * @brief Load the state machine's state from memory.
     *
     * Reads the address of the current state object from the provided char
     * array. If the size of the array is smaller than size of an address the
     * function does nothing.
     *
     * @param pdata Pointer to char array.
     * @param datalen Size of the char array.
     */
    std::size_t load(const char *pdata, std::size_t datalen) {
      if (!pdata) {
        return 0;
      }
      if (datalen < sizeof(p_current_state)) {
        return 0;
      }

      p_current_state = *reinterpret_cast<base_state *const *>(pdata);

      return sizeof(p_current_state);
    }
  
    /**
     * @brief Destructor for the state machine.
     * 
     * Frees the current state object when the state machine is destroyed.
     * @see delete_current_state()
     */
    ~state_machine() {
      if (!p_current_state) {
        return;
      }
  
      delete_current_state();

#if __cplusplus >= 201402L

      state_allocator<cfsm::state, sizeof...(states)>::delete_pool();

#endif

    }

  };

  /* State machine type aliases */

#if __cplusplus >= 201402L

  /**
   * @brief Type alias representing a finite state machine with lazy state
   * object allocation scheme.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam states List of state classes present in the state machine.
   */
  template <
    typename base_state,
    base_state* state_pool[] = nullptr,
    typename... states
  >
  using state_machine_lazy = state_machine<
    base_state,
    alloc_type::LAZY,
    state_pool,
    states...
  >;

  /**
   * @brief Type alias representing a finite state machine with externally
   * managed preallocated state objects scheme.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam states List of state classes present in the state machine.
   */
  template <
    typename base_state,
    base_state* state_pool[] = nullptr,
    typename... states
  >
  using state_machine_ext = state_machine<
    base_state,
    alloc_type::PREALLOCED,
    state_pool,
    states...
  >;

  /**
   * @brief Type alias representing a finite state machine with internally
   * managed preallocated state objects scheme.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam states List of state classes present in the state machine.
   */
  template <
    typename base_state,
    base_state* state_pool[] = nullptr,
    typename... states
  >
  using state_machine_int = state_machine<
    base_state,
    alloc_type::INTERNAL,
    state_pool,
    states...
  >;

#else /* __cplusplus >= 201402L */

  /**
   * @brief Type alias representing a finite state machine with lazy state
   * object allocation scheme.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam state_count Number of states present in the state machine.
   */
  template <
    typename base_state,
    base_state* state_pool[] = nullptr,
    std::size_t state_count = 0
  >
  using state_machine_lazy = state_machine<
    base_state,
    alloc_type::LAZY,
    state_pool,
    state_count
  >;

  /**
   * @brief Type alias representing a finite state machine with internally
   * managed preallocated state objects scheme.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam state_count Number of states present in the state machine.
   */
  template <
    typename base_state,
    base_state* state_pool[] = nullptr,
    std::size_t state_count = 0
  >
  using state_machine_ext = state_machine<
    base_state,
    alloc_type::PREALLOCED,
    state_pool,
    state_count
  >;

  /**
   * @brief Type alias representing a finite state machine with externally
   * managed preallocated state objects scheme.
   * 
   * @tparam base_state The base class from which all state types must derive.
   * @tparam state_pool Array of pointers to base state class objects.
   * @tparam state_count Number of states present in the state machine.
   */
  template <
    typename base_state,
    base_state* state_pool[] = nullptr,
    std::size_t state_count = 0
  >
  using state_machine_int = state_machine<
    base_state,
    alloc_type::INTERNAL,
    state_pool,
    state_count
  >;

#endif /* __cplusplus >= 201402L */

}

#endif /* __SMBUILDER_HPP__ */
