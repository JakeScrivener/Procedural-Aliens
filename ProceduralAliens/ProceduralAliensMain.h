#pragma once

#include "Common\StepTimer.h"
#include "Common\DeviceResources.h"
#include "Content\Sample3DSceneRenderer.h"
#include "Content\SampleFpsTextRenderer.h"
#include <vector>

// Renders Direct2D and 3D content on the screen.
namespace ProceduralAliens
{
	class ProceduralAliensMain : public DX::IDeviceNotify
	{
	public:
		ProceduralAliensMain(const std::shared_ptr<DX::DeviceResources>& deviceResources);
		~ProceduralAliensMain();
		void CreateWindowSizeDependentResources();
		void Update(const std::vector<int>& keysDown);
		bool Render();

		// IDeviceNotify
		virtual void OnDeviceLost();
		virtual void OnDeviceRestored();

	private:
		// Cached pointer to device resources.
		std::shared_ptr<DX::DeviceResources> m_deviceResources;

		// TODO: Replace with your own content renderers.
		std::unique_ptr<Sample3DSceneRenderer> m_sceneRenderer;
		std::unique_ptr<SampleFpsTextRenderer> m_fpsTextRenderer;

		// Rendering loop timer.
		DX::StepTimer m_timer;
	};
}