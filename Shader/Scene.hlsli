cbuffer CbScene : register(b7)
{
	row_major float4x4	cb_view_projection;
	float4				cb_light_direction;
	float4				cb_light_color;
	float4				cb_camera_position;
    float3				cb_sky_color;
    float				cb_hemisphere_weight;
    float3				cb_ground_color;
};
