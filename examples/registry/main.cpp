#include <thread>
#include <chrono>
#include <conio.h>
#include "bond.hpp"
#include "../common/transform.hpp"

#define AUTO_ARG(x) decltype(x), x

using namespace std::chrono_literals;

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    sol::state lua{};
    lua.open_libraries(sol::lib::base, sol::lib::package);
    lua.require("registry", sol::c_call<AUTO_ARG(&open_registry)>, false);
    register_transform(lua);
    register_meta_component<Transform>();

    entt::registry registry{};
    lua["registry"] = std::ref(registry);

    lua.script(R"(
      --registry = entt.registry.new()

      bowser = registry:create()
      assert(bowser == 0 and registry:size() == 1)
      registry:emplace(bowser, Transform(5, 6))
      assert(registry:has(bowser, Transform))
      assert(registry:has(bowser, Transform.type_id()))
      
      assert(not registry:any(bowser, -1, -2))

      transform = registry:get(bowser, Transform)
      transform.x = transform.x + 10
      print('Bowser position = ' .. tostring(transform))
    )");

    entt::entity bowser{lua["bowser"]};
    auto t = registry.try_get<Transform>(bowser);
    assert(t);
    Transform &transform = lua["transform"];
    assert(t->x == transform.x && t->y == transform.y);

    lua.script(R"(
      view = registry:runtime_view(Transform)
      assert(view:size_hint() > 0)
      
      local koopa = registry:create()
      registry:emplace(koopa, Transform(100, -200))
      transform = registry:get(koopa, Transform)
      print('Koopa position = ' .. tostring(transform))
      
      assert(view:size_hint() == 2)

      view:each(function(entity)
        registry:remove(entity, Transform)
      end)

      assert(view:size_hint() == 0)
    )");
  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}