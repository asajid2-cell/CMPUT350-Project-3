#pragma once

namespace ECSEngine {

// The names the systems ask SoundManager for. Setup registers each of these once, so the trigger
// sites and the registration share a single spelling instead of three string literals per sound.
namespace SoundName {
constexpr const char* kJump = "jump";
constexpr const char* kLand = "land";
constexpr const char* kStar = "star";
constexpr const char* kWallSlide = "wallSlide";
constexpr const char* kDash = "dash";
}  // namespace SoundName

}  // namespace ECSEngine
