# EnTT meets Sol2

## Build examples
```bash
mkdir build && cdbuild
cmake ..
```

# Registry
[entt/wiki/registry](https://github.com/skypjack/entt/wiki/Crash-Course:-entity-component-system#the-registry-the-entity-and-the-component)

## Goal
- Expose `registry` to Lua scripts:
    * Manage entity lifetime
    * Set components
    * Get component by value or reference
    * `runtime_view`

### c++
```cpp
entt::registry registry{};

struct Transform { int x, y; };

register_meta_component<Transform>();

lua.new_usertype<Transform>("Transform",
    "type_id", &entt::type_info<Transform>::id,
    sol::call_constructor,
    sol::factories([](int x, int y) {
        return Transform{ x, y };
    }),
    "x", &Transform::x,
    "y", &Transform::y
);
```

### lua
```lua
local registry = require('registry')

mario = registry.create()
registry.emplace(mario, Transform.type_id(), Transform(1, 2))
material = registry.get(mario, Material.type_id())

if (registry.has(mario, Transform.type_id())) then
    registry.emplace(mario, DeletionFlag.type_id())
end
```

```lua
-- runtime_view:
-- Utilizes variadic args - pass as many types as you want
registry.view(Transform.type_id(), DeletionFlag.type_id()):each(
    function(entity)
        registry.remove(entity, DeletionFlag.type_id())
    end
)
```

# Cooperative scheduler

[entt/wiki/cooperative-scheduler](https://github.com/skypjack/entt/wiki/Crash-Course:-cooperative-scheduler)

## Goal
- Define process class in Lua and attach it to `scheduler`
- Allow process chaining
### c++ setup

```cpp
entt::scheduler scheduler{};

lua.require('scheduler', ...);

scheduler.update(dt); // inside loop
```

### lua script
```lua
local scheduler = require('scheduler')

local process_a = {}
function process:update(dt)
    -- ...
end
-- define init, succeeded and other methods ...

-- This one differs from entt attach().then().then ... but works the same
scheduler.attach(
    process_a,
    process_b,
    process_n
    -- etc ...
)
```

# Event dispatcher

[entt/wiki/dispatcher](https://github.com/skypjack/entt/wiki/Crash-Course:-events,-signals-and-everything-in-between#event-dispatcher)

## Goal
- Trigger/enqueue events from Lua side
- Listen for events in Lua scripts
- Support disconnection of listeners

### c++ setup
```cpp
entt::dispatcher dispatcher{};

lua.require("dispatcher", ...);

struct an_event { int value; };

register_meta_event<an_event>();

lua.new_usertype<an_event>("an_event",
    "type_id", &entt::type_info<an_event>::id,
    sol::call_constructor,
    sol::factories([](int value) {
        return an_event{ value };
    }),
    "value", &an_event::value
);

dispatcher.update(); // inside loop
```

### lua script
```lua
local dispatcher = require('dispatcher')

listener = {}
function listener.receive(evt)
    -- ...
end
function listener.method(evt)
    -- ...
end

-- You have to store result of `connect()`, otherwise listener will be
-- disconnected in lua garbage collection step.
conn = dispatcher.connect(an_event.type_id(), listener.receive)
dispatcher.connect(another_event.type_id(), listener.method)

dispatcher.trigger(an_event.type_id(), an_event(42))
dispatcher.enqueue(another_event.type_id(), another_event())

conn = nil -- to diconnect listener
```

# License
Code released under [CC0 1.0 Universal](LICENSE)