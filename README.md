![Examples](https://github.com/notweerdmonk/cfsm/actions/workflows/c-cpp.yml/badge.svg)

# CFSM

###### Compile-time finite state machine library

C++11, C++14, C++17, C++20

Browse [smbuilder.hpp](https://github.com/notweerdmonk/cfsm/blob/master/smbuilder.hpp)

#### Static disposition
States and transitions between them must be known at compile-time. The
library uses templates to implement state transitions.

#### States
All user defined states which are represented by classes should inherit from the
base `state` class provided in `smbuilder.hpp`. The base class provides
`on_entry` and `on_exit` member functions which are called on entry to state and
exit from the state respectively. These pure virtual member functions shall be
overriden in the user defined classes.

The `state_machine` class provides a `state` member function template which
will return a pointer to the current state object which is `dynamic_cast` into
the state class provided as the template parameter. When current state is not of
the requested type, `nullptr` is returned. This can be used as a test for the
current state of a state machine.

#### State transitions
Transitions between user defined states are represented by a struct template
named `transition`. The template parameters are the types of source state class
and target state class respectively.

User defined template specializations should provide two members. First, a
static constexpr bool member variable named `exists` which should be set as
`true`; an helper macro `TRANSITION_EXISTS` is provided for this purpose.
Second, an implementation of the `operator()` function, the call operator
overload, which returns void and takes a void pointer as its argument. This
function shall be called on a valid transition (source and target states are
valid classes and the template specialization of `transition` struct for
source and target state exists), triggered by calling the `transition` member
function of the `state_machine` class.
The function signature shall be:

```C
void operator()(void *dataptr);
```


#### State machine types
State machines are classified based on their state object allocation scheme.
These are lazily allocated, externally preallocated and internally preallocated.
The enum class "alloc_type" passed as a template parameter, specifies the
allocation scheme for a state machine.

#### Lazily allocated states
If the "state_pool" template parameter is set to nullptr (default) the
memory for state objects is dynamically allocated with new operator when
state transition occurs (by calling `transition` member function).

#### Externally managed preallocated states storage
An external array of state objects is required by the state machine. The 
"state_pool" template parameter shall be an array of pointers of the base
"state" class, having size equal to the number of states. The array is indexed
into by the type identifiers of respective state classes enabling constant time
access. Base class pointers should point to objects of derived state classes.
Memory for these derived state class objects shall be managed by user.

#### Internally managed preallocated state storage
The array of preallocated state objects can be managed by the state machine
internally. Type identifiers of the derived state classes will be used to index
into this array. The state objects' lifetimes span the lifetime of the state
machine object. This scheme offers convenience at the cost of reduced
flexibility.

#### Type identifier
The type identifier of a derived state class is an unsigned integer of type
`std::size_t`. Provide a static member function named `type_id` to each derived
state class which returns the type identifier.
The function signature shall be:

```C
static std::size_t type_id();
```

See example programs for type identifier schemes. A static helper function named
`gen_type_id` which returns a `std::size_t` value based on an incremented static
variable is provided for the `state_machine` class.

Browse [examples](https://github.com/notweerdmonk/cfsm/tree/master/examples)

#### Simple usage
User defined states derive from the abstract `state` base class and override the
`on_enter` and `on_exit` virtual member functions.

```C
class state_a final : public state {
public:
  void on_enter(void *dataptr) const override {
    std::cout << "Entering state A\n";
  }

  void on_exit(void *dataptr) const override {
    std::cout << "Exiting state A\n";
  }
};
```

Each transition between a pair of states is defined as a specialization of the
`cfsm::transition` functor template. The `operator()` member function shall
contain user code. User data is provided as a void pointer argument. This
pointer shall be passed to `state_machine::transition` function.

```C
/* Specialize transition for state_a -> state_b */
template<>
struct cfsm::transition<state_a, state_b> {
  static constexpr bool exists = true;
  void operator()(void *dataptr) {
    std::cout << "Transitioning from state A to state B\n";
  }
};
```


State machine objects are created with template specialized constructor of
`state_machine` class.

```C
state_machine<
  state
  alloc_type::LAZY,
  nullptr,
  state_a,
  state_b
> fsm;
```


A state machine is started by initializing it to desired state. The `on_entry`
member function for the initial state shall be called.

```C
fsm.start<state_a>(nullptr);
```

State transitions are triggered by calling template specialization of
`state_machine::transition` member function. Its template arguments are the
source state class and the target state class. This function shall return `true`
on successful state transition.

```C
fsm.transition<state_a, state_b>(nullptr);
```


Current state of a state machine can be checked by calling the
`state_machine::state` member function with the expected state class as its
template parameter. The function shall return a `state` (base class) pointer to
the current state object if the expected state matches the current state. Else,
`nullptr` shall be returned.

```C
assert(fsm.state<state_a>() != nullptr);
```


There is a provision to call the `on_exit` member function of the current state
and move the state machine to inoperable state, somewhat like stopping the
state machine. This can be done with the `state_machine::stop` member function.
The state machine should be started again to operate.

```C
fsm.stop(nullptr);
```


#### Save/Load
State machines can be halted and later resumed. The `state_machine::save`
member function serializes the state and stores it in a char array. The array
should be large enough to store an address. State machines become inoperable
after being saved.

```C
char ser_data[8];
assert(fsm.save(ser_data, sizeof(ser_data)) == sizeof(ser_data));
```

A state machine can be resumed by loading its state from memory. This is done
with the `state_machine::load` member function that loads data from a char
array and deserializes it to restore the state. The serialized state data can
be loaded into the same state machine or a different state machine of same
kind, creating a clone while the original state machine remains inoperable.

```C
state_machine<
  state
  alloc_type::LAZY,
  nullptr,
  state_a,
  state_b
> fsm_clone;
assert(fsm_clone.load(ser_data, sizeof(ser_data)) == sizeof(ser_data));
```

#### Preallocated storage usage
In order to use preallocated (internally or externally managed) state objects
storage, the requirement for the derived state classes is that they should each
have a static member function named `type_id` which returns an unsigned integer
of type `std::size_t`.

```C
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
```

The example above utilizes the `state_machine::gen_type_id` static member
function to generate an state object unique type identifier which gets stored
in a static member variable of the derived state class. Its value gets returned
by the `type_id` static member function. As `gen_type_id` returns values
incremented by one, starting from zero, the type identifiers can be used to
directly index into the preallocated array of state objects.

In case of externally managed state objects the `type` template argument should
be set as `alloc_type::PREALLOCED`. An array of base `state` class pointers
shall be passed as template argument to the `state_machine` class constructor.
These pointers should point to derived state class objects.

```C
/* state_storage is a struct containing the derived state class objects */
state* state_pool[2] = {
  &state_storage.s1,
  &state_storage.s2
};

state_machine<
  state,
  alloc_type::PREALLOCED,
  state_pool,
  state_1,
  state_2
> fsm;
```


The user managed state objects can be statically created as members of a
`struct` or as individual objects. Otherwise, they can be dynamically created
using the `new` operator.

```C
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

/*****************************************************************************/

/* Dynamic state storage */

state* state_pool[2] = { 0 };

/* Somehwere in user code */
//...
{
  /* Allocate state objects on heap */
  state_pool[0] = new state_1;
  state_pool[1] = new state_2;
}
//...
{
  delete state_pool[0];
  delete state_pool[1];
}
```


State objects can be preallocated but managed internally by the state
machine. The template parameter `type` can be set to `alloc_type::INTERNAL`.
These state objects are created and destroyed along with the state machine
object.

```C
state_machine<
  state,
  alloc_type::INTERNAL,
  nullptr,
  state_1,
  state_2
> fsm;
```


Browse [simple.cc](https://github.com/notweerdmonk/cfsm/blob/master/examples/simple.cc)
