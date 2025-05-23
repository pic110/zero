#pragma once

#include <zero/Actuator.h>
#include <zero/ChatQueue.h>
#include <zero/HeuristicEnergyTracker.h>
#include <zero/InfluenceMap.h>
#include <zero/RenderContext.h>
#include <zero/Steering.h>
#include <zero/behavior/Behavior.h>
#include <zero/game/Game.h>
#include <zero/game/GameEvent.h>
#include <zero/path/Pathfinder.h>

#include <memory>

namespace zero {
namespace behavior {

class BehaviorNode;
struct ExecuteContext;

}  // namespace behavior

struct BehaviorChangeEvent : public Event {
  std::string previous;
  std::string name;

  BehaviorChangeEvent(const std::string& previous, const std::string& name) : previous(previous), name(name) {}
};

struct BotController : EventHandler<PlayerFreqAndShipChangeEvent>,
                       EventHandler<JoinGameEvent>,
                       EventHandler<PlayerEnterEvent>,
                       EventHandler<MapLoadEvent>,
                       EventHandler<DoorToggleEvent>,
                       EventHandler<LoginResponseEvent>,
                       EventHandler<BrickTileEvent>,
                       EventHandler<BrickTileClearEvent> {
  Game& game;

  std::unique_ptr<path::Pathfinder> pathfinder;
  std::unique_ptr<RegionRegistry> region_registry;
  std::string behavior_name;
  InputState* input;
  InputState last_input = {};

  ChatQueue chat_queue;
  behavior::BehaviorRepository behaviors;
  Steering steering;
  Actuator actuator;
  path::Path current_path;

  bool enable_dynamic_path;
  path::DoorSolidMethod door_solid_method;

  HeuristicEnergyTracker energy_tracker;
  InfluenceMap influence_map;

  std::string default_arena;

  BotController(Game& game);

  void Update(RenderContext& rc, float dt, InputState& input, behavior::ExecuteContext& execute_ctx);

  void UpdatePathfinder(float radius);

  void HandleEvent(const JoinGameEvent& event) override;
  void HandleEvent(const PlayerEnterEvent& event) override;
  void HandleEvent(const MapLoadEvent& event) override;
  void HandleEvent(const PlayerFreqAndShipChangeEvent& event) override;
  void HandleEvent(const DoorToggleEvent& event) override;
  void HandleEvent(const BrickTileEvent& event) override;
  void HandleEvent(const BrickTileClearEvent& event) override;
  void HandleEvent(const LoginResponseEvent& event) override;

  struct UpdateEvent : public Event {
    BotController& controller;
    behavior::ExecuteContext& ctx;

    UpdateEvent(BotController& controller, behavior::ExecuteContext& ctx) : controller(controller), ctx(ctx) {}
  };

  void SetBehavior(const std::string& name, std::unique_ptr<behavior::BehaviorNode> tree) {
    std::string previous = behavior_name;

    behavior_name = name;
    behavior_tree = std::move(tree);

    Event::Dispatch(BehaviorChangeEvent(previous, name));
  }

 private:
  std::unique_ptr<behavior::BehaviorNode> behavior_tree;
};

}  // namespace zero
