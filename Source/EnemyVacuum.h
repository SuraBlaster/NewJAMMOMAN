#pragma once

#include <d3d11.h>
#include "Enemy.h"

class Player;

class EnemyVacuum final : public Enemy
{
  public:

    // コンストラクタ
    EnemyVacuum(ID3D11Device* device, Player* player);

    // 更新処理
    void Update(float elapsedTime) override;

    void SetDirection(float direction)
    {
        moveDirection = direction < 0.0f ? -1.0f : 1.0f;
    }

    enum class StateId
    {
        None,
        Move,
        Pursuit,
        EnumCount,
    };

    StateId GetState() const
    {
        return currentState;
    }

    class State
    {
      public:

        // コンストラクタ
        explicit State(EnemyVacuum* owner) : owner(owner)
        {
        }

        // デストラクタ
        virtual ~State() = default;

        // 状態に入るときの処理
        virtual void OnEnter() = 0;

        // 現在の状態の更新処理
        virtual void OnUpdate(float elapsedTime) = 0;

      protected:

        EnemyVacuum* owner;
    };

    class MoveState final : public State
    {
      public:

        // コンストラクタ
        explicit MoveState(EnemyVacuum* owner) : State(owner)
        {
        }

        // 状態に入るときの処理
        void OnEnter() override;

        // 現在の状態の更新処理
        void OnUpdate(float elapsedTime) override;

      private:

        const float NORMAL_ROTATION_SPEED = 10.0f;
    };

    class PursuitState final : public State
    {
      public:

        // コンストラクタ
        explicit PursuitState(EnemyVacuum* owner) : State(owner)
        {
        }

        // 状態に入るときの処理
        void OnEnter() override;

        // 現在の状態の更新処理
        void OnUpdate(float elapsedTime) override;

      private:

        const float PURSUIT_ROTATION_SPEED = 20.0f;
    };

  private:

    void SetState(StateId stateId);

    // 状態遷移と現在の状態の更新処理
    void UpdateStateMachine(float elapsedTime);
    float GetPlayerDistance() const;

    // UpdateWallCollision の更新処理
    void UpdateWallCollision(float elapsedTime);

    void OnDead() override
    {
        Destroy();
    }

    Player* player = nullptr;
    StateId currentState = StateId::None;
    StateId nextState = StateId::None;
    std::unique_ptr<State> states[static_cast<size_t>(StateId::EnumCount)];
    float moveSpeed = 1.5f;
    float moveDirection = -1.0f;
    float wallTurnTimer = 0.0f;
    float pursuitSpeed = 3.0f;
    float pursuitStartDistance = 5.0f;
    float pursuitEndDistance = 7.0f;

    float rotationSpeed = 0.0f;
};
