#include <iostream>
#include <conio.h>
#include "entt/entt.hpp"
#include "sol/sol.hpp"

#define AUTO_ARG(x) decltype(x), x

entt::dispatcher gDispatcher{};

template <typename Event>
auto register_listener(entt::dispatcher &dispatcher, const sol::function &f) {
  std::cout << "Registered lua listener: " << f.pointer() << std::endl;

  class ScriptListener {
  public:
    ScriptListener(const sol::function &f) : callback{ f } {}
    ~ScriptListener() {
      connection.release();
      std::cout << "Unregistered lua listener: " << callback.pointer()
                << std::endl;
      callback.abandon();
    }
    void receive(const Event &evt) {
      if (connection && callback.valid()) callback(evt);
    }

    sol::function callback;
    entt::connection connection;
  };

  auto listener = std::make_shared<ScriptListener>(f);
  listener->connection =
    dispatcher.sink<Event>().connect<&ScriptListener::receive>(*listener);
  return listener;
}
template <typename Event>
void trigger_event(entt::dispatcher &dispatcher, const sol::object &evt) {
  dispatcher.trigger<Event>(evt.as<Event>());
}
template <typename Event>
void enqueue_event(entt::dispatcher &dispatcher, const sol::object &evt) {
  dispatcher.enqueue<Event>(evt.as<Event>());
}

template <typename Event> void register_meta_event() {
  entt::meta<Event>().type();
  entt::meta<Event>()
    .template func<&register_listener<Event>>("register_listener"_hs)
    .template func<&trigger_event<Event>>("trigger_event"_hs)
    .template func<&enqueue_event<Event>>("enqueue_event"_hs);
}

sol::table open_dispatcher(const sol::this_state &s) {
  sol::state_view lua{ s };
  auto dispatcher_module = lua.create_table();

  dispatcher_module.set_function(
    "connect", [&](entt::id_type id, const sol::function &listener) {
      if (listener.valid()) {
        if (auto event_type = entt::resolve_type(id); event_type) {
          return event_type.func("register_listener"_hs)
            .invoke({}, std::ref(gDispatcher), listener);
        }
      }
      return entt::meta_any{};
    });

  dispatcher_module.set_function(
    "trigger", [&](entt::id_type id, const sol::object &evt) {
      if (auto event_type = entt::resolve_type(id); event_type) {
        event_type.func("trigger_event"_hs)
          .invoke({}, std::ref(gDispatcher), evt);
      }
    });
  dispatcher_module.set_function(
    "enqueue", [&](entt::id_type id, const sol::object &evt) {
      if (auto event_type = entt::resolve_type(id); event_type) {
        event_type.func("enqueue_event"_hs)
          .invoke({}, std::ref(gDispatcher), evt);
      }
    });

  return dispatcher_module;
}

//
//
//

struct TestEvent {
  std::string origin;
  int value;
};

void native_test_event_listener(const TestEvent &evt) {
  std::cout << "[c++] received : TestEvent{" << evt.value
            << "} from: " << evt.origin << std::endl;
}

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    gDispatcher.sink<TestEvent>().connect<&native_test_event_listener>();

    register_meta_event<TestEvent>();

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
      "value", &TestEvent::value
    );
    // clang-format on

    lua.script(R"(
      dispatcher = require('dispatcher')

      function test_event_handler(e)
        print('[lua] received TestEvent{'
          .. e.value .. '} from: ' .. e.origin)
      end

      conn = dispatcher.connect(TestEvent.type_id(), test_event_handler)
    )");

    lua.script("dispatcher.trigger(TestEvent.type_id(), TestEvent('lua', 7))");

    lua.script(R"(
      listener = {}
      function listener.receive(evt)
        print('[lua] listener:receive(): TestEvent{'
          .. evt.value .. '} from: ' .. evt.origin)
      end
      function listener.method(evt)
        print('[lua] listener:method(): TestEvent{'
          .. evt.value .. '} from: ' .. evt.origin)
      end
      
      listener.connection =
        dispatcher.connect(TestEvent.type_id(), listener.receive)

      dispatcher.connect(TestEvent.type_id(), listener.method)
    )");
    lua.collect_garbage(); // listener.method will be detached now
    lua.script(
      "dispatcher.trigger(TestEvent.type_id(), TestEvent('lua', 117))");

    gDispatcher.trigger<TestEvent>("c++", 2);

    while (true) {
      lua.step_gc(4);
      gDispatcher.update();

      if (_kbhit()) break;
    }
  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}