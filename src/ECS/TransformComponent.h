//
// Created by alex on 4/9/25.
//

#ifndef TRANSFORMCOMPONENT_H
#define TRANSFORMCOMPONENT_H

#include <glm/glm.hpp>

namespace Bcg{
    struct TransformComponent{
      glm::vec3 position = glm::vec3(0.0f);
      glm::vec3 rotation = glm::vec3(0.0f);
      glm::vec3 scale = glm::vec3(1.0f);
      glm::mat4 modelMatrix =glm::mat4(1.0f);
      bool dirty = true;
    };
}

#endif //TRANSFORMCOMPONENT_H
