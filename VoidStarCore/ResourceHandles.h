#pragma once
#include <cstdint>

namespace star::resource
{
	typedef uint32_t Handle;

	struct MeshHandle
	{
		MeshHandle() = default;

		MeshHandle(Handle h)
		{
			handle = h;
		}

		Handle handle;
	};

	struct TextureHandle
	{
		TextureHandle() = default;

		TextureHandle(Handle h)
		{
			handle = h;
		}

		Handle handle;
	};

	struct MaterialHandle
	{
		MaterialHandle() = default;

		MaterialHandle(Handle h)
		{
			handle = h;
		}

		Handle handle;
	};

	struct ModelHandle
	{
		ModelHandle() = default;

		ModelHandle(Handle h)
		{
			handle = h;
		}

		Handle handle;
	};
}
