#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

#include "CameraComponent.h"
#include "CollisionComponent.h"
#include "CollisionUtil.h"
#include "EntityManager.h"
#include "InputComponent.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "ScoreComponent.h"
#include "SoundManager.h"
#include "SoundNames.h"

namespace ECSEngine {

// How hard a hit shakes, per unit of speed into the wall. Small enough that a walk into a wall is a
// nudge and a full-speed dash is a shove.
const float kShakePerSpeed = 0.3f;
const int kShakeFrames = 8;

// How much speed a loose object keeps after hitting a wall. Losing some on every bounce is what the
// assignment asks for, and it also stops a star rattling forever in a corner.
const float kStarBounceDamping = 0.72f;

// Below this speed a bouncing object stops instead of jittering against the wall forever.
const float kStarRestSpeed = 0.35f;

// Runs after movement, so every box is already at its new position. Two passes:

//   1. resolve  -- every dynamic collider against every static collider; the moving one is pushed
//                  back out along the shallower overlap axis and its side flags are set for the
//                  next frame.
//   2. collect  -- every star overlapping the player is removed and its value added to the score.

// Resolution is O(dynamic * static) on purpose. The level has a fixed set of static tiles and only a
// handful of dynamic entities, so a broad phase would cost more than it saves.
template <typename... Components>
class CollisionSystem : public System<Components...> {
public:
    const std::string& GetName() const override {
        static std::string name("CollisionSystem");
        return name;
    }

    tSystemReturn Run(EngineView<Components...>* engine) override {
        auto& entities = engine->GetEntityManager();

        ResolveStatics(engine, entities);
        ResolveDynamicPairs(entities);
        CollectStars(engine, entities);

        return kContinue;
    }

private:
    // Play a registered sound if the game set one up. See the note in InputSystem: guarding on
    // HasSound keeps the systems runnable without audio.
    static void Play(EngineView<Components...>* engine, const char* name) {
        auto& sounds = engine->GetSoundManager();
        if (sounds.HasSound(name)) sounds.GetSound(name).play();
    }

    // Push every dynamic collider out of every static one, then set its contact flags.
    static void ResolveStatics(EngineView<Components...>* engine,
                               EntityManager<Components...>& entities) {
        for (const Entity& moverEntity : entities) {
            const EntityID moverID = moverEntity.GetID();
            if (!entities.IsValid(moverID)) continue;
            if (!entities.template HasComponent<CollisionComponent>(moverID)) continue;
            if (!entities.template HasComponent<LocationComponent>(moverID)) continue;

            auto& mover = entities.template GetComponent<CollisionComponent>(moverID);
            if (mover.staticEntity) continue;

            // The player owns an InputComponent; every other mover is a loose object (a star), which
            // bounces and does not make a footstep when it settles.
            const bool isPlayer = entities.template HasComponent<InputComponent>(moverID);

            // Where we were before this frame's resolve. A "hit" is a transition from no contact to
            // contact, so all four have to be read before the flags are cleared. Without the edges the
            // shake re-fires every frame the player leans into a wall, because the input system
            // re-applies the velocity each frame.
            const bool wasOnGround = mover.onGround;
            const bool wasOnCeiling = mover.onCeiling;
            const bool wasOnLeftWall = mover.onLeftWall;
            const bool wasOnRightWall = mover.onRightWall;

            mover.onGround = false;
            mover.onCeiling = false;
            mover.onLeftWall = false;
            mover.onRightWall = false;

            auto& moverLoc = entities.template GetComponent<LocationComponent>(moverID);

            // A mover with no velocity cannot be resolved meaningfully; the bounce and shake below
            // both need one. MovementSystem moved every mover by exactly its velocity this frame,
            // so subtracting it recovers the box the tile loop started from.
            if (!entities.template HasComponent<MovementComponent>(moverID)) continue;
            auto& velocity = entities.template GetComponent<MovementComponent>(moverID).velocity;
            const Point2D startTopLeft = mover.collisionBox.topLeft + (moverLoc.loc - velocity);

            for (const Entity& tileEntity : entities) {
                const EntityID tileID = tileEntity.GetID();
                if (tileID == moverID) continue;
                if (!entities.IsValid(tileID)) continue;
                if (!entities.template HasComponent<CollisionComponent>(tileID)) continue;
                if (!entities.template HasComponent<LocationComponent>(tileID)) continue;

                const auto& tile = entities.template GetComponent<CollisionComponent>(tileID);
                if (!tile.staticEntity) continue;

                const auto& tileLoc = entities.template GetComponent<LocationComponent>(tileID);

                const Rect moverBox = CollisionUtil::WorldBox(mover, moverLoc);
                const Rect tileBox = CollisionUtil::WorldBox(tile, tileLoc);
                if (!CollisionUtil::Overlaps(moverBox, tileBox)) continue;

                // Overlaps() counts a shared edge as a collision -- that is what keeps a resting
                // collider grounded. But a zero overlap on one axis is only a *touch*, not
                // penetration: a tile flush against our side must not be resolved vertically, and
                // one under our feet must not be resolved sideways. Without this split, pressing
                // into a stacked-tile wall resolves along a tile seam, pushes the player up and
                // flags it on-ground, so it floats there.
                const float overlapX =
                    std::min(moverBox.topLeft.x + moverBox.width, tileBox.topLeft.x + tileBox.width) -
                    std::max(moverBox.topLeft.x, tileBox.topLeft.x);
                const float overlapY =
                    std::min(moverBox.topLeft.y + moverBox.height, tileBox.topLeft.y + tileBox.height) -
                    std::max(moverBox.topLeft.y, tileBox.topLeft.y);
                if (overlapX <= 0.0f && overlapY <= 0.0f) continue;  // corner touch, nothing to resolve

                // Who bounces and who stops? The player stops against a wall; a loose object (star)
                // ricochets and loses speed rather than sticking to it.
                const bool bounces = !isPlayer;

                const auto old_vel = velocity; // velocity prior to detecting the collision

                const Point2D depth = CollisionUtil::Penetration(moverBox, tileBox);

                // A zero overlap on one axis is a flush touch, so resolve on that axis. A shared
                // edge must not count as a face crossing, or sliding up a stacked-tile wall would
                // read every seam above the mover as a ceiling.
                bool horizontal;
                if (overlapY > 0.0f && overlapX <= 0.0f) {
                    horizontal = true;   // flush against a side face
                } else if (overlapX > 0.0f && overlapY <= 0.0f) {
                    horizontal = false;  // standing on / under it
                } else {
                    // Buried on both axes because the mover is pressed into a wall while moving
                    // vertically: it clips the next tile up by only a few units while the wall face
                    // is deeper. Which axis to push on cannot be read off the depths -- the seam
                    // overlap is sometimes the smaller one and sometimes the larger -- so use where
                    // the mover started. A box that began entirely to one side came in through that
                    // side (a shared side edge counts, since a flush mover is already touching), and
                    // a side scrape wins: resolving sideways keeps the rise and leaves the wall.
                    // Otherwise a strict top/bottom crossing is a landing or a ceiling bump.
                    const float startRight = startTopLeft.x + moverBox.width;
                    const float startBottom = startTopLeft.y + moverBox.height;
                    const bool cameFromSide = startRight <= tileBox.topLeft.x ||
                                              startTopLeft.x >= tileBox.topLeft.x + tileBox.width;
                    const bool cameFromEnd = startBottom < tileBox.topLeft.y ||
                                             startTopLeft.y > tileBox.topLeft.y + tileBox.height;
                    if (cameFromSide) horizontal = true;
                    else if (cameFromEnd) horizontal = false;
                    else horizontal = std::abs(depth.x) < std::abs(depth.y);  // already inside
                }
                if (horizontal) {
                    moverLoc.loc.x += depth.x;
                    // Which side the tile is on decides the flag. The sign of depth.x is useless for
                    // a flush contact (it is zero), so read the geometry: a tile entirely to one side
                    // is a wall on that side. A real overlap (tile straddling us) falls back to depth.
                    const bool tileOnLeft = tileBox.topLeft.x + tileBox.width <= moverBox.topLeft.x;
                    const bool tileOnRight = tileBox.topLeft.x >= moverBox.topLeft.x + moverBox.width;
                    const bool wallOnLeft = tileOnLeft ? true : tileOnRight ? false : (depth.x > 0.0f);
                    if (wallOnLeft) {
                        mover.onLeftWall = true;
                        if (!wasOnLeftWall && old_vel.x < 0) AddShake(entities, moverID, old_vel);
                    } else {
                        mover.onRightWall = true;
                        if (!wasOnRightWall && old_vel.x > 0) AddShake(entities, moverID, old_vel);
                    }

                    // Reflect off the vertical face, keeping a fraction of the speed. Stopping dead
                    // would make a spawned star die on the first wall it met.
                    velocity.x = bounces ? -velocity.x * kStarBounceDamping : 0.0f;
                } else {
                    moverLoc.loc.y += depth.y;
                    const bool tileAbove = tileBox.topLeft.y + tileBox.height <= moverBox.topLeft.y;
                    const bool tileBelow = tileBox.topLeft.y >= moverBox.topLeft.y + moverBox.height;
                    const bool groundBelow = tileBelow ? true : tileAbove ? false : (depth.y <= 0.0f);
                    if (groundBelow) {
                        mover.onGround = true;
                        if (!wasOnGround && old_vel.y > 0) AddShake(entities, moverID, old_vel);
                    } else {
                        mover.onCeiling = true;
                        if (!wasOnCeiling && old_vel.y < 0) AddShake(entities, moverID, old_vel);
                    }

                    velocity.y = bounces ? -velocity.y * kStarBounceDamping : 0.0f;
                }

                // A bouncer that has slowed to a crawl is parked on the wall it hit, so zero it
                // rather than let it twitch against the surface every frame.
                if (bounces && std::abs(velocity.x) < kStarRestSpeed &&
                    std::abs(velocity.y) < kStarRestSpeed) {
                    velocity = {0.0f, 0.0f};
                }
            }

            // Landing is an edge: off the ground last frame and on it now. Played once per mover,
            // after every tile is considered -- inside the tile loop it fires again for each floor
            // tile the player overlaps. Only the player lands audibly; a settling star does not.
            if (isPlayer && !wasOnGround && mover.onGround) Play(engine, SoundName::kLand);
        }
    }

    // Two loose objects that run into each other. The assignment leaves the exact behaviour open,
    // so this is an elastic swap: they push apart along the line between their centres and trade
    // velocity along that line. Bouncing off each other is more interesting to watch than passing
    // through, and it keeps the field from collapsing into a pile.
    static void ResolveDynamicPairs(EntityManager<Components...>& entities) {
        std::vector<EntityID> bouncers;
        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<CollisionComponent>(id)) continue;
            if (!entities.template HasComponent<LocationComponent>(id)) continue;
            if (!entities.template HasComponent<MovementComponent>(id)) continue;
            if (entities.template HasComponent<InputComponent>(id)) continue;  // the player handles itself
            if (entities.template GetComponent<CollisionComponent>(id).staticEntity) continue;
            bouncers.push_back(id);
        }

        for (size_t i = 0; i < bouncers.size(); ++i) {
            for (size_t j = i + 1; j < bouncers.size(); ++j) {
                const EntityID aID = bouncers[i];
                const EntityID bID = bouncers[j];

                const auto& aCol = entities.template GetComponent<CollisionComponent>(aID);
                const auto& bCol = entities.template GetComponent<CollisionComponent>(bID);
                const auto& aLoc = entities.template GetComponent<LocationComponent>(aID);
                const auto& bLoc = entities.template GetComponent<LocationComponent>(bID);

                const Rect aBox = CollisionUtil::WorldBox(aCol, aLoc);
                const Rect bBox = CollisionUtil::WorldBox(bCol, bLoc);
                if (!CollisionUtil::Overlaps(aBox, bBox)) continue;

                const Point2D depth = CollisionUtil::Penetration(aBox, bBox);

                // Separate along the shallower axis, moving both half of the overlap each.
                auto& aLocMut = entities.template GetComponent<LocationComponent>(aID);
                auto& bLocMut = entities.template GetComponent<LocationComponent>(bID);
                if (std::abs(depth.x) < std::abs(depth.y)) {
                    aLocMut.loc.x += depth.x * 0.5f;
                    bLocMut.loc.x -= depth.x * 0.5f;
                } else {
                    aLocMut.loc.y += depth.y * 0.5f;
                    bLocMut.loc.y -= depth.y * 0.5f;
                }

                // Trade the velocity along the axis they were separated on, damped, so the two
                // stars rebound off one another rather than merging.
                auto& aVel = entities.template GetComponent<MovementComponent>(aID).velocity;
                auto& bVel = entities.template GetComponent<MovementComponent>(bID).velocity;
                if (std::abs(depth.x) < std::abs(depth.y)) {
                    std::swap(aVel.x, bVel.x);
                    aVel.x *= kStarBounceDamping;
                    bVel.x *= kStarBounceDamping;
                } else {
                    std::swap(aVel.y, bVel.y);
                    aVel.y *= kStarBounceDamping;
                    bVel.y *= kStarBounceDamping;
                }
            }
        }
    }

    // A star is a dynamic collider that moves and is not the player: the player owns an InputComponent,
    // a star owns a MovementComponent. The score holder is the one entity with a ScoreComponent and no
    // collider, so a star carries no score of its own -- every star is worth kScorePoints.
    static void CollectStars(EngineView<Components...>* engine,
                             EntityManager<Components...>& entities) {
        std::vector<EntityID> collected;

        // Find the score holder once, outside the loop.
        EntityID holder = std::numeric_limits<EntityID>::max();
        for (const Entity& entity : entities) {
            const EntityID id = entity.GetID();
            if (!entities.IsValid(id)) continue;
            if (!entities.template HasComponent<ScoreComponent>(id)) continue;
            if (entities.template HasComponent<CollisionComponent>(id)) continue;
            holder = id;
            break;
        }
        if (holder == std::numeric_limits<EntityID>::max()) return;

        for (const Entity& starEntity : entities) {
            const EntityID starID = starEntity.GetID();
            if (starID == holder) continue;
            if (!entities.IsValid(starID)) continue;
            if (!entities.template HasComponent<CollisionComponent>(starID)) continue;
            if (!entities.template HasComponent<LocationComponent>(starID)) continue;
            if (!entities.template HasComponent<MovementComponent>(starID)) continue;

            const auto& starCol = entities.template GetComponent<CollisionComponent>(starID);
            if (starCol.staticEntity) continue;

            const auto& starLoc = entities.template GetComponent<LocationComponent>(starID);
            const Rect starBox = CollisionUtil::WorldBox(starCol, starLoc);

            // Only the player collects: an entity with an InputComponent.
            for (const Entity& otherEntity : entities) {
                const EntityID otherID = otherEntity.GetID();
                if (otherID == starID) continue;
                if (!entities.IsValid(otherID)) continue;
                if (!entities.template HasComponent<InputComponent>(otherID)) continue;
                if (!entities.template HasComponent<CollisionComponent>(otherID)) continue;
                if (!entities.template HasComponent<LocationComponent>(otherID)) continue;

                const auto& otherCol = entities.template GetComponent<CollisionComponent>(otherID);
                const auto& otherLoc = entities.template GetComponent<LocationComponent>(otherID);
                if (!CollisionUtil::Overlaps(starBox, CollisionUtil::WorldBox(otherCol, otherLoc)))
                    continue;

                entities.template GetComponent<ScoreComponent>(holder).score += kScorePoints;
                Play(engine, SoundName::kStar);
                collected.push_back(starID);
                break;
            }
        }

        // Removal happens after the scan, never during it: RemoveEntity frees a slot and could hand
        // the same slot straight back to the next caller.
        for (EntityID id : collected) entities.RemoveEntity(id);
    }

    // Kick the camera opposite the motion, scaled by how fast we were going into the contact, so a
    // slow walk is a nudge and a dash into a wall is a real jolt. `hitVelocity` is the velocity from
    // before the resolve: by now the collision axis has been zeroed, and reading it here would always
    // measure zero and shake nothing.
    static void AddShake(EntityManager<Components...>& entities, EntityID mover,
                         const Point2D& hitVelocity) {
        if (!entities.template HasComponent<MovementComponent>(mover)) return;
        if (!entities.template HasComponent<CameraShake>(mover)) return;

        const float speed =
            std::sqrt(hitVelocity.x * hitVelocity.x + hitVelocity.y * hitVelocity.y);
        if (speed < 0.01f) return;

        auto& shake = entities.template GetComponent<CameraShake>(mover);
        shake.frameLimit = kShakeFrames;
        shake.offset = {-hitVelocity.x * kShakePerSpeed, -hitVelocity.y * kShakePerSpeed};
    }
};

}  // namespace ECSEngine
