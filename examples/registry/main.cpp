#include <conio.h>
#include "entt/entt.hpp"
#include "sol/sol.hpp"

#define AUTO_ARG(x) decltype(x), x

entt::registry gRegistry{};

template <typename Component>
void set_component(entt::registry &registry, entt::entity entity,
                   const sol::object &component) {
  registry.emplace_or_replace<Component>(entity, component.as<Component>());
}
template <typename Component>
sol::object get_component(entt::registry &registry, entt::entity entity,
                          const sol::this_state &s) {
  auto &comp = registry.get_or_emplace<Component>(entity);
  return sol::make_object(s, comp); // or use std::ref(comp) to give more permission
}
template <typename Component>
bool has_component(entt::registry &registry, entt::entity entity) {
  return registry.has<Component>(entity);
}

template <typename Component> void register_meta_component() {
  entt::meta<Component>().type();
  entt::meta<Component>()
    .template func<&set_component<Component>>("set"_hs)
    .template func<&get_component<Component>>("get"_hs)
    .template func<&has_component<Component>>("has"_hs);
}

sol::table open_registry(const sol::this_state &s) {
  sol::state_view lua{ s };
  auto registry_module = lua.create_table();

  registry_module.set_function("create", [&] { return gRegistry.create(); });
  registry_module.set_function(
    "destroy", [&](entt::entity entity) { return gRegistry.destroy(entity); });
  registry_module.set_function(
    "set", [&](entt::entity entity, entt::id_type id, const sol::object &component) {
    if (auto component_type = entt::resolve_type(id); component_type) {
      component_type.func("set"_hs).invoke({}, std::ref(gRegistry), entity,
                                           component);
    }
    });
  registry_module.set_function(
    "get", [&](entt::entity entity, entt::id_type id, const sol::this_state &s) {
      if (auto component_type = entt::resolve_type(id); component_type) {
      auto any = component_type.func("get"_hs).invoke({}, std::ref(gRegistry),
                                                      entity, s);
        return any.cast<sol::object>();
      }
      return sol::make_object(s, sol::lua_nil);
    });
  registry_module.set_function(
    "has", [](entt::entity entity, entt::id_type id) {
      if (auto component_type = entt::resolve_type(id); component_type) {
        auto any =
          component_type.func("has"_hs).invoke({}, std::ref(gRegistry), entity);
        return any.cast<bool>();
      }
      return false;
    });
  registry_module.set_function("size", [&] { return gRegistry.size(); });
  registry_module.set_function("empty", [&] { return gRegistry.empty(); });

  return registry_module;
}

//
//
//

int main(int argc, char *argv[]) {
#ifdef _DEBUG
  _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
  _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
  _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

  try {
    sol::state lua{};
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string);
    lua.require("registry", sol::c_call<AUTO_ARG(&open_registry)>, false);

    struct Transform {
      int x, y;
    };

    register_meta_component<Transform>();

    // clang-format off
    lua.new_usertype<Transform>("Transform",
      "type_id", &entt::type_info<Transform>::id,
      sol::call_constructor,
      sol::factories([](int x, int y) {
        return Transform{ x, y };
      }),
      "x", &Transform::x,
      "y", &Transform::y
    );
    // clang-format on

    lua.script(R"(
      local registry = require('registry')

      entity = registry.create()
      assert(entity == 0 and registry.size() == 1)
      registry.set(entity, Transform.type_id(), Transform(6, 12))
      assert(registry.has(entity, Transform.type_id()))

      transform = registry.get(entity, Transform.type_id())
    )");

    entt::entity entity{ lua["entity"] };
    auto t = gRegistry.try_get<Transform>(entity);
    assert(t);
    Transform transform = lua["transform"];
    assert(t->x == transform.x && t->y == transform.y);
  } catch (const std::exception &e) {
    std::cout << "exception: " << e.what();
    return -1;
  }

  return 0;
}