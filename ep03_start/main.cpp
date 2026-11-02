#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>

#include "framework/canvas.h"

// 3D の位置や向き。x が右、y が上、z が奥
// A 3D position or direction. x points right, y points up, and z points away.
struct Vec3 {
    float x;
    float y;
    float z;
};

Vec3 operator+(Vec3 a, Vec3 b) {
    return Vec3{a.x + b.x, a.y + b.y, a.z + b.z};
}

Vec3 operator-(Vec3 a, Vec3 b) {
    return Vec3{a.x - b.x, a.y - b.y, a.z - b.z};
}

// 3つの成分を、まとめて s 倍する
// Multiply all three components by s.
Vec3 operator*(Vec3 v, float s) {
    return Vec3{v.x * s, v.y * s, v.z * s};
}

// 内積。長さ1の向きどうしなら、向きのそろい具合（同じ向きで 1、直角で 0、反対向きで -1）になる
// Dot product. For unit directions, it measures how well they align: 1 for the same direction, 0 for perpendicular, and -1 for opposite directions.
float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

float length(Vec3 v) {
    return std::sqrt(dot(v, v));
}

Vec3 normalize(Vec3 v) {
    return v * (1.0f / length(v));  // 長さで割って、長さを1にする / Divide by the length to make the length 1.
}

// 光線。origin から direction の向きに進む
// A ray travels from origin in the direction of direction.
struct Ray {
    Vec3 origin;
    Vec3 direction;  // 長さ1にしておく / Make the length 1.
};

// 0〜1 の色（Vec3 の x, y, z が赤, 緑, 青）を、画面に出す 0〜255 の Color に直す
// Convert a color from 0 to 1 (Vec3 x, y, z are red, green, blue) to a screen Color from 0 to 255.
Color to_color(Vec3 c) {
    // 0〜1 の外の値をそのまま 255 倍して uint8_t にすると、値が化けてしまう。先に 0〜1 に切り詰める
    // Converting a value outside 0 to 1 directly to uint8_t after multiplying by 255 can wrap it. Clamp it to 0 to 1 first.
    const float r = std::clamp(c.x, 0.0f, 1.0f);
    const float g = std::clamp(c.y, 0.0f, 1.0f);
    const float b = std::clamp(c.z, 0.0f, 1.0f);
    return Color{
        static_cast<uint8_t>(r * 255.0f + 0.5f),  // + 0.5 で四捨五入する / Add 0.5 to round to the nearest integer.
        static_cast<uint8_t>(g * 255.0f + 0.5f),
        static_cast<uint8_t>(b * 255.0f + 0.5f)};
}

// a と b を混ぜる。t が 0 なら a、1 なら b、その間は割合で混ぜる
// Mix a and b. When t is 0, use a; when t is 1, use b; otherwise, mix them by that amount.
Vec3 mix(Vec3 a, Vec3 b, float t) {
    return a * (1.0f - t) + b * t;
}

// 光線の向きだけで決まる、空の色
// The sky color depends only on the ray direction.
Vec3 sky_color(Ray ray) {
    const Vec3 white = {1.0f, 1.0f, 1.0f};
    const Vec3 sky_blue = {0.1f, 0.3f, 0.95f};
    const Vec3 sun_color = {1.0f, 0.95f, 0.6f};

    // 太陽の向き。左上の高いところ
    // The sun direction. High in the top-left.
    const Vec3 sun_direction = normalize(Vec3{-0.2f, 0.42f, 1.0f});

    // 向きがほぼ太陽と同じ光線だけ、太陽の色にする。0.998 に近いほど小さな太陽になる
    // Only rays almost parallel to the sun direction use the sun color. The closer this value is to 0.998, the smaller the sun appears.
    if (dot(ray.direction, sun_direction) > 0.998f) {
        return sun_color;
    }

    const float t = 0.5f * (ray.direction.y + 1.0f);  // 下向き 0 〜 上向き 1 / Down is 0; up is 1.
    return mix(white, sky_blue, t);  // t が大きいほど青 / A larger t is more blue.
}

// ここに、この回で書くものを足していく
//   Sphere     … 球。中心・半径・色（0〜1 の Vec3）
//   hit_sphere … 光線が球に当たったか調べる関数。当たったら、光線を何歩進んだか（t）も返す
// Add what we write in this episode here.
// Sphere ... a sphere with a center, radius, and color (Vec3 from 0 to 1)
// hit_sphere ... check whether a ray hits a sphere. If it does, also return how far along the ray it hit (t).

// (x, y) のピクセルを color にする
// Set the pixel at (x, y) to color.
void set_pixel(Canvas& canvas, int x, int y, Color color) {
    // 画面の外は何もしない。はみ出して pixels の外を書くと、プログラムが壊れるため
    // Do nothing outside the screen. Writing past pixels would break the program.
    if (x < 0 || x >= canvas.width || y < 0 || y >= canvas.height) {
        return;
    }
    canvas.pixels[y * canvas.width + x] = color;
}

int main() {
    Canvas canvas(320, 180, 4);  // 320×180 の絵を、4倍に拡大して表示する / Display a 320x180 image enlarged 4 times.

    const float width = static_cast<float>(canvas.width);
    const float height = static_cast<float>(canvas.height);
    const float aspect = width / height;  // 画面の縦横比。320 ÷ 180 / The screen aspect ratio: 320 / 180.

    // この回で、ここに球を置く。前に赤い球（中心 (0, 0, 3)、半径 1）、カメラの後ろに青い球（中心 (0, 0, -3)、半径 1）
    // Place spheres here in this episode: a red sphere in front (center (0, 0, 3), radius 1) and a blue sphere behind the camera (center (0, 0, -3), radius 1).

    while (canvas.update()) {
        for (int y = 0; y < canvas.height; y++) {
            for (int x = 0; x < canvas.width; x++) {
                // ピクセルの真ん中（+ 0.5）を通る光線を作る。カメラは原点で、+Z を向く
                // スクリーンは z = 1 の位置に置く。横幅が -1 〜 1 になる
                // Make a ray through the pixel center (+ 0.5). The camera is at the origin and faces +Z. Place the screen at z = 1, with a width from -1 to 1.
                const float u = (static_cast<float>(x) + 0.5f) / width * 2.0f - 1.0f;

                // 画面の y は下向きだが、世界の +Y は上向き。だから符号をひっくり返す
                // 縦は横より短いので、縦横比で割って、-1 〜 1 より狭くする。割らないと横に引き伸ばされる
                // Screen y points down, but world +Y points up, so flip the sign. The screen is shorter than it is wide, so divide by the aspect ratio to keep it narrower than -1 to 1; otherwise it stretches horizontally.
                const float v = (1.0f - (static_cast<float>(y) + 0.5f) / height * 2.0f) / aspect;

                const Ray ray = {Vec3{0.0f, 0.0f, 0.0f}, normalize(Vec3{u, v, 1.0f})};

                // この回で、ここを「球に当たったら球の色、外れたら空の色」に差し替える
                // In this episode, replace this with: use the sphere color on a hit, and the sky color otherwise.
                set_pixel(canvas, x, y, to_color(sky_color(ray)));
            }
        }

        canvas.present();
    }
}
