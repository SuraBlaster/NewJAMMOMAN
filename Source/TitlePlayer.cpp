#include "TitlePlayer.h"
#include "Graphics.h"
#include "ModelRenderer.h"
#include <algorithm>

TitlePlayer::TitlePlayer()
{
    model = std::make_shared<Model>(Graphics::Instance().GetDevice(),
        "Data/Model/Jammo/Jammo_Player.gltf");
    InitializeAnimator(model->GetAnimationIndex("Idle_Seq_0"));
    Reset();
}

void TitlePlayer::Reset()
{
    position = {8.0f, GroundY, -3.0f};
    rotation = {0, DirectX::XMConvertToRadians(180), 0};
    scale = {0.016f, 0.016f, 0.016f};
    velocity = {0, 0, 0};
    state = State::Idle;
    timer = 0;
    PlayAnimation(model->GetAnimationIndex("Idle_Seq_0"), true, 0);
    if (animator) animator->Update(0);
    UpdateTransform();
}

void TitlePlayer::BeginStart()
{
    if (state != State::Idle) return;
    state = State::WaitingToJump;
    timer = 1.0f;
}

void TitlePlayer::Update(float elapsedTime)
{
    const float dt = (std::max)(0.0f, elapsedTime);
    float jumpTime = state == State::Jump ? dt : 0;
    if (state == State::WaitingToJump)
    {
        timer -= dt;
        if (timer <= 0)
        {
            jumpTime = -timer;
            state = State::Jump;
            velocity.y = JumpSpeed;
            PlayAnimation(model->GetAnimationIndex("Jump_Start_0_Seq_0"), false, 0.1f);
        }
    }
    if (state == State::Jump)
    {
        position.y += velocity.y * jumpTime - 0.5f * 30.0f * jumpTime * jumpTime;
        velocity.y -= 30.0f * jumpTime;
        if (position.y <= GroundY) Reset();
    }
    if (animator) animator->Update(dt);
    UpdateTransform();
}

void TitlePlayer::Render(ModelRenderer* renderer)
{
    renderer->Draw(ShaderId::Model, model);
}
