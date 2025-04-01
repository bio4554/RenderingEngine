#pragma once
#include <atomic>

namespace star::core
{
	class AtomBool
	{
	public:
		AtomBool() : _bool()
		{
			_bool = false;
		}

		AtomBool(const AtomBool& a) : _bool(a.get())
		{
			
		}

		AtomBool& operator=(const AtomBool& other)
		{
			_bool.store(other.get());
			return *this;
		}

		bool get() const
		{
			return _bool.load();
		}

		void store(bool b)
		{
			return _bool.store(b);
		}

	private:
		std::atomic<bool> _bool;
	};
}
