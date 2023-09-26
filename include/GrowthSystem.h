#pragma once
#include <vector>
#include <deque>
#include <cassert>

#include <flecs.h>
#include <glm/glm.hpp>

#include "PositionSystem.h"
#include "RenderSystem.h"

template <typename T>
struct Create {
	static auto create(flecs::entity&& entity, const flecs::world& ecs, Position p, Velocity v, Renderer r) {
		entity.add<T>().set(p).set(v).set(r);
		return entity;
	};
};

struct Apple : Create<Apple> {};

struct Head : Create<Head> {};

struct Tail : Create<Tail> {};

struct Snake {
	flecs::entity			   head;
	std::vector<flecs::entity> tail;
};