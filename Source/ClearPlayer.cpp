#include "ClearPlayer.h"
#include <imgui.h>
#include "Input/Input.h"
#include "Camera.h"
#include "Graphics/Graphics.h"

static ClearPlayer * instance = nullptr;

ClearPlayer& ClearPlayer::Instance()
{
    return *instance;
}

void ClearPlayer::UpdateWaveState(float elapsedTime)
{
}

//コンストラクタ
ClearPlayer::ClearPlayer()
{
    ID3D11Device* device = Graphics::Instance().GetDevice();

    model = std::make_shared<Model>(device, "Data/Model/Jammo/Jammo.gltf");

    instance = this;

    scale.x = scale.y = scale.z = 0.01f;

    angle.y = DirectX::XMConvertToRadians(-90);

    position.x = 11.0f;
    position.y = 9.0f;
    position.z = -7.0f;

    health = maxHealth = 20;

    model->GetNodePoses(nodePoses);

    //待機ステートへ遷移
    TransitionIdleState();
}

ClearPlayer::~ClearPlayer()
{
}

//更新処理
void ClearPlayer::Update(float elapsedTime)
{
    GamePad& gamePad = Input::Instance().GetGamePad();

    switch (state)
    {
    case State::Idle:
        UpdateIdleState(elapsedTime);
        break;
    case State::Move:
        UpdateMoveState(elapsedTime);
        break;
    case State::Wave:
        UpdateWaveState(elapsedTime);
        break;

    }

    UpdateTransform();

    //走力速度更新
    UpdateVelocity(elapsedTime);

    model->UpdateAnimation(elapsedTime, nodePoses);

    model->SetNodePoses(nodePoses);

    model->UpdateTransform(transform);

    
    timer -= elapsedTime;
    
    if (position.y < 5.5f)
    {
        position.y = 5.5f;
    }
}

void ClearPlayer::TransitionIdleState()
{
    state = State::Idle;

    //待機アニメーション再生
    model->PlayAnimation(Anim_Idle, true, 1.0f);
}

void ClearPlayer::UpdateIdleState(float elapsedTime)
{
    if (timer < 0.0f)
    {
        TransitionMoveState();
    }
}

void ClearPlayer::TransitionMoveState()
{
    state = State::Move;

    //待機アニメーション再生
    model->PlayAnimation(Anim_Run, true, 0.2f,1.5f);
}

void ClearPlayer::UpdateMoveState(float elapsedTime)
{

    position.x -= moveSpeed * elapsedTime;

    if (position.x < 0)
    {
        angle.y = DirectX::XMConvertToRadians(180);
        TransitionWaveState();
    }
}

void ClearPlayer::TransitionWaveState()
{
    state = State::Wave;

    //待機アニメーション再生
    model->PlayAnimation(Anim_Wave, true, 1.0f);
}

//描画処理
void ClearPlayer::Render(ModelRenderer* modelRenderer)
{
    modelRenderer->Draw(ShaderId::Toon, model);
}