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

namespace Snake {
	static auto addTail(flecs::entity base, flecs::entity tip) {
		tip.child_of(base);
	}

	static auto addToWorld(const flecs::world& ecs, const auto& randColor) {
		auto head  = Head::create(ecs.entity("SnakeHead"), ecs, Position::zero(), Velocity::zero(), Renderer::headColor());
		auto tail0 = Tail::create(ecs.entity("Tail0"), ecs, Position{ { 0, -1 } }, Velocity::zero(), Renderer::random(randColor));
		auto tail1 = Tail::create(ecs.entity("Tail1"), ecs, Position{ { 0, -2 } }, Velocity::zero(), Renderer::random(randColor));
		addTail(head, tail0);
		addTail(tail0, tail1);
		return head;
	}

	static auto reset( flecs::entity head, flecs::world& ecs, const auto& randColor) {
		head.get_mut<Position>()->vec = {0,0};
		head.get_mut<Velocity>()->vec = {0,0};

		const auto tails = ecs.filter<const Tail, Velocity>();

		head.children([](flecs::entity c) {c.destruct();});
		auto tail0 = Tail::create(ecs.entity("Tail0"), ecs, Position{ { 0, -1 } }, Velocity::zero(), Renderer::random(randColor));
		auto tail1 = Tail::create(ecs.entity("Tail1"), ecs, Position{ { 0, -2 } }, Velocity::zero(), Renderer::random(randColor));
		addTail(head, tail0);
		addTail(tail0, tail1);
		return head;
	}
}