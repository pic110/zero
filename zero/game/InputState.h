#ifndef ZERO_INPUTSTATE_H_
#define ZERO_INPUTSTATE_H_

#include <zero/Types.h>

namespace zero {

// TODO: All of this needs redone to handle os repeating presses correctly

// Reserved key presses that exist in ascii space outside of normal characters
#define ZERO_KEY_BACKSPACE 8
#define ZERO_KEY_ENTER 10
#define ZERO_KEY_PASTE 26
#define ZERO_KEY_ESCAPE 27

#define ZERO_KEY_MOD_SHIFT (1 << 0)
#define ZERO_KEY_MOD_CONTROL (1 << 1)
#define ZERO_KEY_MOD_ALT (1 << 2)

enum class InputAction {
  Left,
  Right,
  Forward,
  Backward,
  Afterburner,
  Bomb,
  Bullet,
  Mine,
  Thor,
  Burst,
  Multifire,
  Antiwarp,
  Stealth,
  Cloak,
  XRadar,
  Repel,
  Warp,
  Portal,
  Decoy,
  Rocket,
  Brick,
  Attach,
  StatBoxCycle,
  StatBoxPrevious,
  StatBoxNext,
  StatBoxPreviousPage,
  StatBoxNextPage,
  StatBoxHelpNext,
  Play,
  DisplayMap,
  ChatDisplay,
};

using CharacterCallback = void (*)(void* user, int codepoint, int mods);
using ActionCallback = void (*)(void* user, InputAction action);

struct InputState {
  u32 actions = 0;
  CharacterCallback callback = nullptr;
  ActionCallback action_callback = nullptr;
  void* user = nullptr;

  void Clear() { actions = 0; }

  void ClearWeapons() {
    u32 movement_mask = (1 << (u32)InputAction::Left) | (1 << (u32)InputAction::Right) |
                        (1 << (u32)InputAction::Forward) | (1 << (u32)InputAction::Backward) |
                        (1 << (u32)InputAction::Afterburner);
    actions &= movement_mask;
  }

  void SetAction(InputAction action, bool value) {
    size_t action_bit = (size_t)action;

    if (value) {
      actions |= (1 << action_bit);
    } else {
      actions &= ~(1 << action_bit);
    }
  }

  void OnCharacter(int codepoint, int mods = 0) {
    if (callback) {
      callback(user, codepoint, mods);
    }
  }

  void OnAction(InputAction action) {
    if (action_callback) {
      action_callback(user, action);
    }
  }

  void SetCallback(CharacterCallback callback, void* user) {
    this->user = user;
    this->callback = callback;
  }

  bool IsDown(InputAction action) const { return actions & (1 << (size_t)action); }
};

}  // namespace zero

#endif
