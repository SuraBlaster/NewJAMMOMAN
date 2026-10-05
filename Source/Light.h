#pragma once

#include <DirectXMath.h>

struct DirectionalLight
{
	DirectX::XMFLOAT3	direction = { 0.267261f, -0.534522f, 0.801784f };
	DirectX::XMFLOAT3	color = { 1, 1, 1 };
};

class LightManager
{
private:
	LightManager() = default;
	~LightManager() = default;

public:
	static LightManager& Instance()
	{
		static LightManager instance;
		return instance;
	}

	// ディレクショナルライト設定
	void SetDirectionalLight(DirectionalLight& light) { directionalLight = light; }

	// ディレクショナルライト取得
	const DirectionalLight& GetDirectionalLight() const { return directionalLight; }

	// 
	const DirectX::XMFLOAT3& GetSkyColor() const { return skyColor; }
	const DirectX::XMFLOAT3& GetGroundColor() const { return groundColor; }
	float GetHemisphereWeight() const { return hemisphereWeight; }

private:
	DirectionalLight	directionalLight;
	DirectX::XMFLOAT3	skyColor = { 0.5f, 0.5f, 1.0f };
	DirectX::XMFLOAT3	groundColor = { 0.5f, 0.5f, 0.5f };
	float				hemisphereWeight = 0.5f;
};
