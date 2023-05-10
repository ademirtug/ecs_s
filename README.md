# ecs_s
Sparse set based ECS implementation.

## Requirements
- [msvc](https://visualstudio.microsoft.com/) **>= 2022**

## Installation

- ecs_s is a header only library.
```cpp
#include "ecs_s.hpp"
```

## Basic Usage

```cpp
#include "ecs_s.hpp"

using namespace ecs_s;

struct model{
  void draw();
};

class renderer_system : public sub_system<std::chrono::nanoseconds> {
public:
    renderer_system(){}
    void renderer_system::process(ecs_s::registry& world, std::chrono::nanoseconds interval) {
      world.view<std::shared_ptr<model>>([&](ecs_s::entity e, std::shared_ptr<model>& m) {
          m->draw();
      });

};

int main()
{
  renderer_system renderer;
  registry world;
  entity earth = world.new_entity();

  std::shared_ptr<model> world_plate_a = std::make_shared<model>();
  world.add_component(earth, world_plate_a);
  renderer.process(world, std::chrono::nanoseconds(100));
}

```
