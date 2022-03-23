#include <conio.h>
#include "bond.hpp"

#define AUTO_ARG(x) decltype(x), x

namespace {

struct TestEvent {
  std::string origin;
  int value{-1};

  [[nodiscard]] std::string to_string() const {
    return "#" + std::to_string(value) + " from " + origin;
  }
};

struct NativeListener {
  void receive(const TestEvent &evt) const {
    std::cout << "[c++] received TestEvent: " << evt.to_string() << std::endl;
  }
};

void expose_test_event(sol::state &lua) {
  // clang-format off
  lua.new_usertype<TestEvent>("TestEvent",
    // Make sure that every event you define has 'type_id' getter
    "type_id", &entt::type_hash<TestEvent>::value,

    sol::call_constructor,
    sol::factories([](const char *origin, int value) {
      return TestEvent{origin, value};
    }),
    "origin", &TestEvent::origin,
    "value", &TestEvent::value,

    sol::meta_function::to_string, &TestEvent::to_string
  );
  // clang-format on
}

} // namespace

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    entt::dispatcher dispatcher{};
    NativeListener listener{};
    dispatcher.sink<TestEvent>().connect<&NativeListener::receive>(listener);

    register_meta_event<TestEvent>();

    sol::state lua{};
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);
    lua.require("dispatcher", sol::c_call<AUTO_ARG(&open_dispatcher)>, false);
    expose_test_event(lua); // Make TestEvent available to Lua

    lua["dispatcher"] =
      std::ref(dispatcher); // Make the dispatcher available to Lua

    lua.do_file("lua/test_event_handler.lua");

    lua.script(
      "dispatcher:trigger(TestEvent('lua', 7))"); // Emit an event from script

    lua.do_file("lua/more_listeners.lua");
    lua.collect_garbage(); // listener.method will be detached now

    lua.script("dispatcher:trigger(TestEvent('lua', 117))");

    dispatcher.trigger<TestEvent>("c++", 2);

    while (true) {
      lua.step_gc(4);
      dispatcher.update();

      if (_kbhit()) break;
    }
  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}
