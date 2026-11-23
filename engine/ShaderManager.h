#pragma once

#include <string>
#include <unordered_map>

#include <SFML/Graphics.hpp>

namespace ECSEngine {

// Owns the game's shaders, keyed by the name the game registered them under.
//
// Lifetime contract:
//   - a pointer from GetShader points at a shader owned by this manager and stays valid until the
//     manager is destroyed. Later RegisterShader calls never invalidate it, because the shaders live
//     in an unordered_map whose nodes do not move when the map grows.
//   - registering the same name twice keeps the first shader, so setup can call it unconditionally.
class ShaderManager {
public:
    ShaderManager() = default;
    ~ShaderManager() = default;

    // Loads a shader from file and remembers it under this name. SFML decides the stage (vertex or
    // fragment) from the file extension: .vert for a vertex shader, .frag or .geom otherwise checked
    // against the type argument.
    void RegisterShader(const std::string& shaderPath, const std::string& shaderName,
                        sf::Shader::Type type = sf::Shader::Type::Fragment);

    // The shader registered under this name, or nullptr if there is none. Doing it this way instead
    // of asserting means a sprite can name a shader that was never registered and simply draw
    // without it, which keeps a scene usable while the shaders are still being written.
    sf::Shader* GetShader(const std::string& shaderName);

    // True when a shader is registered under this name.
    bool HasShader(const std::string& shaderName) const;

private:
    std::unordered_map<std::string, sf::Shader> mShaders;
};

}  // namespace ECSEngine
