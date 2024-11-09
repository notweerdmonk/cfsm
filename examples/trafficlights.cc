#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <smbuilder.hpp>

/* ANSI escape codes */
#define ANSI_FG_RED     "\x1b[31m"
#define ANSI_FG_GREEN   "\x1b[32m"
#define ANSI_FG_YELLOW  "\x1b[33m"
#define ANSI_RESET      "\x1b[0m"

using namespace cfsm;

enum class fsm_enum {
  NS,
  EW
};

struct fsm_data {
  enum fsm_enum type;
  std::string name;
  std::string trafficname;
  std::condition_variable &cv;
  std::unique_lock<std::mutex> *plock;
  bool &ns_turn;
};

/**
 * @brief Represents the state where the green light is on.
 */
class green_light : public state {
public:
  void on_enter(void *dataptr) const override {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": " << ANSI_FG_GREEN
      << "Green" << ANSI_RESET << " light ON. Cars can go.\n";
  }

  void on_exit(void *dataptr) const override {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": Green light OFF.\n";
  }
};

/**
 * @brief Represents the state where the yellow light is on.
 */
class yellow_light : public state {
public:
  void on_enter(void *dataptr) const override {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": " << ANSI_FG_YELLOW
      << "Yellow" << ANSI_RESET << " light ON. Cars should slow down.\n";
  }

  void on_exit(void *dataptr) const override {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": Yellow light OFF.\n";
  }
};

/**
 * @brief Represents the state where the red light is on.
 */
class red_light : public state {
public:
  void on_enter(void *dataptr) const override {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": " << ANSI_FG_RED
      << "Red" << ANSI_RESET << " light ON. Cars must stop.\n";
  }

  void on_exit(void *dataptr) const override {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": Red light OFF.\n";
  }
};

/**
 * @brief Transition from Green to Yellow light.
 */
TRANSITION_BEGIN(green_light, yellow_light)
  void operator()(void *dataptr) const {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": Transitioning from "
      << ANSI_FG_GREEN << "Green" << ANSI_RESET << " to "
      << ANSI_FG_YELLOW << "Yellow" << ANSI_RESET << " light.\n";
    std::cout << pdata->trafficname << " is slowing down.\n";
  }
TRANSITION_END

/**
 * @brief Transition from Yellow to Red light.
 */
TRANSITION_BEGIN(yellow_light, red_light)
  void operator()(void *dataptr) const {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);

    std::cout << pdata->name << ": Transitioning from "
      << ANSI_FG_YELLOW << "Yellow" << ANSI_RESET << " to "
      << ANSI_FG_RED << "Red" << ANSI_RESET << " light.\n";
    std::cout << pdata->trafficname << " has stopped.\n";

    pdata->plock->lock();
    pdata->ns_turn = (pdata->type == fsm_enum::NS) ? false : true;
    /* Notify East-West / North-South FSM to start */
    pdata->cv.notify_all();
  }
TRANSITION_END

/**
 * @brief Transition from Red to Green light.
 */
TRANSITION_BEGIN(red_light, green_light)
  void operator()(void *dataptr) const {
    struct fsm_data *pdata = reinterpret_cast<struct fsm_data*>(dataptr);
    std::cout << pdata->name << ": Transitioning from "
      << ANSI_FG_RED << "Red" << ANSI_RESET << " to "
      << ANSI_FG_GREEN << "Green" << ANSI_RESET << " light.\n";
    std::cout << pdata->trafficname << " is passing.\n";
  }
TRANSITION_END

/* Traffic signal controller */
class traffic_ctl {
private:
#if __cplusplus >= 201402L
  using fsm_type = 
    state_machine<
      state,
      alloc_type::LAZY,
      nullptr,
      green_light,
      yellow_light,
      red_light
    >;
#else
  using fsm_type = 
    state_machine<
      state,
      alloc_type::LAZY,
      nullptr,
      3
    >;
#endif

  fsm_type ns_fsm_;
  fsm_type ew_fsm_;

  fsm_data ns_data, ew_data;

  std::mutex mtx_;
  std::condition_variable cv_;
  bool ns_turn_;

public:
  traffic_ctl() : ns_turn_(true),
    ns_data({
        fsm_enum::NS,
        "N-S state machine",
        "North-South traffic",
        cv_,
        nullptr,
        ns_turn_
      }),
    ew_data({
        fsm_enum::EW,
        "E-W state machine",
        "East-West traffic",
        cv_,
        nullptr,
        ns_turn_
      }) {
  }

  void start(int num_cycles, bool ns_turn = true) {
    ns_fsm_.start<red_light>(&ns_data);
    ew_fsm_.start<red_light>(&ew_data);

    std::thread ns_thread(&traffic_ctl::run_ns, this, num_cycles);
    std::thread ew_thread(&traffic_ctl::run_ew, this, num_cycles);

    ns_turn_ = ns_turn;
    cv_.notify_all();

    ns_thread.join();
    ew_thread.join();

    ns_fsm_.stop(&ns_data);
    ew_fsm_.stop(&ew_data);
  }

private:
  void run_ns(int num_cycles) {
    while (num_cycles--) {
      std::unique_lock<std::mutex> lock(mtx_);
      ns_data.plock = &lock;

      cv_.wait(lock, [&] { return ns_turn_; });
      lock.unlock();

      if (!ns_fsm_.transition<red_light, green_light>(&ns_data)) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));

      if (!ns_fsm_.transition<green_light, yellow_light>(&ns_data)) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));

      if (!ns_fsm_.transition<yellow_light, red_light>(&ns_data)) {
        break;
      }

    }
  }

  void run_ew(int num_cycles) {
    while (num_cycles--) {
      std::unique_lock<std::mutex> lock(mtx_);
      ew_data.plock = &lock;

      cv_.wait(lock, [&] { return !ns_turn_; });
      lock.unlock();

      if (!ew_fsm_.transition<red_light, green_light>(&ew_data)) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1500));

      if (!ew_fsm_.transition<green_light, yellow_light>(&ew_data)) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::seconds(1));

      if (!ew_fsm_.transition<yellow_light, red_light>(&ew_data)) {
        break;
      }
    }
  }
};

int main() {
  traffic_ctl controller;
  controller.start(2);
  return 0;
}
