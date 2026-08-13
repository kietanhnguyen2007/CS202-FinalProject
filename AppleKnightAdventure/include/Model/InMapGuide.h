#pragma once

#include "Model/Entity.h"
#include <string>

class InMapGuide : public Entity {
public:
    InMapGuide(Vector2 position, std::string key, std::string caption = {});

    void Update(float deltaTime) override;

    const std::string& GetKey() const;
    const std::string& GetCaption() const;

private:
    std::string m_key;
    std::string m_caption;
};
