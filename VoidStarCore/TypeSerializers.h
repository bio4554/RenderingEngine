#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/optional.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/unique_ptr.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/vec2.hpp>

namespace boost::serialization
{
	template<class Archive>
	inline void serialize(Archive& ar, glm::vec3& v, const unsigned int version)
	{
		ar& v.x;
		ar& v.y;
		ar& v.z;
	}

	template<class Archive>
	inline void serialize(Archive& ar, glm::vec4& v, const unsigned int version)
	{
		ar& v.x;
		ar& v.y;
		ar& v.z;
		ar& v.w;
	}

	template<class Archive>
	inline void serialize(Archive& ar, glm::vec2& v, const unsigned int version)
	{
		ar& v.x;
		ar& v.y;
	}
}
