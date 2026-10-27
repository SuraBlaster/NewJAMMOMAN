#include "EnemyActivation.h"
#include "EnemyVisibility.h"

#include <iostream>
#include <stdexcept>

static void Check(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

int main()
{
    using namespace DirectX;

    XMFLOAT4X4 view;
    XMFLOAT4X4 projection;
    XMStoreFloat4x4(&view,
                    XMMatrixLookAtLH(XMVectorSet(0, 0, -10, 1), XMVectorZero(), XMVectorSet(0, 1, 0, 0)));

    for (bool perspective : {false, true})
    {
        XMStoreFloat4x4(&projection, perspective ? XMMatrixPerspectiveFovLH(XMConvertToRadians(60),
                                                                            16.0f / 9.0f, 0.1f, 100)
                                                 : XMMatrixOrthographicLH(20, 12, 0.1f, 100));

        ViewVolume volume(view, projection);

        Check(IsEnemyInView(volume, {9, 0, 0}, 0.45f, 1),
              "Visible enemy outside old 7m range did not activate");
        Check(IsEnemyInView(volume, {0, 4, 0}, 0.45f, 1), "Visible upper-floor enemy did not activate");
        Check(IsEnemyInView(volume, {-9, 0, 0}, 0.45f, 1), "Visible left enemy did not activate");
        Check(!IsEnemyInView(volume, {15, 0, 0}, 0.45f, 1), "Offscreen enemy activated");
        Check(!IsEnemyInView(volume, {0, 15, 0}, 0.45f, 1), "Offscreen upper-floor enemy activated");

        EnemyActivation activation;
        Check(!activation.UpdateVisibility(false), "Unseen enemy activated");
        Check(activation.UpdateVisibility(true), "Visible enemy remained asleep");
        Check(activation.UpdateVisibility(false), "Active enemy froze after leaving screen");
    }

    XMStoreFloat4x4(&projection, XMMatrixOrthographicLH(20, 12, 0.1f, 100));
    ViewVolume volume(view, projection);
    Check(IsEnemyInView(volume, {10.3f, 0, 0}, 0.45f, 1), "Partially visible enemy did not activate");

    std::cout
        << "PASS: perspective/orthographic screen activation, both sides, upper floors, edge visibility and latched movement\n";
}
