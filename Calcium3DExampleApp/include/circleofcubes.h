#ifndef CIRCLEOFCUBES_H
#define CIRCLEOFCUBES_H
#pragma once

#include <c3d/game.h>
#include <c3d/gamescene.h>
#include <c3d/component.h>

class CircleOfCubes
	: public c3d::Component 
{
	GA_CTTI_OBJECT(CircleOfCubes, c3d::Component);

public:
	explicit CircleOfCubes(c3d::GameObject* owner);
	explicit CircleOfCubes(ga::core::CttiDeserializer& deserializer);

};

#endif