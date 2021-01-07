#include <conio.h>
#include <iostream>
#include "bond.hpp"

#define AUTO_ARG(x) decltype(x), x

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    struct TestEvent {
      std::string origin;
      int value{ -1 };

      std::string to_string() const {
        return "#" + std::to_string(value) + " from " + origin;
      }
    };

    register_meta_event<TestEvent>();

    struct nativeListener_t {
      void receive(const TestEvent &evt) {
        std::cout << "[c++] received TestEvent: " << evt.to_string()
                  << std::endl;
      }
    } listener;

    entt::dispatcher dispatcher{};
    dispatcher.sink<TestEvent>().connect<&nativeListener_t::receive>(listener);

    sol::state lua{};
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);
    lua.require("dispatcher", sol::c_call<AUTO_ARG(&open_dispatcher)>, false);

    // clang-format off
    lua.new_usertype<TestEvent>("TestEvent",
      "type_id", &entt::type_info<TestEvent>::id,
      sol::call_constructor,
      sol::factories([](const char *origin, int value) {
        return TestEvent{ origin, value };
      }),
      "origin", &TestEvent::origin,
      "value", &TestEvent::value,

      sol::meta_function::to_string, &TestEvent::to_string
    );
    // clang-format on

    lua["dispatcher"] = std::ref(dispatcher);

    lua.script(R"(
      --dispatcher = entt.dispatcher.new()

      function test_event_handler(e)
        print('[lua] received TestEvent: ' .. tostring(e))
      end

      conn = dispatcher:connect(TestEvent, test_event_handler)
    )");

    lua.script("dispatcher:trigger(TestEvent('lua', 7))");

    lua.script(R"(
      listener = { count = 2 }
      function listener.receive(evt)
        print('[lua] listener:receive(): TestEvent: ' .. tostring(evt))
      end
      function listener.method(evt)
        print('[lua] listener:method(): TestEvent: ' .. tostring(evt))
      end

      listener.connection =
        dispatcher:connect(TestEvent, listener.receive)
      dispatcher:connect(TestEvent, listener.method)
    )");
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