#include "Character.h"
#include <cassert>
#include <cstdio>

class TestCharacter final : public Character
{
  public:

    void Update(float elapsedTime) override
    {
        UpdateInvincibleTimer(elapsedTime);
    }

    int damageEvents = 0;
    int deathEvents = 0;

  private:

    void OnDamaged() override
    {
        ++damageEvents;
    }

    void OnDead() override
    {
        ++deathEvents;
    }
};

int main()
{
    TestCharacter character;
    assert(character.ApplyDamage(1, 1.0f));
    assert(character.GetHealth() == 4 && character.damageEvents == 1);
    assert(character.GetInvincibleTime() == 1.0f);
    assert(!character.ApplyDamage(1, 1.0f));
    assert(!character.ApplyDamage(1, 0.0f));
    assert(character.GetHealth() == 4 && character.damageEvents == 1);
    character.Update(0.5f);
    assert(!character.ApplyDamage(1, 1.0f));
    character.Update(0.6f);
    assert(character.GetInvincibleTime() == 0.0f);
    assert(character.ApplyDamage(1, 0.0f));
    assert(character.GetHealth() == 3 && character.damageEvents == 2);
    assert(!character.ApplyDamage(0, 1.0f));
    assert(!character.ApplyDamage(-1, 1.0f));
    assert(character.ApplyDamage(10, 1.0f));
    assert(character.IsDead() && character.GetHealth() == 0 && character.deathEvents == 1);
    character.Update(2.0f);
    assert(!character.ApplyDamage(1, 0.0f));
    std::puts("Character damage and invincibility: PASS");
}
