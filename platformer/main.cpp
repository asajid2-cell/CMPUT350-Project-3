#include <cstddef>
#include <filesystem>
#include <fstream>
#include <cassert>
#include <optional>
#include <string>
#include <sstream>
#include <regex>
#include <unordered_map>
#include <vector>
#include "CameraSystem.h"
#include "CollisionComponent.h"
#include "CollisionLateSystem.h"
#include "CollisionSystem.h"
#include "ECSEngine.h"
#include "EntityManager.h"
#include "GravitySystem.h"
#include "InputComponent.h"
#include "InputSystem.h"
#include "LocationComponent.h"
#include "MathUtil.h"
#include "MovementSystem.h"
#include "ScoreComponent.h"
#include "ScoreSystem.h"
#include "SoundNames.h"
#include "SpawnComponent.h"
#include "SpawnSystem.h"
#include "SpriteComponent.h"
#include "SpriteManager.h"
#include "SpriteSystem.h"
#include <SFML/Graphics/Texture.hpp>

// Where the asset folder lives at run time. Overridable with -path so the exe can be pointed at a
// copy of assets from anywhere; the default matches the copy CMake makes next to the build.
std::string gResourcePath = "assets";

// Frames between star spawns, and how many stars stay alive at once.
static constexpr int kSpawnInterval = 30;

// Where the player starts. The floor is row 10 of world.map (y = 640) and the player is one tile
// tall, so spawning at y = 576 puts it one tile clear of the ground.
static constexpr float kPlayerStartX = 768.0f;
static constexpr float kPlayerStartY = 576.0f;
static constexpr float kPlayerSize = 64.0f;

// The camera never moves in y. The level is 14 rows of 64-unit tiles (896 tall) and the window is
// 768, so a centre of 512 frames rows 2..13 -- the playable half.
static constexpr float kCameraY = 512.0f;

#define COMPONENTS                                                                             \
    ECSEngine::LocationComponent, ECSEngine::MovementComponent, ECSEngine::CollisionComponent, \
        ECSEngine::SpriteComponent, ECSEngine::SpawnComponent, ECSEngine::CameraComponent,     \
        ECSEngine::CameraFollower, ECSEngine::InputComponent, ECSEngine::GravityComponent,     \
        ECSEngine::CameraShake, ECSEngine::ScoreComponent, ECSEngine::RespawnComponent
using Engine = ECSEngine::ECSEngine<COMPONENTS>;

// Loads one .map file into the engine. A dictionary of 1 is sprites only (the sky layer); 2 adds a
// collision box per sprite (the world). Every grid cell becomes an entity at its world position,
// and the spawner symbol S gets a SpawnComponent instead of a collider.
void LoadMap(ECSEngine::SpriteManager& spriteManager, ECSEngine::EntityManager<COMPONENTS>& entityManager,
             const std::string& path, const std::string& file) {
    std::fstream fs;
    fs.open(std::filesystem::path{path} / std::filesystem::path{file}, std::fstream::in);
    assert(!fs.fail());
    std::string line;

    std::getline(fs, line);
    const bool hasBounds = line.back() == '2';
    assert(line.back() == '2' || line.back() == '1');

    struct SpriteData{
        ECSEngine::SpriteID id;
        struct BoundsInfo {
            ECSEngine::Point2D offset;
            float width;
            float height;
        };

        std::optional<BoundsInfo> bounds{};
    };


    std::unordered_map<char, SpriteData> c;
    std::getline(fs, line);
    while(line.substr(0,3) != "map") {
        std::string current_parse;
        std::stringstream line_stream{line};

        std::getline(line_stream, current_parse, ' ');

        const char sym = current_parse[0];

        std::getline(line_stream, current_parse, ' ');
        std::string file_path = (std::filesystem::path{path} / std::filesystem::path{current_parse}).string();


        std::vector<float> pts(hasBounds ? 8 : 4,0.0f);
        for (auto& f : pts) assert(line_stream >> f);

        ECSEngine::Rect textRect{{pts[0],pts[1]},pts[2],pts[3]};

        SpriteData data{
            spriteManager.RegisterTexture(file_path, textRect),
            hasBounds ? SpriteData::BoundsInfo{ECSEngine::Point2D{pts[4], pts[5]}, pts[6] ,pts[7]} : std::optional<SpriteData::BoundsInfo>{} };


        c[sym] = std::move(data);


        std::getline(fs, line);      
    }

    std::array<float, 6> pts = {};
    std::string placeholder;
    std::stringstream l{line};
    size_t i = 0;
    while (l >> placeholder) {
        try {
            pts[i] = std::stof(placeholder);
            ++i;
        } catch (...) {
            // The line is "map origin 0 0 tile 64 64 size 32 12": the words are not numbers, so they
            // throw and are skipped. Only the six numeric fields are kept, in order.
        }
    }

    assert(i == 6);

    ECSEngine::Point2D origin{pts[0], pts[1]};
    float scale = pts[2]; // WARNING: assuming that given tile <width> <height>, <width> = <height>. Because our scaling doest change between x adn y
    ECSEngine::Point2D worldSize{pts[4], pts[5]};

    
    for (size_t row = 0; row < static_cast<size_t>(pts[5]); ++row) {
        std::getline(fs, line);
        assert(line.size() == static_cast<size_t>(pts[4]));

        for (size_t col = 0; col < static_cast<size_t>(pts[4]); ++col) {
            const char symbol = line[col];
            // A '.' or an undefined character is not in the dictionary, so nothing is placed there.
            if (c.find(symbol) == c.end()) continue;

            // The grid cell's world position. Every entity gets its own location, which is what
            // the sprite and collision systems treat as the entity's origin.
            const ECSEngine::Point2D cellLoc{origin.x + static_cast<float>(col) * scale,
                                             origin.y + static_cast<float>(row) * scale};

            // The sprite's draw box is relative to the entity location, so it starts at the cell's
            // top-left and is one tile wide and tall.
            ECSEngine::SpriteComponent sprite{c[symbol].id, {ECSEngine::Point2D{0.0f, 0.0f}, scale, scale}, true};
            ECSEngine::LocationComponent loc{cellLoc};

            const ECSEngine::EntityID id = entityManager.CreateEntity("tile");

            // The spawner tile has no sprite of its own -- it is a marker. It gets a spawn
            // component instead and is otherwise invisible.
            if (symbol == 'S') {
                ECSEngine::SpawnComponent spawner{};
                ECSEngine::SpriteComponent spawnerSprite{sprite};
                spawner.entityID = id;
                spawner.spawnTextureID = c['S'].id;
                spawner.spawnType = "star";
                spawner.timeToNextSpawn = 0;  // first star after a full interval, not immediately
                spawner.timeBetweenSpawns = kSpawnInterval;
                spawner.totalSpawns = -1;  // keep going

                spawnerSprite.bounds = sprite.bounds;
                spawnerSprite.textureID = spawner.spawnTextureID;
                spawnerSprite.worldSpace = true;

                entityManager.template AddComponent<ECSEngine::SpriteComponent>(id, spawnerSprite);
                entityManager.template AddComponent<ECSEngine::SpawnComponent>(id, spawner);
                entityManager.template AddComponent<ECSEngine::LocationComponent>(id, loc);
                continue;
            }

            entityManager.template AddComponent<ECSEngine::SpriteComponent>(id, sprite);
            entityManager.template AddComponent<ECSEngine::LocationComponent>(id, loc);

            if (hasBounds) {
                const auto& info = c[symbol].bounds;
                ECSEngine::CollisionComponent collision{};
                collision.staticEntity = true;
                collision.collisionBox = {info->offset, info->width, info->height};
                entityManager.template AddComponent<ECSEngine::CollisionComponent>(id, collision);
            }
        }
    }








}

int main(int argc, char* argv[]) {
    if (argc >= 3 && argv[1] == std::string("-path")) {
        gResourcePath = argv[2];
    }
    std::cout << "Usage: " << argv[0] << " [-path resource_path]\n";
    std::cout << "Using resource path: " << gResourcePath << "\n";

    // Engine test code
    Engine engine(1024, 768, "Test Engine");

    // Load sounds. From the editor's own sfx set: a jump for leaving the ground, a footstep for
    // landing, the gem chime for a collected star, the error blip for a wall slide, and a higher
    // jump for the dash.
    {
        auto& sounds = engine.GetSoundManager();
        sounds.RegisterSound(gResourcePath + "/sfx_jump.ogg", ECSEngine::SoundName::kJump);
        sounds.RegisterSound(gResourcePath + "/footstep_grass_003.ogg", ECSEngine::SoundName::kLand);
        sounds.RegisterSound(gResourcePath + "/sfx_gem.ogg", ECSEngine::SoundName::kStar);
        sounds.RegisterSound(gResourcePath + "/error_008.ogg", ECSEngine::SoundName::kWallSlide);
        sounds.RegisterSound(gResourcePath + "/sfx_jump-high.ogg", ECSEngine::SoundName::kDash);
    }

    // Create the gameplay scene. Everything the game draws and updates lives in this scene's own
    // entity manager; the engine only owns the window and the shared resources.
    std::shared_ptr<ECSEngine::SceneView<COMPONENTS>> gameplay = engine.MakeScene();
    ECSEngine::EntityManager<COMPONENTS>& entityManager = gameplay->GetEntityManager();
    ECSEngine::SpriteManager& spriteManager = engine.GetSpriteManager();

    // Load maps
    LoadMap(spriteManager, entityManager, gResourcePath, "sky.map");
    LoadMap(spriteManager, entityManager, gResourcePath, "world.map");

    // The scene has its own WindowManager (sharing the engine's window). The camera belongs to the
    // scene now, so the view centre and scale are set there, not on the engine.
    engine.GetWindowManager().GetWindow()->setFramerateLimit(30);
    gameplay->GetWindowManager().SetWorldScale(1);
    gameplay->GetWindowManager().SetWorldCenter({kPlayerStartX, kCameraY});

    // Place player. The character sheet's first 128x128 cell is the orange alien; the sprite is
    // scaled down to a 64-unit box so it fits the tile grid.
    //
    // The player is one entity carrying Location, Movement, Sprite, Collision, Gravity, Input,
    // CameraShake and Respawn. There is no Player class -- the systems find it by asking which
    // entities have the components they need.
    ECSEngine::EntityID player = 0;
    {
        ECSEngine::SpriteID playerSprite = spriteManager.RegisterTexture(
            gResourcePath + "/spritesheet-characters-default.png",
            ECSEngine::Rect{ECSEngine::Point2D{0.0f, 0.0f}, 128.0f, 128.0f});

        player = entityManager.CreateEntity("player");

        ECSEngine::LocationComponent playerLoc{{kPlayerStartX, kPlayerStartY}};
        ECSEngine::MovementComponent playerMove{{0.0f, 0.0f}};
        ECSEngine::SpriteComponent playerSpriteComp{
            playerSprite, {ECSEngine::Point2D{0.0f, 0.0f}, kPlayerSize, kPlayerSize}, true};

        ECSEngine::CollisionComponent playerCollision{};
        playerCollision.collisionBox = {{0.0f, 0.0f}, kPlayerSize, kPlayerSize};
        ECSEngine::GravityComponent playerGravity{{0.0f, 1.0f}};
        ECSEngine::InputComponent playerInput{player};
        ECSEngine::CameraShake playerShake{};
        ECSEngine::RespawnComponent playerSpawn{{kPlayerStartX, kPlayerStartY}};

        entityManager.AddComponent<ECSEngine::LocationComponent>(player, playerLoc);
        entityManager.AddComponent<ECSEngine::MovementComponent>(player, playerMove);
        entityManager.AddComponent<ECSEngine::SpriteComponent>(player, playerSpriteComp);
        entityManager.AddComponent<ECSEngine::CollisionComponent>(player, playerCollision);
        entityManager.AddComponent<ECSEngine::GravityComponent>(player, playerGravity);
        entityManager.AddComponent<ECSEngine::InputComponent>(player, playerInput);
        entityManager.AddComponent<ECSEngine::CameraShake>(player, playerShake);
        entityManager.AddComponent<ECSEngine::RespawnComponent>(player, playerSpawn);
    }

    // Setup score component. The HUD digit sprites come from the tiles sheet, which is where the
    // number glyphs live.
    {
        ECSEngine::ScoreComponent scoreComponent{};
        scoreComponent.score = 0;
        for (int d = 0; d < 10; ++d) {
            // The numerals are one column of the tiles sheet, counting down 9..0 from the top.
            const float cellY = 256.0f + static_cast<float>(9 - d) * 64.0f;
            scoreComponent.digitTextureID[d] = spriteManager.RegisterTexture(
                gResourcePath + "/spritesheet-tiles-default.png",
                ECSEngine::Rect{ECSEngine::Point2D{832.0f, cellY}, 64.0f, 64.0f});
        }

        const ECSEngine::EntityID hud = entityManager.CreateEntity("hud");
        ECSEngine::LocationComponent hudLoc{{8.0f, 8.0f}};
        entityManager.AddComponent<ECSEngine::LocationComponent>(hud, hudLoc);

        // Three digit entities, written by ScoreSystem. They live in screen space, so the camera
        // transform never touches them.
        for (int slot = 0; slot < 3; ++slot) {
            const ECSEngine::EntityID digit = entityManager.CreateEntity("hud-digit");
            ECSEngine::SpriteComponent digitSprite{
                scoreComponent.digitTextureID[0],
                {{hudLoc.loc.x + static_cast<float>(slot) * 30.0f, hudLoc.loc.y}, 32.0f, 32.0f},
                false};
            ECSEngine::LocationComponent digitLoc{{0.0f, 0.0f}};
            entityManager.AddComponent<ECSEngine::SpriteComponent>(digit, digitSprite);
            entityManager.AddComponent<ECSEngine::LocationComponent>(digit, digitLoc);
            scoreComponent.digitID[slot] = digit;
        }

        entityManager.AddComponent<ECSEngine::ScoreComponent>(hud, scoreComponent);
    }

    // Setup Camera. One camera entity, pinned to the player, with a fixed y from the map.
    {
        const ECSEngine::EntityID camera = entityManager.CreateEntity("camera");

        ECSEngine::CameraComponent cameraComponent{};
        cameraComponent.position = {kPlayerStartX, kCameraY};
        cameraComponent.worldUnitsPerPixel = 1.0f;
        cameraComponent.speed = 1.0f;

        ECSEngine::CameraFollower follower{};
        follower.entityID = player;

        entityManager.AddComponent<ECSEngine::CameraComponent>(camera, cameraComponent);
        entityManager.AddComponent<ECSEngine::CameraFollower>(camera, follower);
    }

    // Add all systems to the scene, in the order they run.
    auto& sm = gameplay->GetSystemManager();
    sm.AddSystem(std::make_unique<ECSEngine::CollisionLateSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::InputSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::GravitySystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::MovementSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CollisionSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::ScoreSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::CameraSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpriteSystem<COMPONENTS>>());
    sm.AddSystem(std::make_unique<ECSEngine::SpawnSystem<COMPONENTS>>());

    engine.AddScene(gameplay);
    engine.Run();
    return 0;
}
