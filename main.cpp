#include <vector>
#include <cmath>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"
#include <iostream>
#include <algorithm>

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red = TGAColor(255, 0, 0, 255);
const TGAColor green = TGAColor(0, 255, 0, 255);
Model* model = NULL;
const int width = 800;
const int height = 800;
const int depth = 255;
Vec3f camera(0, 0, 3.f);
//4d-->3d
//除以最后一个分量。（当最后一个分量为0，表示向量）
//不为0，表示坐标
Vec3f m2v(Matrix m) {
    return Vec3f(m[0][0] / m[3][0], m[1][0] / m[3][0], m[2][0] / m[3][0]);
}

//3d-->4d
//添加一个1表示坐标
Matrix v2m(Vec3f v) {
    Matrix m(4, 1);
    m[0][0] = v.x;
    m[1][0] = v.y;
    m[2][0] = v.z;
    m[3][0] = 1.f;
    return m;
}

//视角矩阵
//将物体x，y坐标(-1,1)转换到屏幕坐标(100,700)    1/8width~7/8width
//zbuffer(-1,1)转换到0~255
Matrix viewport(int x, int y, int w, int h) {
    Matrix m = Matrix::identity(4);
    //第4列表示平移信息
    m[0][3] = x + w / 2.f;
    m[1][3] = y + h / 2.f;
    m[2][3] = depth / 2.f;
    //对角线表示缩放信息
    m[0][0] = w / 2.f;
    m[1][1] = h / 2.f;
    m[2][2] = depth / 2.f;
    return m;
}

Vec3f barycentric(Vec3f* pts, Vec3f P) {
    Vec3f r = Vec3f(pts[1].x - pts[0].x, pts[2].x - pts[0].x, pts[0].x - P.x)^Vec3f(pts[1].y - pts[0].y, pts[2].y - pts[0].y, pts[0].y - P.y);
    if (r.z < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.0f - (r.x + r.y) / r.z, r.x / r.z, r.y / r.z);
}

void line(int x0, int y0, int x1, int y1, TGAImage& image, TGAColor color) {
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1)) {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    for (int x = x0; x <= x1; x++) {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1. - t) + y1 * t;
        if (steep) {
            image.set(y, x, color);
        }
        else {
            image.set(x, y, color);
        }
    }
}

void triangle(Vec3f* pts, float *zBuffer, TGAImage& image, TGAColor color) {
    Vec2f min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

    for (int i = 0; i < 3; i++) {
        min.x = std::max(0.f, std::min(min.x, pts[i].x));
        min.y = std::max(0.f, std::min(min.y, pts[i].y));
        max.x = std::min(clamp.x,std::max(max.x, pts[i].x));
        max.y = std::min(clamp.y,std::max(max.y, pts[i].y));
    }
    Vec3f P;
    for (P.x = min.x; P.x <= max.x; P.x++) {
        for (P.y = min.y; P.y <= max.y; P.y++) {
            Vec3f p = barycentric(pts, P);
            if (p.x < 0 || p.y < 0 || p.z < 0) continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) P.z += pts[i].z * p[i];
            if (zBuffer[int(P.y * width + P.x)] < P.z) {
                image.set(P.x, P.y, color);
                zBuffer[int(P.y * width + P.x)] = P.z;
            }
        }
    }
}

void triangle(Vec3f* pts, Vec3f* tex, float* zBuffer, TGAImage& image, TGAImage& source, float intensity) {
    Vec2f min(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f max(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width() - 1, image.get_height() - 1);

    for (int i = 0; i < 3; i++) {
        min.x = std::max(0.f, std::min(min.x, pts[i].x));
        min.y = std::max(0.f, std::min(min.y, pts[i].y));
        max.x = std::min(clamp.x, std::max(max.x, pts[i].x));
        max.y = std::min(clamp.y, std::max(max.y, pts[i].y));
    }

    Vec3f P;
    for (P.x = min.x; P.x <= max.x; P.x++) {
        for (P.y = min.y; P.y <= max.y; P.y++) {
            Vec3f p = barycentric(pts, P);
            Vec3f u;
            if (p.x < 0 || p.y < 0 || p.z < 0) continue;
            P.z = 0;
            for (int i = 0; i < 3; i++) { 
                P.z += pts[i].z * p[i];
                u.x += p[i] * tex[i].x;
                u.y += p[i] * tex[i].y;
            }
            if (zBuffer[int(P.y * width + P.x)] < P.z) {
                TGAColor color = source.get(u.x * source.get_width(), u.y * source.get_height());
                image.set(P.x, P.y, TGAColor(color.r * intensity, color.g * intensity, color.b * intensity, color.a * intensity));
                zBuffer[int(P.y * width + P.x)] = P.z;
            }
        }
    }
}


int main(int argc, char** argv) {

    if (2 == argc) {
        model = new Model(argv[1]);
    }
    else {
        model = new Model("obj/african_head.obj");
    }

    //初始化投影矩阵
    Matrix Projection = Matrix::identity(4);
    //初始化视角矩阵
    Matrix ViewPort = viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);
    //投影矩阵[3][2]=-1/c，c为相机z坐标
    Projection[3][2] = -1.f / camera.z;
    TGAImage frame(width, height, TGAImage::RGB);
    TGAImage a;
    a.read_tga_file("african_head_diffuse.tga");
    a.flip_vertically();
    Vec3f light_dir(0, 0, -1); // define light_dir
    float* zBuffer = new float[width * height];
    for (int i = width * height; i--; zBuffer[i] = -std::numeric_limits<float>::max());

    for (int i = 0; i < model->nfaces(); i++) {
        auto face = model->face(i);
        Vec3f screen_coords[3];
        Vec3f world_coords[3];
        Vec3f tex_coords[3];
        for (int j = 0; j < 3; j++) {
            Vec3f v = model->vert(face.first[j]);
            Vec3f u = model->tex(face.second[j]);
            Vec3f ar = m2v(ViewPort * Projection * v2m(v));
            screen_coords[j] = Vec3f(int(ar.x + .5), int(ar.y + .5), ar.z); //四舍五入防止截断
            world_coords[j] = v;
            tex_coords[j] = u;
        }
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n = n.normalize();
        float intensity = n * light_dir;
        if (intensity > 0) {
            triangle(screen_coords, tex_coords, zBuffer, frame, a, intensity);
        }
    }
    frame.flip_vertically(); // to place the origin in the bottom left corner of the image 
    frame.write_tga_file("output.tga");
    return 0;
}