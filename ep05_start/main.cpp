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

// 光線の向きだけで決まる、空の色。太陽は光が来る向き（to_light）に置く。光が動いたら、太陽も一緒に動く
// The sky color depends only on the ray direction. Put the sun in the direction of the incoming light (to_light), so it moves with the light.
Vec3 sky_color(Ray ray, Vec3 to_light) {
    const Vec3 white = {1.0f, 1.0f, 1.0f};
    const Vec3 sky_blue = {0.1f, 0.3f, 0.95f};
    const Vec3 sun_color = {1.0f, 0.95f, 0.6f};

    // 向きがほぼ太陽と同じ光線だけ、太陽の色にする。0.998 に近いほど小さな太陽になる
    // Only rays almost parallel to the sun direction use the sun color. The closer this value is to 0.998, the smaller the sun appears.
    if (dot(ray.direction, to_light) > 0.998f) {
        return sun_color;
    }

    const float t = 0.5f * (ray.direction.y + 1.0f);  // 下向き 0 〜 上向き 1 / Down is 0; up is 1.
    return mix(white, sky_blue, t);  // t が大きいほど青 / A larger t is more blue.
}

// 球。中心と半径と色で表す
// Sphere. Defined by its center, radius, and color.
struct Sphere {
    Vec3 center;
    float radius;
    Vec3 color;  // 0〜1 の色 / A color from 0 to 1.
};

// 光線が球に当たったら true を返し、当たった距離を t に入れる。外れたら t は触らない
// 光線の上の点は origin + direction * t。この t が「光線を何歩進んだか」になる
// この回で、t の範囲（t_min 〜 t_max）を受け取る形に直す。手前の解が範囲の外なら、奥の解も調べる
// Return true if a ray hits a sphere and store the hit distance in t. If it misses, leave t unchanged. A point on the ray is origin + direction * t; t is how far the ray travels. In this episode, accept a range (t_min to t_max). If the nearer solution is outside it, check the farther one too.
bool hit_sphere(Ray ray, Sphere sphere, float& t) {
    // 光線の上の点が球の表面にある条件は、t の2次方程式 a t^2 + b t + c = 0 になる
    // A point on the ray is on the sphere when t solves the quadratic equation a t^2 + b t + c = 0.
    const Vec3 oc = ray.origin - sphere.center;
    const float a = dot(ray.direction, ray.direction);
    const float b = 2.0f * dot(oc, ray.direction);
    const float c = dot(oc, oc) - sphere.radius * sphere.radius;

    // 判別式がマイナスなら、光線のどこも球に届かない
    // If the discriminant is negative, the ray never reaches the sphere.
    const float discriminant = b * b - 4.0f * a * c;
    if (discriminant < 0.0f) {
        return false;
    }

    // 解は2つ（球に入る点と出る点）。見えるのは手前なので、t が小さいほうを使う
    // There are two solutions: the entry and exit points on the sphere. The nearer one is visible, so use the smaller t.
    const float t_near = (-b - std::sqrt(discriminant)) / (2.0f * a);

    // 判別式は、光線を前にも後ろにも伸びる直線として調べている。t がマイナスは、カメラの後ろで当たったということ
    // カメラの後ろは見えないので、当たりにしない。カメラが球の中にいる場合は、この回では扱わない
    // The discriminant checks the ray as a line extending in both directions. A negative t is a hit behind the camera. We cannot see behind the camera, so ignore it. This episode does not handle a camera inside a sphere.
    if (t_near <= 0.0f) {
        return false;
    }

    t = t_near;
    return true;
}

// 球は全部ここに並べる
// All spheres are listed here.
const int sphere_count = 1;
const Sphere spheres[sphere_count] = {
    {Vec3{-1.9f, 0.0f, 3.8f}, 1.0f, Vec3{0.9f, 0.2f, 0.2f}},
    // この回で、球を3つに増やす。最後に、地面になる巨大な球を足す
    // Increase the number of spheres to three in this episode. Add a huge sphere for the ground at the end.
};

// ここに、この回で書くものを足していく
//   Hit … 光線が最初に当たったものの情報（距離、球の番号、当たった点、法線）
//   hit_scene … 全部の球を調べて、一番手前の当たりを選ぶ関数
// Add what we write in this episode here.
// Hit ... information about the first thing a ray hits (distance, sphere index, hit point, and normal)
// hit_scene ... check all spheres and choose the closest hit

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

    // 光へ向かう向き。当たった点から見て、左上のカメラ寄りに光源がある。平行光源なので、どこでも同じ向き
    // この回の最後で、時間とともに回るようにする
    // The direction toward the light. The source is above and to the left, near the camera. It is the same everywhere for a directional light. Make it rotate over time at the end of this episode.
    const Vec3 to_light = normalize(Vec3{-1.0f, 1.0f, -1.0f});

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

                // 当たらなければ空の色
                // 順に調べて、当たったものの色にする
                // If there is no hit, use the sky color. Check objects in order and use the color of the first hit.
                Vec3 color = sky_color(ray, to_light);
                for (int i = 0; i < sphere_count; i++) {
                    float t = 0.0f;
                    if (!hit_sphere(ray, spheres[i], t)) {
                        continue;
                    }

                    // 当たった点。光線を t 歩進めたところ
                    // The hit point, reached by moving t steps along the ray.
                    const Vec3 hit_point = ray.origin + ray.direction * t;

                    // 法線は、中心から当たった点へ向かう向き。半径で割って長さを1にする
                    // The normal points from the center to the hit point. Divide by the radius to make it unit length.
                    const Vec3 normal = (hit_point - spheres[i].center) * (1.0f / spheres[i].radius);

                    // ランバートの式。明るさは、法線と光へ向かう向きの内積
                    // 光と反対を向いた面は内積がマイナスになるが、光の量はマイナスにならないので 0 にする
                    // Lambert's formula. Brightness is the dot product of the normal and the direction to the light. A surface facing away from the light has a negative dot product; clamp it to 0 because light cannot be negative.
                    const float brightness = std::max(0.0f, dot(normal, to_light));
                    color = spheres[i].color * brightness;

                    // この回で、ここに影を調べる処理を足す
                    // Add the shadow test here in this episode.
                }
                set_pixel(canvas, x, y, to_color(color));
            }
        }

        canvas.present();
    }
}
