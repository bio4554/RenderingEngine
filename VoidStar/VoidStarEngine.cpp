#include "VoidStarEngine.h"

#include <tracy/Tracy.hpp>
#include <format>
#include <fstream>

#include "InputSystem.h"
#include "MeshBaker.h"
#include "ModelConverter.h"
#include "ResourceRegistry.h"
#include "UpdateObjectsJob.h"
#include "VoidFigure.h"
#include "VoidLight.h"
#include "VoidObject.h"
#include "../FoundationObjects/PlayerCamera.h"
#include "../imgui/imgui.h"

namespace star
{
	VoidStarEngine::VoidStarEngine()
	{
		_renderContext = nullptr;
	}


	void VoidStarEngine::Init(VoidExtent2D windowExtent, SDL_Window* window)
	{
		// setup render backend
		_renderContext = IVoidRenderContext::create();
		_renderContext->init(windowExtent, window);

		// setup renderer
		_renderer = new renderer::Renderer(_renderContext);
		_renderer->Init(windowExtent);

		// setup input handler
		system::GInputSystem = new system::InputSystem();

		// setup resource registry
		resource::GResourceRegistry = new resource::ResourceRegistry(_renderContext);

		// allocate world
		_world = std::make_unique<game::World>();

		// debug
		InitDebugWorld();
	}

	void VoidStarEngine::Run()
	{
		printf("Enter loop");
		while (system::GInputSystem->Is(system::InputButton::Quit, system::ButtonState::Up) && system::GInputSystem->Is(system::InputButton::Escape, system::ButtonState::Up))
		{
			ZoneScopedN("Loop tick");
			auto builder = core::JobBuilder();

			// SDL has to pump events on the main thread
			{
				ZoneScopedN("Process SDL events");
				system::GInputSystem->ProcessEvents();
			}

			builder.DispatchWait(jobs::UpdateObjectsJob::Execute(_world.get(), 0.f));

			//auto updateFence = builder.ExtractCounter();

			//std::shared_ptr<core::SyncCounter> renderCounter = nullptr;

			builder.Dispatch("Render world", [this]
				{
					ZoneScopedN("Render world");
					DrawImGui();
					_renderer->Render(_world.get());
				});

			builder.WaitAll();
			
			auto stats = _renderContext->get_memory_usage_stats();

			auto availableMb = stats.available / 1000000;
			auto usedMb = stats.used / 1000000;

			ImGui::Begin("VRAM usage");
			ImGui::Text(std::format("Available: {}mb", availableMb).c_str());
			ImGui::Text(std::format("Used: {}mb", usedMb).c_str());
			ImGui::End();

			float camPos[3];
			camPos[0] = _world->active_camera->position.x;
			camPos[1] = _world->active_camera->position.y;
			camPos[2] = _world->active_camera->position.z;

			ImGui::Begin("Camera stat");
			ImGui::InputFloat3("Cam pos", camPos);
			ImGui::End();

			_world->active_camera->position.x = camPos[0];
			_world->active_camera->position.y = camPos[1];
			_world->active_camera->position.z = camPos[2];

			FrameMark;
		}
	}

	void VoidStarEngine::Shutdown()
	{
		delete core::GTaskSystem;
		delete resource::GResourceRegistry;
		delete system::GInputSystem;
		delete _renderer;

		_renderContext->shutdown();

		delete _renderContext;
	}

	void VoidStarEngine::InitDebugWorld()
	{
		printf("Importing model...\n");
		
		auto modelPath = "C:\\Users\\bio4554\\source\\repos\\RenderingEngine\\test-resources\\sponza-new\\main1_sponza\\NewSponza_Main_Yup_003.fbx";
		
		auto scale = 100.f;
		auto model = asset_baker::ModelConverter::ImportScene(modelPath);

		auto baker = asset_baker::MeshBaker(&model);
		baker.Bake();

		_world->root_object = std::make_shared<game::Object>();

		auto curtainsModel = asset_baker::ModelConverter::ImportScene("C:\\Users\\bio4554\\source\\repos\\RenderingEngine\\test-resources\\sponza-new\\pkg_a_curtains\\NewSponza_Curtains_FBX_YUp.fbx");

		baker = asset_baker::MeshBaker(&curtainsModel);
		baker.Bake();

		LoadImportedModel(model, 1.f);
		LoadImportedModel(curtainsModel, 1.f);

		auto camera = std::make_shared<foundation::PlayerCamera>();
		camera->fov = 90.f;
		camera->aspectRatio = 16.f / 9.f;
		camera->far = 6000.f;
		camera->near = 1.f;
		_world->root_object->children.push_back(camera);
		_world->active_camera = camera;


		auto sunLight = std::make_shared<game::Light>();
		sunLight->direction = { 0.f, -1.f, 0.69f };
		sunLight->intensity = 1.f;
		sunLight->color = { 1.f, 1.f, 1.f };

		_world->root_object->children.push_back(sunLight);
	}

	template<typename T>
	void debug_write_raw(std::vector<char>& out, T value)
	{
		auto p = reinterpret_cast<char*>(&value);
		for (uint32_t i = 0; i < sizeof(T); i++)
		{
			out.push_back(p[i]);
		}
	}

	void debug_write_model(asset_baker::ImportedModelData& model)
	{
		std::vector<char> output;
		std::vector<glm::vec3> vertices;
		std::vector<uint32_t> indices;

		uint32_t vertexOffset = 0;
		for (auto& mesh : model.meshes)
		{
			for (auto& v : mesh.vertices)
			{
				vertices.push_back(v.position);
			}

			for (auto& i : mesh.indices)
			{
				indices.push_back(i + vertexOffset);
			}

			vertexOffset += static_cast<uint32_t>(mesh.vertices.size());
		}

		// write vertex count
		debug_write_raw(output, static_cast<uint32_t>(vertices.size()));

		for (const auto& v : vertices)
		{
			debug_write_raw(output, v.x);
			debug_write_raw(output, v.y);
			debug_write_raw(output, v.z);
		}

		debug_write_raw(output, static_cast<uint32_t>(indices.size()));

		for (auto i : indices)
		{
			debug_write_raw(output, i);
		}

		std::ofstream fs("test.mod", std::ios::out | std::ios::binary | std::ios::app);
		fs.write(output.data(), output.size());
		fs.close();
	}

	void VoidStarEngine::LoadImportedModel(asset_baker::ImportedModelData& model, float scale)
	{
		debug_write_model(model);
		printf("Registering model...\n");
		std::unordered_map<std::string, resource::MaterialHandle> matMap;
		for (auto& m : model.materials)
		{
			auto texture = std::make_shared<graphics::Texture>();
			texture->height = m.second.albedo.height;
			texture->width = m.second.albedo.width;
			texture->imageData = m.second.albedo.data;
			auto handle = resource::GResourceRegistry->RegisterTexture(texture);

			auto material = std::make_shared<graphics::Material>(handle);

			if (m.second.hasNormal)
			{
				auto normal = std::make_shared<graphics::Texture>();
				normal->height = m.second.normal.height;
				normal->width = m.second.normal.width;
				normal->imageData = m.second.normal.data;
				auto normalHandle = resource::GResourceRegistry->RegisterTexture(normal);

				material->normal = normalHandle;
			}

			if (m.second.hasMetalness)
			{
				auto metalness = std::make_shared<graphics::Texture>();
				metalness->height = m.second.metalness.height;
				metalness->width = m.second.metalness.width;
				metalness->imageData = m.second.metalness.data;
				auto metalnessHandle = resource::GResourceRegistry->RegisterTexture(metalness);

				material->metalness = metalnessHandle;
			}

			if (m.second.hasRoughness)
			{
				auto roughness = std::make_shared<graphics::Texture>();
				roughness->height = m.second.roughness.height;
				roughness->width = m.second.roughness.width;
				roughness->imageData = m.second.roughness.data;
				auto roughnessHandle = resource::GResourceRegistry->RegisterTexture(roughness);

				material->roughness = roughnessHandle;
			}

			if(m.second.hasAlpha)
			{
				auto alpha = std::make_shared<graphics::Texture>();
				alpha->height = m.second.alpha.height;
				alpha->width = m.second.alpha.width;
				alpha->imageData = m.second.alpha.data;
				auto alphaHandle = resource::GResourceRegistry->RegisterTexture(alpha);

				material->alpha = alphaHandle;
			}

			auto matHandle = resource::GResourceRegistry->RegisterMaterial(material);

			matMap[m.first] = matHandle;
		}

		std::vector<resource::MeshHandle> meshes;
		for (auto& m : model.meshes)
		{
			auto mesh = std::make_shared<graphics::Mesh>(matMap[m.materialKey]);
			mesh->vertices = m.vertices;
			mesh->meshlets = m.meshlets;
			mesh->numMeshlets = static_cast<uint32_t>(m.meshlets.size());
			auto handle = resource::GResourceRegistry->RegisterMesh(mesh);
			meshes.push_back(handle);
		}

		printf("Build objects...\n");

		for (auto m : meshes)
		{
			auto figure = std::make_shared<game::Figure>();
			figure->meshHandle = m;
			figure->transform.scale = { scale, scale, scale };
			_world->root_object->children.push_back(figure);
		}
	}

	void VoidStarEngine::DrawImGui()
	{
		static bool normalMapEnable = false;
		static bool meshletDebug = false;
		static bool enableLight = true;

		ImGui::Begin("Render settings");
		ImGui::Checkbox("Normal maps", &normalMapEnable);
		ImGui::Checkbox("Meshlet debug", &meshletDebug);
		ImGui::Checkbox("Enable light", &enableLight);
		ImGui::End();

		_renderer->renderSettings.normalMapEnable = normalMapEnable ? 1 : 0;
		_renderer->renderSettings.meshletDebug = meshletDebug ? 1 : 0;
		_renderer->renderSettings.enableLight = enableLight ? 1 : 0;

		for(auto o : _world->root_object->children)
		{
			if(o->IsType("Light"))
			{
				auto light = reinterpret_cast<game::Light*>(o.get());
				ImGui::Begin("Light stuff");
				ImGui::DragFloat3("Direction", &light->direction.x, 0.01f, -1.f, 1.f);
				ImGui::End();
			}
		}
	}
}
