#pragma once


namespace core
{
	// for singleton-like data
	// distinct type to intentionally prevent accidental 
	// addition of type(say int) as Global
	class IGlobal
	{
	public:
		virtual ~IGlobal() = default;
	};
}