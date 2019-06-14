#include "pch.h"
#include "ProceduralAliensMain.h"
#include "Common\DirectXHelper.h"

using namespace ProceduralAliens;
using namespace Windows::Foundation;
using namespace Windows::System::Threading;
using namespace Concurrency;

// Loads and initializes application assets when the application is loaded.
ProceduralAliensMain::ProceduralAliensMain(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
	m_deviceResources(deviceResources)
{
	// Register to be notified if the Device is lost or recreated
	m_deviceResources->RegisterDeviceNotify(this);

	// TODO: Replace this with your app's content initialization.
	m_sceneRenderer = std::unique_ptr<Sample3DSceneRenderer>(new Sample3DSceneRenderer(m_deviceResources));

	m_fpsTextRenderer = std::unique_ptr<SampleFpsTextRenderer>(new SampleFpsTextRenderer(m_deviceResources));

	// TODO: Change the timer settings if you want something other than the default variable timestep mode.
	// e.g. for 60 FPS fixed timestep update logic, call:
	/*
	m_timer.SetFixedTimeStep(true);
	m_timer.SetTargetElapsedSeconds(1.0 / 60);
	*/
}

ProceduralAliensMain::~ProceduralAliensMain()
{
	// Deregister device notification
	m_deviceResources->RegisterDeviceNotify(nullptr);
}

// Updates application state when the window size changes (e.g. device orientation change)
void ProceduralAliensMain::CreateWindowSizeDependentResources() 
{
	// TODO: Replace this with the size-dependent initialization of your app's content.
	m_sceneRenderer->CreateWindowSizeDependentResources();
}

// Updates the application state once per frame.
void ProceduralAliensMain::Update(const std::vector<int>& keysDown)
{
	// Update scene objects.
	m_timer.Tick([&]()
	{
		float deltaTime = m_timer.GetElapsedSeconds();
		if (std::find(keysDown.begin(), keysDown.end(), 87) != keysDown.end()) // w
		{
			m_sceneRenderer->MoveEye(DirectX::XMFLOAT4(0, 0, 10 * deltaTime, 0));
		}
		if (std::find(keysDown.begin(), keysDown.end(), 83) != keysDown.end()) // s
		{
			m_sceneRenderer->MoveEye(DirectX::XMFLOAT4(0, 0, -10 * deltaTime, 0));
		}
		if (std::find(keysDown.begin(), keysDown.end(), 65) != keysDown.end()) // a
		{
			m_sceneRenderer->MoveEye(DirectX::XMFLOAT4(-10 * deltaTime, 0, 0, 0));
		}
		if (std::find(keysDown.begin(), keysDown.end(), 68) != keysDown.end()) // d
		{
			m_sceneRenderer->MoveEye(DirectX::XMFLOAT4(10 * deltaTime, 0, 0, 0));
		}
		if (std::find(keysDown.begin(), keysDown.end(), 16) != keysDown.end()) // shift
		{
			m_sceneRenderer->MoveEye(DirectX::XMFLOAT4(0, -10 * deltaTime, 0, 0));
		}
		if (std::find(keysDown.begin(), keysDown.end(), 32) != keysDown.end()) // space
		{
			m_sceneRenderer->MoveEye(DirectX::XMFLOAT4(0, 10*deltaTime, 0, 0));
		}
		// TODO: Replace this with your app's content update functions.
		m_sceneRenderer->Update(m_timer);
		m_fpsTextRenderer->Update(m_timer);
	});
}

// Renders the current frame according to the current application state.
// Returns true if the frame was rendered and is ready to be displayed.
bool ProceduralAliensMain::Render() 
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return false;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Reset the viewport to target the whole screen.
	auto viewport = m_deviceResources->GetScreenViewport();
	context->RSSetViewports(1, &viewport);

	// Reset render targets to the screen.
	ID3D11RenderTargetView *const targets[1] = { m_deviceResources->GetBackBufferRenderTargetView() };
	context->OMSetRenderTargets(1, targets, m_deviceResources->GetDepthStencilView());

	// Clear the back buffer and depth stencil view.
	context->ClearRenderTargetView(m_deviceResources->GetBackBufferRenderTargetView(), DirectX::Colors::LightGray);
	context->ClearDepthStencilView(m_deviceResources->GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// Render the scene objects.
	// TODO: Replace this with your app's content rendering functions.
	m_sceneRenderer->Render();
	m_fpsTextRenderer->Render();

	return true;
}

// Notifies renderers that device resources need to be released.
void ProceduralAliensMain::OnDeviceLost()
{
	m_sceneRenderer->ReleaseDeviceDependentResources();
	m_fpsTextRenderer->ReleaseDeviceDependentResources();
}

// Notifies renderers that device resources may now be recreated.
void ProceduralAliensMain::OnDeviceRestored()
{
	m_sceneRenderer->CreateDeviceDependentResources();
	m_fpsTextRenderer->CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}
