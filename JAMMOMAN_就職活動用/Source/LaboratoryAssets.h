#pragma once
#include <DirectXCollision.h>

namespace LaboratoryAssets
{
    inline const char* names[] = {"Wall (1 x 1)",          "Column (height 3)",     "Block (1 x 1 x 1)",
                                  "Computer (height 1.5)", "Capsule (height 2)",    "Pipes (width 3)",
                                  "Vent (width 0.75)",     "Wall pipes (height 2)", "Double door (height 3)",
                                  "Crate (height 0.75)",   "Vessel (height 2)"};
    inline const char* paths[] = {"Data/Model/laboratory/Generated/LabWall_1m.cereal",
                                  "Data/Model/laboratory/Generated/LabColumn_3m.cereal",
                                  "Data/Model/laboratory/Generated/LabBlock_1m.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Props_Computer.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Props_Capsule.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Pipes.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Details_Vent_1.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Details_Pipes_Long.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Door_Double.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Props_Crate.cereal",
                                  "Data/Model/laboratory/Generated/Lab_Props_Vessel_Tall.cereal"};
    inline const DirectX::BoundingBox bounds[] = {
        {{0.0000000f, 0.0000000f, 0.0000000f}, {0.5000000f, 0.5000000f, 0.0650000f}},
        {{0.0000000f, 1.5000000f, 0.0000000f}, {0.2299669f, 1.5000000f, 0.2299672f}},
        {{0.0000000f, 0.0000000f, 0.0000000f}, {0.5000000f, 0.5000000f, 0.5000000f}},
        {{0.0000000f, 0.7500000f, 0.0000000f}, {0.2093355f, 0.7500000f, 0.2322965f}},
        {{0.0000000f, 1.0000000f, 0.0000000f}, {0.5368861f, 1.0000000f, 0.5368862f}},
        {{0.0000000f, 0.0000000f, 0.0000000f}, {1.5000000f, 0.1582444f, 0.0545481f}},
        {{0.0000000f, 0.0000000f, 0.0000000f}, {0.3750000f, 0.2207299f, 0.0293977f}},
        {{0.0000000f, 0.0000000f, 0.0000000f}, {0.3245816f, 1.0000000f, 0.0587934f}},
        {{0.0000000f, 1.5000000f, -0.0000000f}, {1.4256280f, 1.5000000f, 0.2441085f}},
        {{0.0000000f, 0.3750000f, 0.0000000f}, {0.3710750f, 0.3750000f, 0.3710750f}},
        {{0.0000000f, 1.0000000f, 0.0000000f}, {0.2809256f, 1.0000000f, 0.2809256f}}};
    inline constexpr int COUNT = sizeof(names) / sizeof(names[0]);
}
