#include "LoadingPlayer.h"
#include <imgui.h>
#include "Input/Input.h"
#include "Camera.h"
#include "Graphics/Graphics.h"

//コンストラクタ
LoadingPlayer::LoadingPlayer()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    model = std::make_shared<Model>(device, "Data/Model/Jammo/Jammo.gltf");

    scale.x = scale.y = scale.z = 0.01f;

    angle.y = DirectX::XMConvertToRadians(-90);

    position.x = 4.0f;
    position.y = 5.5f;
    position.z = -7.0f;

    auto resource = model->GetResource();
    const auto& meshes = resource->GetMeshes();

    for (auto& mesh : meshes)
    {
        mesh.material->baseColor = DirectX::XMFLOAT4(100.0f, 100.0f, 100.0f, 1.0f); // <-- 白
    }

    model->GetNodePoses(nodePoses);

    model->PlayAnimation(Anim_Run, true, 0.2f, 1.5f);
}

LoadingPlayer::~LoadingPlayer()
{
}

//更新処理
void LoadingPlayer::Update(float elapsedTime)
{
    GamePad& gamePad = Input::Instance().GetGamePad();

    UpdateTransform();

    UpdateVelocity(elapsedTime);

    model->UpdateAnimation(elapsedTime, nodePoses);

    model->SetNodePoses(nodePoses);

    model->UpdateTransform(transform);

    if (position.y < 5.5f)
    {
        position.y = 5.5f;
    }
}

//描画処理
void LoadingPlayer::Render(ModelRenderer* modelRenderer)
{
    modelRenderer->Draw(ShaderId::Toon, model);
}