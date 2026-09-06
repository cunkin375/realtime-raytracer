#define PI 3.1415926358

// wormhole settings

float a = 2.f;
float M = .1f;

[[shader("compute")]]
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    // keep ray tracing to pixels within the actual image
    // prevents stray pixels to the left of the viewport
    // if (x >= meta_buffer.image_width || y >= meta_buffer.image_height)
    //     return;

}
