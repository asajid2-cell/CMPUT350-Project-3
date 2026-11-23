#include "ShaderManager.h"

#include <cassert>

namespace ECSEngine {

void ShaderManager::RegisterShader(const std::string& shaderPath, const std::string& shaderName,
                                   sf::Shader::Type type) {
    // Already loaded under this name: keep the existing shader and do nothing. That makes a second
    // call harmless instead of a reload.
    if (mShaders.find(shaderName) != mShaders.end()) return;

    // operator[] default-constructs the shader in place and hands back a reference. The map owns it,
    // so its address is stable for the manager's whole life -- which is what lets GetShader hand out
    // a plain pointer.
    sf::Shader& shader = mShaders[shaderName];
    const bool loaded = shader.loadFromFile(shaderPath, type);
    assert(loaded && "ShaderManager::RegisterShader could not load the file");
    (void)loaded;  // in a release build an unloaded shader just draws unshaded
}

sf::Shader* ShaderManager::GetShader(const std::string& shaderName) {
    auto it = mShaders.find(shaderName);
    return it == mShaders.end() ? nullptr : &it->second;
}

bool ShaderManager::HasShader(const std::string& shaderName) const {
    return mShaders.find(shaderName) != mShaders.end();
}

}  // namespace ECSEngine
