#include "Model/InMapGuide.h"

InMapGuide::InMapGuide(Vector2 position, std::string key, std::string caption)
    : Entity(position, {40.0f, 40.0f}, EntityType::InMapGuide)
    , m_key(std::move(key))
    , m_caption(std::move(caption)) {
}

void InMapGuide::Update(float deltaTime) {
    (void)deltaTime;
}

const std::string& InMapGuide::GetKey() const { return m_key; }
const std::string& InMapGuide::GetCaption() const { return m_caption; }
