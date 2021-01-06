#pragma once

// @see https://github.com/skypjack/entt/wiki/Crash-Course:-runtime-reflection-system
template <typename... Args>
auto invoke_meta_func(entt::id_type type_id, entt::id_type func_id,
                      Args... args) {
  if (auto type = entt::resolve_type(type_id); type) {
    return type.func(func_id).invoke({}, std::forward<Args>(args)...);
  }
  return entt::meta_any{};
}