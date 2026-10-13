#include <circleofcubes.h>

CircleOfCubes::CircleOfCubes(c3d::GameObject* owner)
	: Component(owner)
{
}

CircleOfCubes::CircleOfCubes(ga::core::CttiDeserializer& deserializer)
	: Component(deserializer)
{
}

void CircleOfCubes::CreateListeCubeTower(unsigned int nbrCube, glm::vec3 center, float r)
{
    constexpr float kBrickW = 1.0f;
    constexpr float kBrickH = 1.0f;
    constexpr float kBrickD = 1.0f;

    float c = 2.0f * r * glm::tan(glm::pi<float>() / float(nbrCube));
    float scale = c / kBrickW;
    float rPrime = r + scale * (kBrickD / 2.0f);

    for (unsigned int X; X < nbrCube; X++)
    {
        float angle = (2.0f * glm::pi<float>()) * (float(X) / float(nbrCube));

        float dx = rPrime * glm::cos(angle);
        float dz = rPrime * glm::sin(angle);

        glm::vec3 cubePos = center + glm::vec3(dx, 0.0f, dz);

    }
}