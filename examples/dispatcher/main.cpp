#include <iostream>
#include <conio.h>
#include "entt/entt.hpp"
#include "../meta_helper.hpp"
#include "sol/sol.hpp"

#define AUTO_ARG(x) decltype(x), x

template <typename Event>
auto register_listener(entt::dispatcher &dispatcher, const sol::function &f) {
  assert(f.valid());
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
void trigger_event(entt::dispatcher &dispatcher, const sol::table &evt) {
  assert(evt.valid());
  dispatcher.trigger(evt.as<Event>());
}
template <typename Event>
void enqueue_event(entt::dispatcher &dispatcher, const sol::table &evt) {
  assert(evt.valid());
  dispatcher.enqueue(evt.as<Event>());
}
template <typename Event>
void clear_event(entt::dispatcher &dispatcher) {
  dispatcher.clear<Event>();
}
template <typename Event>
void update_event(entt::dispatcher &dispatcher) {
  dispatcher.update<Event>();
}

template <typename Event> void register_meta_event() {
  entt::meta<Event>().type();
  entt::meta<Event>()
    .template func<&register_listener<Event>>("register_listener"_hs)
    .template func<&trigger_event<Event>>("trigger_event"_hs)
    .template func<&enqueue_event<Event>>("enqueue_event"_hs)
    .template func<&clear_event<Event>>("clear_event"_hs)
    .template func<&update_event<Event>>("update_event"_hs);
}

sol::table open_dispatcher(const sol::this_state &s) {
  sol::state_view lua{ s };
  auto entt_module = lua["entt"].get_or_create<sol::table>();

#define AS_DISPATCHER_REF(self) std::ref(self.as<entt::dispatcher>())

  // clang-format off
  entt_module.new_usertype<entt::dispatcher>("dispatcher",
    sol::meta_function::construct,
    sol::factories([] { return entt::dispatcher{}; }),

    "trigger", [](const sol::object &self, const sol::table &evt) {
      auto type_id = evt["type_id"]().get<entt::id_type>();
      invoke_meta_func(type_id, "trigger_event"_hs, AS_DISPATCHER_REF(self), evt);
    },
    "enqueue", [](const sol::object &self, const sol::table &evt) {
      auto type_id = evt["type_id"]().get<entt::id_type>();
      invoke_meta_func(type_id, "enqueue_event"_hs, AS_DISPATCHER_REF(self), evt);
    },
    "clear", sol::overload(
      &entt::dispatcher::clear<>,
      [](const sol::object &self, entt::id_type type_id) {
        invoke_meta_func(type_id, "clear_event"_hs, AS_DISPATCHER_REF(self));
      }
    ),
    "update", sol::overload(
      &entt::dispatcher::update,
      [](const sol::object &self, entt::id_type type_id) {
        invoke_meta_func(type_id, "update_event"_hs, AS_DISPATCHER_REF(self));
      }
    ),
    "connect",
        [](const sol::object &self, entt::id_type type_id, const sol::function &listener) {
          if (listener.valid()) {
            return invoke_meta_func(type_id, "register_listener"_hs,
              AS_DISPATCHER_REF(self), listener);
          }
          return entt::meta_any{};
        }
    );
  // clang-format off

#undef AS_DISPATCHER_REF

  return entt_module;
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
    entt::dispatcher dispatcher{};
    dispatcher.sink<TestEvent>().connect<&native_test_event_listener>();

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

    lua["dispatcher"] = std::ref(dispatcher);

    lua.script(R"(
      --dispatcher = entt.dispatcher.new()

      function test_event_handler(e)
        print('[lua] received TestEvent{'
          .. e.value .. '} from: ' .. e.origin)
      end

      conn = dispatcher:connect(TestEvent.type_id(), test_event_handler)
    )");

    lua.script("dispatcher:trigger(TestEvent('lua', 7))");

    lua.script(R"(
      listener = { count = 2 }
      function listener.receive(evt)
        print('[lua] listener:receive(): TestEvent{'
          .. evt.value .. '} from: ' .. evt.origin)
      end
      function listener.method(evt)
        print('[lua] listener:method(): TestEvent{'
          .. evt.value .. '} from: ' .. evt.origin)
      end

      listener.connection =
        dispatcher:connect(TestEvent.type_id(), listener.receive)
      dispatcher:connect(TestEvent.type_id(), listener.method)
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