#pragma once
/*
 * MIT LICENSE
 * Copyright (c) 2019 Vladyslav Joss
 */
namespace core
{
	class World;
	class ISystem
	{
	public:
		virtual ~ISystem() {};

		virtual void OnUpdate(World& world) {};
	};
}