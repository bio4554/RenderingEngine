#pragma once

#include <memory>
#include <vector>

#include "TypeSerializers.h"

namespace star::game
{
	class Object
	{
	public:
		virtual ~Object() = default;
		Object();
		explicit Object(const std::string& type);
		Object(const Object& other);

		inline std::string Type() const
		{
			return type;
		}

		bool IsType(const std::string& name) const
		{
			return type == name;
		}

		virtual std::shared_ptr<Object> DeepClone();

		virtual void Update(float delta);

		template<class Archive>
		void serialize(Archive& ar, const unsigned int version)
		{
			ar& boost::serialization::base_object<Object>(*this);
			ar& type;
		}

		std::vector<std::shared_ptr<Object>> children;
	private:
		std::string type;
	};
}