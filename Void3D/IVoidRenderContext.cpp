#include "pch.h"
#include "IVoidRenderContext.h"

#ifdef VOID_STAR_D3D12_BACKEND
#include "VoidRenderContext_D3D12.h"
#endif


IVoidRenderContext* IVoidRenderContext::create()
{
#ifdef VOID_STAR_D3D12_BACKEND
	return new VoidRenderContext_D3D12();
#endif
}
