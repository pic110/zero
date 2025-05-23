#pragma once

#include <zero/ZeroBot.h>
#include <zero/behavior/BehaviorTree.h>
#include <zero/game/Game.h>
#include <zero/game/Logger.h>

namespace zero {
namespace behavior {

// Returns success if the weapon is on cooldown
struct ShipWeaponCooldownQueryNode : public BehaviorNode {
  ShipWeaponCooldownQueryNode(WeaponType type) : type(type) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    u32 current_tick = GetCurrentTick();
    u32 cooldown_tick = current_tick;

    switch (type) {
      case WeaponType::Bullet:
      case WeaponType::BouncingBullet: {
        cooldown_tick = ctx.bot->game->ship_controller.ship.next_bullet_tick;
      } break;
      case WeaponType::Bomb:
      case WeaponType::ProximityBomb:
      case WeaponType::Thor:
      case WeaponType::Burst:
      case WeaponType::Decoy: {
        cooldown_tick = ctx.bot->game->ship_controller.ship.next_bomb_tick;
      } break;
      case WeaponType::Repel: {
        cooldown_tick = ctx.bot->game->ship_controller.ship.next_repel_tick;
      } break;
      default: {
      } break;
    }

    return TICK_GT(cooldown_tick, current_tick) ? ExecuteResult::Success : ExecuteResult::Failure;
  }

  WeaponType type;
};

struct ShipQueryNode : public BehaviorNode {
  ShipQueryNode(int ship) : ship(ship) {}
  ShipQueryNode(const char* ship_key) : ship_key(ship_key) {}
  ShipQueryNode(const char* player_key, int ship) : player_key(player_key), ship(ship) {}
  ShipQueryNode(const char* player_key, const char* ship_key) : player_key(player_key), ship_key(ship_key) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    auto player = ctx.bot->game->player_manager.GetSelf();

    if (player_key) {
      auto opt_player = ctx.blackboard.Value<Player*>(player_key);
      if (!opt_player.has_value()) return ExecuteResult::Failure;

      player = opt_player.value();
    }

    if (!player) return ExecuteResult::Failure;

    int check_ship = this->ship;

    if (ship_key) {
      auto opt_ship = ctx.blackboard.Value<int>(ship_key);
      if (!opt_ship.has_value()) return ExecuteResult::Failure;

      check_ship = opt_ship.value();
    }

    if (check_ship < 0 || check_ship > 8) return ExecuteResult::Failure;

    if (player->ship == check_ship) {
      return ExecuteResult::Success;
    }

    return ExecuteResult::Failure;
  }

  int ship = 0;

  const char* player_key = nullptr;
  const char* ship_key = nullptr;
};

struct ShipRequestNode : public BehaviorNode {
  ShipRequestNode(int ship) : ship(ship) {}
  ShipRequestNode(const char* ship_key) : ship_key(ship_key) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    constexpr s32 kRequestInterval = 300;
    constexpr const char* kLastRequestKey = "last_ship_request_tick";

    auto self = ctx.bot->game->player_manager.GetSelf();

    if (!self) return ExecuteResult::Failure;

    int requested_ship = this->ship;

    if (ship_key) {
      auto opt_ship = ctx.blackboard.Value<int>(ship_key);
      if (!opt_ship.has_value()) return ExecuteResult::Failure;

      requested_ship = opt_ship.value();
    }

    if (self->ship == requested_ship) return ExecuteResult::Success;

    s32 last_request_tick = ctx.blackboard.ValueOr<s32>(kLastRequestKey, 0);
    s32 current_tick = GetCurrentTick();
    s32 next_allowed_tick = MAKE_TICK(last_request_tick + kRequestInterval);

    bool allowed = TICK_GTE(current_tick, next_allowed_tick);

    if (!ctx.blackboard.Has(kLastRequestKey)) {
      allowed = true;
    }

    if (allowed) {
      Log(LogLevel::Info, "Sending ship request for %d.", requested_ship);

      ctx.bot->game->connection.SendShipRequest(requested_ship);

      ctx.blackboard.Set(kLastRequestKey, current_tick);

      return ExecuteResult::Running;
    }

    return ExecuteResult::Running;
  }

  int ship = 0;
  const char* ship_key = nullptr;
};

struct ShipPortalPositionQueryNode : public BehaviorNode {
  ShipPortalPositionQueryNode(const char* output_key = nullptr) : output_key(output_key) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    if (ctx.bot->game->ship_controller.ship.portal_time <= 0.0f) {
      return ExecuteResult::Failure;
    }

    if (output_key) {
      ctx.blackboard.Set(output_key, ctx.bot->game->ship_controller.ship.portal_location);
    }

    return ExecuteResult::Success;
  }

  const char* output_key = nullptr;
};

struct ShipCapabilityQueryNode : public BehaviorNode {
  ShipCapabilityQueryNode(ShipCapabilityFlags cap) : cap(cap) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    Player* player = ctx.bot->game->player_manager.GetSelf();
    if (!player || player->ship >= 8) return ExecuteResult::Failure;

    if (ctx.bot->game->ship_controller.ship.capability & cap) {
      return ExecuteResult::Success;
    }

    return ExecuteResult::Failure;
  }

  ShipCapabilityFlags cap;
};

struct ShipItemCountThresholdNode : public BehaviorNode {
  ShipItemCountThresholdNode(ShipItemType type, u32 count = 1) : type(type), count(count) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    auto& ship = ctx.bot->game->ship_controller.ship;

    u32* items[] = {&ship.repels, &ship.bursts, &ship.decoys, &ship.thors, &ship.bricks, &ship.rockets, &ship.portals};

    size_t index = (size_t)type;
    if (index >= ZERO_ARRAY_SIZE(items)) return ExecuteResult::Failure;

    return *items[index] >= count ? ExecuteResult::Success : ExecuteResult::Failure;
  }

  ShipItemType type;
  u32 count;
};

struct ShipItemCountQueryNode : public BehaviorNode {
  ShipItemCountQueryNode(ShipItemType type, const char* output_key) : type(type), output_key(output_key) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    auto& ship = ctx.bot->game->ship_controller.ship;

    u32* items[] = {&ship.repels, &ship.bursts, &ship.decoys, &ship.thors, &ship.bricks, &ship.rockets, &ship.portals};

    size_t index = (size_t)type;
    if (index >= ZERO_ARRAY_SIZE(items)) return ExecuteResult::Failure;

    ctx.blackboard.Set(output_key, *items[index]);

    return ExecuteResult::Success;
  }

  ShipItemType type = ShipItemType::Repel;
  const char* output_key = nullptr;
};

struct ShipWeaponCapabilityQueryNode : public BehaviorNode {
  ShipWeaponCapabilityQueryNode(WeaponType type, u32 level = 1) : type(type), level(level) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    Player* player = ctx.bot->game->player_manager.GetSelf();
    if (!player || player->ship >= 8) return ExecuteResult::Failure;

    auto& ship = ctx.bot->game->ship_controller.ship;

    switch (type) {
      case WeaponType::Bullet: {
        return ship.guns >= level ? ExecuteResult::Success : ExecuteResult::Failure;
      } break;
      case WeaponType::BouncingBullet: {
        return (ship.capability & ShipCapability_BouncingBullets && ship.guns >= level) ? ExecuteResult::Success
                                                                                        : ExecuteResult::Failure;
      } break;
      case WeaponType::Bomb: {
        return ship.bombs >= level ? ExecuteResult::Success : ExecuteResult::Failure;
      } break;
      case WeaponType::ProximityBomb: {
        return (ship.capability & ShipCapability_Proximity && ship.bombs >= level) ? ExecuteResult::Success
                                                                                   : ExecuteResult::Failure;
      } break;
      case WeaponType::Repel: {
        return ship.repels >= level ? ExecuteResult::Success : ExecuteResult::Failure;
      } break;
      case WeaponType::Decoy: {
        return ship.decoys >= level ? ExecuteResult::Success : ExecuteResult::Failure;
      } break;
      case WeaponType::Burst: {
        return ship.bursts >= level ? ExecuteResult::Success : ExecuteResult::Failure;
      } break;
      case WeaponType::Thor: {
        return ship.thors >= level ? ExecuteResult::Success : ExecuteResult::Failure;
      } break;
      default: {
      } break;
    }

    return ExecuteResult::Failure;
  }

  WeaponType type;
  u32 level;
};

struct ShipMultifireQueryNode : public BehaviorNode {
  ExecuteResult Execute(ExecuteContext& ctx) override {
    Player* player = ctx.bot->game->player_manager.GetSelf();
    if (!player || player->ship >= 8) return ExecuteResult::Failure;

    return ctx.bot->game->ship_controller.ship.multifire ? ExecuteResult::Success : ExecuteResult::Failure;
  }
};

// Returns Success if we can currently use a mine.
// This checks self mine count, team mine count, if mine is directly in our position, and bomb cooldown.
struct ShipMineCapableQueryNode : public BehaviorNode {
  ExecuteResult Execute(ExecuteContext& ctx) override {
    Player* self = ctx.bot->game->player_manager.GetSelf();
    if (!self || self->ship >= 8) return ExecuteResult::Failure;

    ShipWeaponCooldownQueryNode cooldown_query(WeaponType::Bomb);

    if (cooldown_query.Execute(ctx) == ExecuteResult::Success) {
      return ExecuteResult::Failure;
    }

    size_t self_max_mines = ctx.bot->game->connection.settings.ShipSettings[self->ship].MaxMines;
    size_t team_max_mines = ctx.bot->game->connection.settings.TeamMaxMines;

    size_t self_mine_count = 0;
    size_t team_count = 0;
    bool has_check_mine = false;

    ctx.bot->game->weapon_manager.GetMineCounts(*self, self->position, &self_mine_count, &team_count, &has_check_mine);

    if (has_check_mine || self_mine_count >= self_max_mines || team_count >= team_max_mines) {
      return ExecuteResult::Failure;
    }

    return ExecuteResult::Success;
  }
};

struct RepelDistanceQueryNode : public BehaviorNode {
  RepelDistanceQueryNode(const char* output_key) : output_key(output_key) {}

  ExecuteResult Execute(ExecuteContext& ctx) override {
    Player* player = ctx.bot->game->player_manager.GetSelf();
    if (!player || player->ship >= 8) return ExecuteResult::Failure;

    float dist = ctx.bot->game->connection.settings.RepelDistance / 16.0f;

    ctx.blackboard.Set(output_key, dist);

    return ExecuteResult::Success;
  }

  const char* output_key = nullptr;
};

}  // namespace behavior
}  // namespace zero
