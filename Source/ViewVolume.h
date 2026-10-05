#pragma once
#include <DirectXCollision.h>
#include <cmath>

// Clip planes from the actual row-vector view/projection matrix. Unlike
// BoundingFrustum::CreateFromMatrix, this supports Camera's perspective/ortho blend.
class ViewVolume
{
public:
    ViewVolume(const DirectX::XMFLOAT4X4& view,const DirectX::XMFLOAT4X4& projection)
    {
        DirectX::XMFLOAT4X4 m;
        DirectX::XMStoreFloat4x4(&m,DirectX::XMLoadFloat4x4(&view)*DirectX::XMLoadFloat4x4(&projection));
        planes[0]={m._14+m._11,m._24+m._21,m._34+m._31,m._44+m._41};
        planes[1]={m._14-m._11,m._24-m._21,m._34-m._31,m._44-m._41};
        planes[2]={m._14+m._12,m._24+m._22,m._34+m._32,m._44+m._42};
        planes[3]={m._14-m._12,m._24-m._22,m._34-m._32,m._44-m._42};
        planes[4]={m._13,m._23,m._33,m._43}; // Direct3D: 0 <= z <= w
        planes[5]={m._14-m._13,m._24-m._23,m._34-m._33,m._44-m._43};
    }
    bool Intersects(const DirectX::BoundingBox& box) const
    {
        // Small world-space guard band keeps boundary rounding conservative.
        constexpr float margin=0.05f;
        for(const auto& p:planes)
        {
            const float center=p.x*box.Center.x+p.y*box.Center.y+p.z*box.Center.z+p.w;
            const float radius=std::abs(p.x)*(box.Extents.x+margin)+
                std::abs(p.y)*(box.Extents.y+margin)+std::abs(p.z)*(box.Extents.z+margin);
            if(center+radius<0)return false;
        }
        return true;
    }
private:
    DirectX::XMFLOAT4 planes[6];
};
